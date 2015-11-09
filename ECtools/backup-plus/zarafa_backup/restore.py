#!/usr/bin/env python
from contextlib import closing
import cPickle as pickle
import dbhash
import os.path
import time

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
        changes = self.restore_rec(self.data_path, store.subtree)
        self.log.info('restore completed in %.2f seconds (%d changes, ~%.2f/sec)' % (time.time()-t0, changes, changes/(time.time()-t0)))

    def restore_rec(self, path, subtree):
        changes = 0
        for sourcekey in os.listdir(path+'/folders'):
            folder_path = path+'/folders/'+sourcekey
            changes += self.restore_rec(folder_path, subtree) # recursion
            xpath = file(folder_path+'/path').read()
            if self.options.folders and xpath not in self.options.folders:
                continue
            self.log.info('restoring folder %s' % xpath)
            subfolder = subtree.folder(xpath, create=True)
            with closing(dbhash.open(folder_path+'/index', 'c')) as db:
                index = dict((a, pickle.loads(b)) for (a,b) in db.iteritems())
            with closing(dbhash.open(folder_path+'/items', 'c')) as db:
                for sourcekey2 in db.iterkeys():
                    with log_exc(self.log):
                        last_modified = index[sourcekey2]['last_modified']
                        if ((self.options.period_begin and last_modified < self.options.period_begin) or
                            (self.options.period_end and last_modified >= self.options.period_end)):
                            continue
                        self.log.debug('restoring item with sourcekey %s' % sourcekey2)
                        subfolder.create_item(loads=db[sourcekey2])
                        changes += 1
        return changes

def main():
    options, args = zarafa.parser('ckpsufUPlSbe', usage='zarafa-restore PATH [options]').parse_args()
    options.foreground = True
    Service('backup', options=options, args=args, logname='restore').start()
    
if __name__ == '__main__':
    main()
