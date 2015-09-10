#!/usr/bin/env python

# print store structure, down to attachments

# usage: ./tree.py [-u username]

import zarafa

server = zarafa.Server()
for user in server.users(): # checks -u/--user command-line option
     print user
     for folder in user.store.folders():
         indent = (folder.depth+1)*'  '
         print indent + unicode(folder)
         for item in folder:
             print indent + '  ' + unicode(item)
             for attachment in item.attachments():
                 print indent + '    ' + attachment.filename 
