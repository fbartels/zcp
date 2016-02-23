#!/usr/bin/env python

# playing around with a python-zarafa based version of zarafa-admin (far from complete, patches/ideas welcome!)

# usage: ./zarafa-admin.py -h

from MAPI.Util import PR_EC_STATSTABLE_SYSTEM, PR_DISPLAY_NAME, PR_EC_STATS_SYSTEM_VALUE # XXX

import zarafa

def parser_opt_args():
    parser = zarafa.parser('skpc')
    parser.add_option('--create-public-store', dest='public_store', action='store_true',  help='Create public store')
    parser.add_option('--sync', dest='sync', action='store_true', help='Synchronize users and groups with external source')
    parser.add_option('--list-users', dest='list_users', action='store_true', help='List users')
    parser.add_option('--list-companies', dest='list_companies', action='store_true', help='List companies')
    parser.add_option('--list-stores', dest='list_stores', action='store_true', help='List stores')
    parser.add_option('--user-details', dest='user_details', action='store', help='Show user details', metavar='NAME')
    parser.add_option('--user-count', dest='usercount', action='store_true', help='Output the system users counts')
    # DB Plugin
    parser.add_option('--create-group', dest='create_group', action='store', help='Create group, -e options optional', metavar='NAME')
    parser.add_option('--delete-group', dest='delete_group', action='store', help='Delete group', metavar='NAME')
    return (parser,) + parser.parse_args()

def main():
    parser, options, args = parser_opt_args()
    server = zarafa.Server(options)

    if options.sync:
        server.sync_users()

    elif options.public_store:
        if not server.public_store:
            server.create_store(public=True)
        else:
            print 'public store already exists'

    elif options.list_users:
        for company in server.companies():
            print 'User list for %s(%s):' % (company.name, len(list(company.users()))) # XXX SYSTEM user?
            print '\tUsername\tFullname\tHomeserver'
            print '\t'+'-' * 45
            for user in company.users():
                print '\t%s\t%s\t%s' % (user.name, user.fullname, user.home_server) # XXX formatting

    elif options.list_companies:
        for company in server.companies():
            print company.name

    elif options.list_stores:
        for company in server.companies():
            print company.name
            for user in company.users(): # XXX company.stores
                print user.store.guid, user.name if user else ''
            print company.public_store.guid + ' public'

    elif options.user_details:
        user = server.user(options.user_details)
        print 'Username:\t' + user.name
        print 'Fullname:\t' + user.fullname
        print 'Emailaddress:\t' + user.email
        print 'Active:\t\t' + ('yes' if user.active else 'no')
        print 'Features:\t' + '; '.join(user.features)

    elif options.usercount:
        stats = server.table(PR_EC_STATSTABLE_SYSTEM).dict_(PR_DISPLAY_NAME, PR_EC_STATS_SYSTEM_VALUE)
        print 'Active:\t\t', int(stats['usercnt_active'])
        print 'Non-active:\t', int(stats['usercnt_nonactive'])
        print '  Users:\t', int(stats['usercnt_na_user'])
        print '  Rooms:\t', int(stats['usercnt_room'])
        print '  Equipment:\t', int(stats['usercnt_equipment'])

    elif options.create_group:
        server.create_group(options.create_group)

    elif options.delete_group:
        server.remove_group(options.delete_group)

    else:
        parser.print_help()

if __name__ == '__main__':
    main()
