#!/usr/bin/env python
import fcntl
import glob
import grp
import os
import pwd
import shutil
import subprocess
import sys
import traceback

import zarafa

"""
uses 'xapian-compact' to compact given database(s), specified using store GUID(s),
or all databases found in the 'index_path' directory specified in search.cfg.

"""

# TODO: purge removed users (stores)

def _default(name):
    if name == '':
        return 'root'
    elif name is None:
        return 'zarafa'
    else:
        return name

def main():
    server = zarafa.Server()

    # get index_path, run_as_user, run_as_group from  search.cfg
    index_path = _default(zarafa.Config(None, 'search').get('index_path'))
    search_user = _default(zarafa.Config(None, 'search').get('run_as_user'))
    uid = pwd.getpwnam(search_user).pw_uid
    search_group = _default(zarafa.Config(None, 'search').get('run_as_group'))
    gid = grp.getgrnam(search_group).gr_gid

    if (uid, gid) != (os.getuid(), os.getgid()):
        print 'ERROR: script should run under user/group as specified in search.cfg'
        sys.exit(1)

    # find database(s) corresponding to given store GUID(s)
    dbpaths = []
    if len(sys.argv) > 1:
        for arg in sys.argv[1:]:
            dbpath = os.path.join(index_path, server.guid+'-'+arg)
            dbpaths.append(dbpath)

    # otherwise, select all databases for compaction
    else:
        dbpaths = []
        for path in glob.glob(os.path.join(index_path, server.guid+'-*')):
            if not path.endswith('.lock'):
                dbpaths.append(path)

    # loop and compact
    count = len(dbpaths)
    errors = 0
    for dbpath in dbpaths:
        try:
            print 'compact:', dbpath
            if os.path.isdir(dbpath):
                with open('%s.lock' % dbpath, 'w') as lockfile: # do not index at the same time
                    fcntl.flock(lockfile.fileno(), fcntl.LOCK_EX)
                    shutil.rmtree(dbpath+'.compact', ignore_errors=True)
                    subprocess.call(['xapian-compact', dbpath, dbpath+'.compact'])
                    # quickly swap in compacted database
                    shutil.move(dbpath, dbpath+'.old')
                    shutil.move(dbpath+'.compact', dbpath)
                    shutil.rmtree(dbpath+'.old')
            else:
                print 'ERROR: no such database: %s', dbpath
                errors += 1
            print
        except Exception, e:
            print 'ERROR'
            traceback.print_exc(e)
            errors += 1

    # summarize
    print 'done compacting (%d processed, %d errors)' % (count, errors)

if __name__ == '__main__':
    main()
