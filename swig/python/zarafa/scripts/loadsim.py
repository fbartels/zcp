#!/usr/bin/env python

# basic loadsim framework

import getopt
import os
import random
import sys
import time
import zarafa

user = 'user1'
n_write_processes = 1
n_read_processes = 1
reconnect = False
eml_file = None

opts, args = getopt.getopt(sys.argv[1:], "u:N:n:re:h", ["user=", "n-write-processes", "n-read-processes=", "reconnect", "eml-file", "help"])

def help():
	print '-u user   user to send mails to'
	print '-N #      number of write workers to start'
	print '-n #      number of read workers to start'
	print '-r        start a new session for each iteration'
	print '-e file   eml-file to use'
	print '-h        this help'

for o, a in opts:
	if o in ('-u', '--user'):
		user = a

	elif o in ('-N', '--n-write-processes'):
		n_write_processes = int(a)

	elif o in ('-n', '--n-read-processes'):
		n_read_processes = int(a)

	elif o in ('-r', '--reconnect'):
		reconnect = True

	elif o in ('-e', '--eml-file'):
		eml_file = a

	elif o in ('-h', '--help'):
		help()
		sys.exit(0)

	else:
		print '%s is not understood' % o
		print
		help()
		sys.exit(1)

if eml_file == None:
	print 'EML file missing'
	sys.exit(1)

eml_file_data = file(eml_file).read()

def read_worker():
	server = None

	if reconnect == False:
		server = zarafa.Server(o)

	while True:
		try:
			if reconnect == True:
				server = zarafa.Server(o)

			u = server.user(user)
			for folder in u.store.folders():
				for item in folder:
					if random.randint(0, 1) == 1:
						dummy = [(att.filename, att.mimetype, len(att.data)) for att in item.attachments()]

		except KeyboardInterrupt:
			return

		except Exception, e:
			print e

def write_worker():
	server = None

	if reconnect == False:
		server = zarafa.Server(o)

	while True:
		try:
			if reconnect == True:
				server = zarafa.Server(o)

			item = server.user(user).store.inbox.create_item(eml = eml_file_data)

		except KeyboardInterrupt:
			return

		except Exception, e:
			print e

print 'Starting %d writers' % n_write_processes
for i in range(n_write_processes):
	if os.fork() == 0:
		write_worker()
		sys.exit(0)

print 'Starting %d readers' % n_read_processes
for i in range(n_read_processes):
	if os.fork() == 0:
		read_worker()
		sys.exit(0)

while True:
	time.sleep(86400)
