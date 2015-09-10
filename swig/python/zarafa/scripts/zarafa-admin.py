#!/usr/bin/env python

# playing around with a python-zarafa based version of zarafa-admin (far from complete, patches/ideas welcome!)

# usage: ./zarafa-admin.py -h

import zarafa

def opt_args():
    parser = zarafa.parser('skpc')
    parser.add_option('--create-public-store', dest='public_store', action='store_true',  help='Create public store') # XXX: conflicts with socket
    parser.add_option('--sync', dest='sync', action='store_true', help='Synchronize users and groups with external source.')
    parser.add_option('-l', dest='list_users', action='store_true', help='List users. Use -I to list users of a specific company, if applicable.')
    parser.add_option('-L', dest='list_companies', action='store_true', help='List users. Use -I to list users of a specific company, if applicable.')
    parser.add_option('--details', dest='details', action='store', help='Show object details, use --type to indicate the object type.')
    # DB Plugin
    parser.add_option('-g', dest='create_group', action='store', help='Create group, -e options optional.')
    parser.add_option('-G', dest='delete_group', action='store', help='Delete group.')
    return parser.parse_args()

def main():
    options, args = opt_args()
    server = zarafa.Server(options)

    if options.sync:
        server.sync_users()
    elif options.public_store:
        server.create_store(public=True) # Handle already existing public store
    elif options.list_users:
        for company in server.companies():
            print 'User list for %s(%s):' % (company.name, len(list(company.users()))) # Missing SYSTEM in list
            print '\tUsername\tFullname\tHomeserver'
            print '\t'+'-' * 45
            for user in company.users():
                print '\t%s\t%s\t%s' % (user.name, user.fullname, user.home_server) # FIXME formatting
    elif options.details:
        user = server.user(options.details)
    elif options.create_group:
        server.create_group(options.create_group)
    elif options.delete_group:
        server.remove_group(options.delete_group)


if __name__ == '__main__':
    main()
