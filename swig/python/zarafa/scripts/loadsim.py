#! /usr/bin/python

# very simple load simulator

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

parser = zarafa.parser('skpc', usage='loadsim [options]')
parser.add_option('-u', '--user', dest='user', action='store', help='user to send mails to')
parser.add_option('-N', '--n-write-workers', dest='n_write_workers', action='store', help='number of write workers to start')
parser.add_option('-n', '--n-read-workers', dest='n_read_workers', action='store', help='number of read workers to start')
parser.add_option('-r', '--new-session', dest='restart_session', action='store_false', help='start a new session for each iteration')
parser.add_option('-e', '--eml', dest='eml', action='store', help='eml-file to use')

o, a = parser.parse_args()

user = o.user
n_write_processes = int(o.n_write_workers)
n_read_processes = int(o.n_read_workers)
reconnect = o.restart_session
eml_file = o.eml

if eml_file == None:
	print 'EML file missing'
	sys.exit(1)

eml_file_data = file(eml_file).read()

abort = False

def read_worker():
	server = None

	while True:
		try:
			if reconnect == True or server == None:
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

	while True:
		try:
			if reconnect == True or server == None:
				server = zarafa.Server(o)

			item = server.user(user).store.inbox.create_item(eml = eml_file_data)

		except KeyboardInterrupt:
			return

		except Exception, e:
			print e

pids = []

print 'Starting %d writers' % n_write_processes
for i in range(n_write_processes):
	pid = os.fork()

	if pid == 0:
		write_worker()
		sys.exit(0)

	elif pid == -1:
		print 'fork failed'
		break;

	pids.append(pid)

print 'Starting %d readers' % n_read_processes
for i in range(n_read_processes):
	pid = os.fork()

	if pid == 0:
		read_worker()
		sys.exit(0)

	elif pid == -1:
		print 'fork failed'
		break;

	pids.append(pid)

print 'Child processes started'

try:
	while True:
		time.sleep(86400)

except:
	pass

print 'Terminating...'

for pid in pids:
	try:
		os.kill(pid, -9)

	except:
		pass

print 'Finished.'
