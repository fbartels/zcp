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
        config, server = self.service.config, self.service.server
        while True:
            changes = 0
            with log_exc(self.log):
                (storeguid, username, path) = self.iqueue.get()
                assert os.system('mkdir -p %s' % path) == 0
                if username:
                    file(path+'/user', 'w').write(dump_props(server.user(username).props()))
                store = server.store(storeguid)
                file(path+'/store', 'w').write(dump_props(store.props()))
                t0 = time.time()
                self.log.info('backing up: %s' % path)
                changes = self.backup_folder_rec(store, store.subtree, [], path, config)
                self.log.info('backing up %s took %.2f seconds (%d changes, ~%.2f/sec)' % (path, time.time()-t0, changes, changes/(time.time()-t0)))
            self.oqueue.put(changes)

    def backup_folder_rec(self, store, folder, path, basepath, config):
        changes = 0
        folder_path = os.path.join(basepath, *path)
        assert os.system('mkdir -p %s/folders' % folder_path) == 0
        if path: # skip subtree folder itself
            file(folder_path+'/path', 'w').write(folder.path)
            file(folder_path+'/folder', 'w').write(dump_props(folder.props()))
            filter_ = self.service.options.folders
            if not filter_ or folder.path in filter_:
                self.log.info('backing up folder: %s' % folder.path)
                importer = FolderImporter(folder, folder_path, config, self.log)
                statepath = '%s/state' % folder_path
                state = None
                if os.path.exists(statepath):
                    state = file(statepath).read()
                    self.log.info('found previous folder sync state: %s' % state)
                new_state = folder.sync(importer, state, log=self.log)
                if new_state != state:
                    changes += importer.changes + importer.deletes
                    file(statepath, 'w').write(new_state)
                    self.log.info('saved folder sync state: %s' % new_state)
        sub_sourcekeys = set()
        for subfolder in folder.folders(recurse=False):
            sub_sourcekeys.add(subfolder.sourcekey)
            if self.service.options.skip_junk and not store.public and subfolder.sourcekey == store.junk.sourcekey:
                continue
            changes += self.backup_folder_rec(store, subfolder, path+['folders', subfolder.sourcekey], basepath, config) # recursion
        stored_sourcekeys = set([x for x in os.listdir(folder_path+'/folders')])
        for sourcekey in stored_sourcekeys-sub_sourcekeys:
            self.log.info('removing deleted subfolder: %s' % folder_path+'/folders/'+sourcekey)
            assert os.system('rm -rf %s' % folder_path+'/folders/'+sourcekey) == 0
        return changes

class FolderImporter:
    def __init__(self, *args):
        self.folder, self.folder_path, self.config, self.log = args
        self.changes = self.deletes = 0

    def update(self, item, flags):
        with log_exc(self.log):
            self.log.debug('folder %s: new/updated document with sourcekey %s' % (self.folder.sourcekey, item.sourcekey))
            with closing(dbhash.open(self.folder_path+'/items', 'c')) as db:
                db[item.sourcekey] = item.dumps()
            with closing(dbhash.open(self.folder_path+'/index', 'c')) as db:
                db[item.sourcekey] = pickle.dumps({
                    'subject': item.subject,
                    'last_modified': item.last_modified,
                })
            self.changes += 1

    def delete(self, item, flags):
        with log_exc(self.log):
            self.log.debug('folder %s: deleted document with sourcekey %s' % (self.folder.sourcekey, item.sourcekey))
            with closing(dbhash.open(self.folder_path+'/items', 'c')) as db:
                del db[item.sourcekey]
            with closing(dbhash.open(self.folder_path+'/index', 'c')) as db:
                del db[item.sourcekey]
            self.deletes += 1

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
        changes = sum([self.oqueue.get() for i in range(len(jobs))]) # blocking
        self.log.info('queue processed in %.2f seconds (%d changes, ~%.2f/sec)' % (time.time()-t0, changes, changes/(time.time()-t0)))

    def create_jobs(self):
        output_dir = self.options.output_dir or ''
        jobs = []
        if self.options.companies or not (self.options.users or self.options.stores):
            for company in self.server.companies():
                companyname = company.name if company.name != 'Default' else ''
                for user in company.users():
                    jobs.append((user.store, user.name, os.path.join(output_dir, companyname, user.name)))
                if not self.options.skip_public:
                    target = companyname+'@'+companyname if companyname else 'Public'
                    jobs.append((company.public_store, None, os.path.join(output_dir, companyname, target)))
        for username in self.options.users:
            if username == 'Public':
                jobs.append((self.server.public_store, None, os.path.join(output_dir, 'Public')))
            elif '@' in username and username.split('@')[0] == username.split('@')[1]: # XXX
                u, c = username.split('@')
                jobs.append((self.server.company(c).public_store, None, os.path.join(output_dir, username)))
            else:
                user = self.server.user(username)
                jobs.append((user.store, user.name, os.path.join(output_dir, username)))
        for storeguid in self.options.stores:
            jobs.append((self.server.store(storeguid), None, os.path.join(output_dir, storeguid)))
        return [(job[0].guid,)+job[1:] for job in sorted(jobs, reverse=True, key=lambda x: x[0].size)]

def main():
    parser = zarafa.parser('ckpsufwUPCSlO')
    parser.add_option('-J', '--skip-junk', dest='skip_junk', action='store_true', help='do not backup junk mail')
    parser.add_option('-N', '--skip-public', dest='skip_public', action='store_true', help='do not backup public store')
    options, args = parser.parse_args()
    options.foreground = True
    Service('backup', options=options).start()
    
if __name__ == '__main__':
    main()
