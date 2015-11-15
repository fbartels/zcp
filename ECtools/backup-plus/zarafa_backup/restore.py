#!/usr/bin/env python
from contextlib import closing
import cPickle as pickle
import dbhash
import os.path
import time
import zlib

from MAPI.Util import PR_SOURCE_KEY, PR_ZC_ORIGINAL_SOURCE_KEY, MAPIErrorNotFound
import zarafa
from zarafa import log_exc

class Service(zarafa.Service):
    def _store(self, username):
        if '@' in username: # XXX
            u, c = username.split('@')
            if u == c: return self.server.company(c).public_store
            else: return self.server.user(username).store
        elif username == 'Public': return self.server.public_store
        else: return self.server.user(username).store

    def main(self):
        self.data_path = self.args[0] # XXX
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
        self.restore_rec(self.data_path, store.subtree, stats)
        self.log.info('restore completed in %.2f seconds (%d changes, ~%.2f/sec, %d errors)' % (time.time()-t0, stats['changes'], stats['changes']/(time.time()-t0), stats['errors']))

    def restore_rec(self, path, subtree, stats):
        for sourcekey in os.listdir(path+'/folders'):
            folder_path = path+'/folders/'+sourcekey
            self.restore_rec(folder_path, subtree, stats) # recursion
            xpath = file(folder_path+'/path').read().decode('utf8')
            if self.options.folders and xpath not in self.options.folders:
                continue
            self.log.info('restoring folder %s' % xpath)
            subfolder = subtree.folder(self.options.target_folder or xpath, create=True)
            existing = set()
            for item in subfolder:
                for proptag in (PR_SOURCE_KEY, PR_ZC_ORIGINAL_SOURCE_KEY):
                    try:
                        existing.add(item.prop(proptag).mapiobj.Value.encode('hex').upper())
                    except MAPIErrorNotFound:
                        pass
            with closing(dbhash.open(folder_path+'/index', 'c')) as db:
                index = dict((a, pickle.loads(b)) for (a,b) in db.iteritems())
            with closing(dbhash.open(folder_path+'/items', 'c')) as db:
                for sourcekey2 in db.iterkeys():
                    with log_exc(self.log, stats):
                        last_modified = index[sourcekey2]['last_modified']
                        if ((self.options.period_begin and last_modified < self.options.period_begin) or
                            (self.options.period_end and last_modified >= self.options.period_end)):
                            continue
                        if sourcekey2 in existing:
                            self.log.warning('duplicate item with sourcekey %s' % sourcekey2)
                        else:
                            self.log.debug('restoring item with sourcekey %s' % sourcekey2)
                            subfolder.create_item(loads=zlib.decompress(db[sourcekey2]))
                            stats['changes'] += 1

def show_contents_rec(data_path, options):
    if os.path.exists(data_path+'/path'):
        path = file(data_path+'/path').read()
        if not options.folders or path in options.folders:
            name = path.split('/')[-1]
            if options.stats:
                count = 0
                if os.path.exists(data_path+'/index'):
                    with closing(dbhash.open(data_path+'/index', 'c')) as db:
                        count = len(db.keys())
                print path+' '+str(count)
            if options.index:
                print path
                if os.path.exists(data_path+'/index'):
                    with closing(dbhash.open(data_path+'/index', 'c')) as db:
                        items = []
                        for key, value in db.iteritems():
                            d = pickle.loads(value)
                            items.append((key, d))
                        items.sort(key=lambda (k, d): d['last_modified'])
                        for key, d in items:
                            print key, d['last_modified'], d['subject']
    for f in os.listdir(data_path+'/folders'):
        d = data_path+'/folders/'+f
        if os.path.isdir(d):
            show_contents_rec(d, options)

def main():
    parser = zarafa.parser('ckpsufUPlSbe', usage='zarafa-restore PATH [options]')
    parser.add_option('-t', '--target-folder', dest='target_folder', help='restore items to specific folder', metavar='PATH')
    parser.add_option('', '--stats', dest='stats', action='store_true', help='show statistics for backup')
    parser.add_option('', '--index', dest='index', action='store_true', help='show index for backup')
    options, args = parser.parse_args()
    options.foreground = True

    if options.stats or options.index:
        show_contents_rec(args[0], options)
    else:
        Service('backup', options=options, args=args, logname='restore').start()
    
if __name__ == '__main__':
    main()
