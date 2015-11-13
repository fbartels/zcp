#!/usr/bin/env python
from contextlib import closing
import cPickle as pickle
import dbhash
from multiprocessing import Queue
import os.path
import time

import zarafa
from zarafa import log_exc

def dump_props(props):
    return pickle.dumps(dict((prop.proptag, prop.mapiobj.Value) for prop in props))

class BackupWorker(zarafa.Worker):
    def main(self):
        config, server, options = self.service.config, self.service.server, self.service.options
        while True:
            changes = 0
            with log_exc(self.log):
                (storeguid, username, path) = self.iqueue.get()
                store = server.store(storeguid)
                assert os.system('mkdir -p %s' % path) == 0
                if not options.folders:
                    if username:
                        file(path+'/user', 'w').write(dump_props(server.user(username).props()))
                    file(path+'/store', 'w').write(dump_props(store.props()))
                t0 = time.time()
                self.log.info('backing up: %s' % path)
                stats = {'changes': 0, 'deletes': 0, 'errors': 0}
                self.backup_folder_rec(store, store.subtree, [], path, config, stats)
                changes = stats['changes'] + stats['deletes']
                self.log.info('backing up %s took %.2f seconds (%d changes, ~%.2f/sec, %d errors)' % (path, time.time()-t0, changes, changes/(time.time()-t0), stats['errors']))
            self.oqueue.put(stats)

    def backup_folder_rec(self, store, folder, path, basepath, config, stats):
        filter_ = self.service.options.folders
        folder_path = os.path.join(basepath, *path)
        if path: # skip subtree folder itself
            if not filter_ or folder.path in filter_:
                assert os.system('mkdir -p %s/folders' % folder_path) == 0
                file(folder_path+'/path', 'w').write(folder.path)
                file(folder_path+'/folder', 'w').write(dump_props(folder.props()))
                self.log.info('backing up folder: %s' % folder.path)
                importer = FolderImporter(folder, folder_path, config, self.service.options, self.log, stats)
                statepath = '%s/state' % folder_path
                state = None
                if os.path.exists(statepath):
                    state = file(statepath).read()
                    self.log.info('found previous folder sync state: %s' % state)
                new_state = folder.sync(importer, state, log=self.log, stats=stats)
                if new_state != state:
                    file(statepath, 'w').write(new_state)
                    self.log.info('saved folder sync state: %s' % new_state)
        sub_sourcekeys = set()
        for subfolder in folder.folders(recurse=False):
            sub_sourcekeys.add(subfolder.sourcekey)
            if (not store.public and \
                (self.service.options.skip_junk and subfolder == store.junk) or \
                (self.service.options.skip_deleted and subfolder == store.wastebasket)):
                continue
            self.backup_folder_rec(store, subfolder, path+['folders', subfolder.sourcekey], basepath, config, stats) # recursion
        if not filter_:
            if os.path.exists(folder_path+'/folders'):
                stored_sourcekeys = set([x for x in os.listdir(folder_path+'/folders')])
                for sourcekey in stored_sourcekeys-sub_sourcekeys:
                    self.log.info('removing deleted subfolder: %s' % folder_path+'/folders/'+sourcekey)
                    assert os.system('rm -rf %s' % folder_path+'/folders/'+sourcekey) == 0

class FolderImporter:
    def __init__(self, *args):
        self.folder, self.folder_path, self.config, self.options, self.log, self.stats = args

    def update(self, item, flags):
        with log_exc(self.log, self.stats):
            self.log.debug('folder %s: new/updated document with sourcekey %s' % (self.folder.sourcekey, item.sourcekey))
            with closing(dbhash.open(self.folder_path+'/items', 'c')) as db:
                db[item.sourcekey] = item.dumps(attachments=not self.options.skip_attachments)
            with closing(dbhash.open(self.folder_path+'/index', 'c')) as db:
                db[item.sourcekey] = pickle.dumps({
                    'subject': item.subject,
                    'last_modified': item.last_modified,
                })
            self.stats['changes'] += 1

    def delete(self, item, flags):
        with log_exc(self.log, self.stats):
            self.log.debug('folder %s: deleted document with sourcekey %s' % (self.folder.sourcekey, item.sourcekey))
            with closing(dbhash.open(self.folder_path+'/items', 'c')) as db:
                del db[item.sourcekey]
            with closing(dbhash.open(self.folder_path+'/index', 'c')) as db:
                del db[item.sourcekey]
            self.stats['deletes'] += 1

class Service(zarafa.Service):
    def main(self):
        self.iqueue, self.oqueue = Queue(), Queue()
        workers = [BackupWorker(self, 'backup%d'%i, nr=i, iqueue=self.iqueue, oqueue=self.oqueue) for i in range(self.config['worker_processes'])]
        for worker in workers:
            worker.start()
        jobs = self.create_jobs()
        for job in jobs:
            self.iqueue.put(job)
        self.log.info('queued %d store(s) for parallel backup (%s processes)' % (len(jobs), len(workers)))
        t0 = time.time()
        stats = [self.oqueue.get() for i in range(len(jobs))] # blocking
        changes = sum(s['changes'] + s['deletes'] for s in stats)
        errors = sum(s['errors'] for s in stats)
        self.log.info('queue processed in %.2f seconds (%d changes, ~%.2f/sec, %d errors)' % (time.time()-t0, changes, changes/(time.time()-t0), errors))

    def create_jobs(self):
        output_dir = self.options.output_dir or ''
        jobs = []
        if self.options.companies or not (self.options.users or self.options.stores):
            for company in self.server.companies():
                companyname = company.name if company.name != 'Default' else ''
                for user in company.users():
                    jobs.append((user.store, user.name, os.path.join(output_dir, companyname, user.name)))
                if not self.options.skip_public:
                    target = 'public@'+companyname if companyname else 'public'
                    jobs.append((company.public_store, None, os.path.join(output_dir, companyname, target)))
        if self.options.users:
            for user in self.server.users():
                jobs.append((user.store, user.name, os.path.join(output_dir, user.name)))
        if self.options.stores:
            for store in self.server.stores():
                if store.public:
                    target = 'public' + ('@'+store.company.name if store.company.name != 'Default' else '')
                else:
                    target = store.guid
                jobs.append((store, None, os.path.join(output_dir, target)))
        return [(job[0].guid,)+job[1:] for job in sorted(jobs, reverse=True, key=lambda x: x[0].size)]

def main():
    parser = zarafa.parser('ckpsufwUPCSlO')
    parser.add_option('-J', '--skip-junk', dest='skip_junk', action='store_true', help='do not backup junk mail')
    parser.add_option('-D', '--skip-deleted', dest='skip_deleted', action='store_true', help='do not backup deleted mail')
    parser.add_option('-N', '--skip-public', dest='skip_public', action='store_true', help='do not backup public store')
    parser.add_option('-A', '--skip-attachments', dest='skip_attachments', action='store_true', help='do not backup attachments')
    options, args = parser.parse_args()
    options.foreground = True
    Service('backup', options=options).start()
    
if __name__ == '__main__':
    main()
