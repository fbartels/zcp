#!/usr/bin/env python
import csv
from contextlib import closing
import cPickle as pickle
import dbhash
import shutil
from multiprocessing import Queue
import os.path
import sys
import time
import zlib

from MAPI.Util import PR_SOURCE_KEY, PR_EC_BACKUP_SOURCE_KEY, MAPIErrorNotFound, PT_ERROR, PROP_TYPE, SPropValue

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
                if not os.path.isdir(path):
                    os.makedirs(path)
                if not options.folders:
                    if username:
                        file(path+'/user', 'w').write(dump_props(server.user(username).props()))
                    file(path+'/store', 'w').write(dump_props(store.props()))
                t0 = time.time()
                self.log.info('backing up: %s' % path)
                stats = {'changes': 0, 'deletes': 0, 'errors': 0}
                paths = set()
                folders = list(store.folders())
                if options.recursive:
                    folders = sum([[f]+list(f.folders()) for f in folders], [])
                for folder in folders:
                    if (not store.public and \
                        ((options.skip_junk and folder == store.junk) or \
                         (options.skip_deleted and folder == store.wastebasket))):
                        continue
                    paths.add(folder.path)
                    self.backup_folder(path, folder, store.subtree, config, options, stats)
                path_folder = folder_struct(path, options)
                if not options.folders:
                    for fpath in set(path_folder) - paths:
                        self.log.info('removing deleted folder: %s' % fpath)
                        shutil.rmtree(path_folder[fpath])
                changes = stats['changes'] + stats['deletes']
                self.log.info('backing up %s took %.2f seconds (%d changes, ~%.2f/sec, %d errors)' %
                    (path, time.time()-t0, changes, changes/(time.time()-t0), stats['errors']))
            self.oqueue.put(stats)

    def backup_folder(self, path, folder, subtree, config, options, stats):
        data_path = path+'/'+folder_path(folder, subtree)
        if not os.path.isdir('%s/folders' % data_path):
            os.makedirs('%s/folders' % data_path)
        file(data_path+'/path', 'w').write(folder.path.encode('utf8'))
        file(data_path+'/folder', 'w').write(dump_props(folder.props()))
        self.log.info('backing up folder: %s' % folder.path)
        importer = FolderImporter(folder, data_path, config, options, self.log, stats)
        statepath = '%s/state' % data_path
        state = None
        if os.path.exists(statepath):
            state = file(statepath).read()
            self.log.info('found previous folder sync state: %s' % state)
        new_state = folder.sync(importer, state, log=self.log, stats=stats, begin=options.period_begin, end=options.period_end)
        if new_state != state:
            file(statepath, 'w').write(new_state)
            self.log.info('saved folder sync state: %s' % new_state)

class FolderImporter:
    def __init__(self, *args):
        self.folder, self.folder_path, self.config, self.options, self.log, self.stats = args

    def update(self, item, flags):
        with log_exc(self.log, self.stats):
            self.log.debug('folder %s: new/updated document with sourcekey %s' % (self.folder.sourcekey, item.sourcekey))
            with closing(dbhash.open(self.folder_path+'/items', 'c')) as db:
                db[item.sourcekey] = zlib.compress(item.dumps(attachments=not self.options.skip_attachments, archiver=False))
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
        if self.options.restore:
            self.restore()
        else:
            self.backup()

    def backup(self):
        self.iqueue, self.oqueue = Queue(), Queue()
        workers = [BackupWorker(self, 'backup%d'%i, nr=i, iqueue=self.iqueue, oqueue=self.oqueue)
                       for i in range(self.config['worker_processes'])]
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
        self.log.info('queue processed in %.2f seconds (%d changes, ~%.2f/sec, %d errors)' %
            (time.time()-t0, changes, changes/(time.time()-t0), errors))

    def restore(self):
        self.data_path = self.args[0]
        self.log.info('starting restore of %s' % self.data_path)
        username = os.path.split(self.data_path)[1]
        if self.options.users:
            store = self._store(self.options.users[0])
        elif self.options.stores:
            store = self.server.store(self.options.stores[0])
        else:
            store = self._store(username)
        self.log.info('restoring to store %s' % store.guid)
        t0 = time.time()
        stats = {'changes': 0, 'errors': 0}
        path_folder = folder_struct(self.data_path, self.options)
        paths = self.options.folders or sorted(path_folder.keys())
        if self.options.recursive:
            paths = [path2 for path2 in path_folder for path in paths if path2.startswith(path+'/')]
        for path in paths:
            if path not in path_folder:
                self.log.error('no such folder: %s' % path)
                stats['errors'] += 1
            else:
                restore_path = self.options.restore_root+'/'+path if self.options.restore_root else path
                folder = store.subtree.folder(restore_path, create=True)
                if (not store.public and \
                    ((self.options.skip_junk and folder == store.junk) or \
                    (self.options.skip_deleted and folder == store.wastebasket))):
                        continue
                data_path = path_folder[path]
                self.restore_folder(folder, path, data_path, store, store.subtree, stats)
        self.log.info('restore completed in %.2f seconds (%d changes, ~%.2f/sec, %d errors)' %
            (time.time()-t0, stats['changes'], stats['changes']/(time.time()-t0), stats['errors']))

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

    def restore_folder(self, folder, path, data_path, store, subtree, stats):
        if self.options.sourcekeys:
            with closing(dbhash.open(data_path+'/items', 'c')) as db:
                if not [sk for sk in self.options.sourcekeys if sk in db]:
                    return
        else:
            self.log.info('restoring folder %s' % path)
        existing = set()
        table = folder.mapiobj.GetContentsTable(0)
        table.SetColumns([PR_SOURCE_KEY, PR_EC_BACKUP_SOURCE_KEY], 0)
        for row in table.QueryRows(-1, 0):
            if PROP_TYPE(row[1].ulPropTag) != PT_ERROR:
                existing.add(row[1].Value)
            else:
                existing.add(row[0].Value)
        with closing(dbhash.open(data_path+'/index', 'c')) as db:
            index = dict((a, pickle.loads(b)) for (a,b) in db.iteritems())
        with closing(dbhash.open(data_path+'/items', 'c')) as db:
            sourcekeys = db.keys()
            if self.options.sourcekeys:
                sourcekeys = [sk for sk in sourcekeys if sk in self.options.sourcekeys]
            for sourcekey2 in sourcekeys:
                with log_exc(self.log, stats):
                    last_modified = index[sourcekey2]['last_modified']
                    if ((self.options.period_begin and last_modified < self.options.period_begin) or
                        (self.options.period_end and last_modified >= self.options.period_end)):
                        continue
                    if sourcekey2.decode('hex') in existing:
                        self.log.warning('skipping duplicate item with sourcekey %s' % sourcekey2)
                    else:
                        self.log.debug('restoring item with sourcekey %s' % sourcekey2)
                        item = folder.create_item()
                        item.loads(zlib.decompress(db[sourcekey2]), attachments=not self.options.skip_attachments)
                        try:
                            item.prop(PR_EC_BACKUP_SOURCE_KEY)
                        except MAPIErrorNotFound:
                            item.mapiobj.SetProps([SPropValue(PR_EC_BACKUP_SOURCE_KEY, sourcekey2.decode('hex'))])
                            item.mapiobj.SaveChanges(0)
                        stats['changes'] += 1

    def _store(self, username):
        if '@' in username: # XXX
            u, c = username.split('@')
            if u == c: return self.server.company(c).public_store
            else: return self.server.user(username).store
        elif username == 'Public': return self.server.public_store
        else: return self.server.user(username).store

