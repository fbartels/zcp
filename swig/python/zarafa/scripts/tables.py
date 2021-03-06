#!/usr/bin/env python

# some example of working with MAPI tables

# usage: ./tables.py (change USER to username)

from MAPI.Util import *
import zarafa

USER = 'user1'

server = zarafa.Server()

for table in server.tables():
    print table
    print table.csv(delimiter=';')

for item in server.user('user1').store.inbox:
    print item
    for table in item.tables():
        print table
        for row in table:
            print row
    print item.table(PR_MESSAGE_ATTACHMENTS).text()
    print