def folder_struct(data_path, options, mapper={}):
    if os.path.exists(data_path+'/path'):
        path = file(data_path+'/path').read()
        mapper[path] = data_path
    if os.path.exists(data_path+'/folders'):
        for f in os.listdir(data_path+'/folders'):
            d = data_path+'/folders/'+f
            if os.path.isdir(d):
                folder_struct(d, options, mapper)
    return mapper

def folder_path(folder, subtree):
    path = ''
    parent = folder
    while parent and parent != subtree:
        path = '/folders/'+parent.sourcekey+path
        parent = parent.parent
    return path[1:]

def show_contents(data_path, options):
    writer = csv.writer(sys.stdout)
    path_folder = folder_struct(data_path, options)
    paths = options.folders or sorted(path_folder)
    for path in paths:
        if path not in path_folder:
            print 'no such folder:', path
            sys.exit(-1)
    for path in paths:
        data_path = path_folder[path]
        items = []
        if os.path.exists(data_path+'/index'):
            with closing(dbhash.open(data_path+'/index', 'c')) as db:
                for key, value in db.iteritems():
                    d = pickle.loads(value)
                    if ((options.period_begin and d['last_modified'] < options.period_begin) or
                        (options.period_end and d['last_modified'] >= options.period_end)):
                        continue
                    items.append((key, d))
        if options.stats:
            writer.writerow([path, len(items)])
        elif options.index:
            items.sort(key=lambda (k, d): d['last_modified'])
            for key, d in items:
                writer.writerow([key, path, d['last_modified'], d['subject'].encode(sys.stdout.encoding or 'utf8')])

def main():
    parser = zarafa.parser('ckpsufwUPCSlObe', usage='zarafa-backup [PATH] [options]')
    parser.add_option('-J', '--skip-junk', dest='skip_junk', action='store_true', help='do not backup junk mail')
    parser.add_option('-D', '--skip-deleted', dest='skip_deleted', action='store_true', help='do not backup deleted mail')
    parser.add_option('-N', '--skip-public', dest='skip_public', action='store_true', help='do not backup public store')
    parser.add_option('-A', '--skip-attachments', dest='skip_attachments', action='store_true', help='do not backup attachments')
    parser.add_option('', '--restore', dest='restore', action='store_true', help='restore from backup')
    parser.add_option('', '--restore-root', dest='restore_root', help='restore under specific folder', metavar='PATH')
    parser.add_option('', '--stats', dest='stats', action='store_true', help='show statistics for backup')
    parser.add_option('', '--index', dest='index', action='store_true', help='show index for backup')
    parser.add_option('', '--sourcekey', dest='sourcekeys', action='append', help='restore specific sourcekey', metavar='SOURCEKEY')
    parser.add_option('', '--recursive', dest='recursive', action='store_true', help='backup/restore folders recursively')
    options, args = parser.parse_args()
    options.foreground = True
    if options.restore or options.stats or options.index:
        assert (len(args) == 1 and os.path.isdir(args[0])), 'please specify path to backup data'
    if options.stats or options.index:
        show_contents(args[0], options)
    else:
        Service('backup', options=options, args=args).start()

if __name__ == '__main__':
    main()
