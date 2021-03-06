#!/usr/bin/env python

# generate graphs, containing e.g. the store size for each user

# usage: ./z-plot.py -h

import zarafa
import matplotlib.pyplot as plt

def opt_args():
    parser = zarafa.parser('skpc')
    parser.add_option('--store', dest='store', action='store_true',  help='Plots a graph with store sizes of users')
    parser.add_option('--folders', dest='plotfolders', action='store_true',  help='Plots a graph with the number of folders per user')
    parser.add_option('--items', dest='items', action='store_true',  help='Plots a graph with the number of items per user')
    parser.add_option('--save', dest='save', action='store',  help='Save plot to file (png)')
    parser.add_option('--sort', dest='sort', action='store_true',  help='Sort the graph')
    return parser.parse_args()

def b2m(bytes):
    return (bytes / 1024) / 1024

def main():
    options, args = opt_args()
    users = list(zarafa.Server(options).users())
    data = []

    fig, ax = plt.subplots() 

    if options.store:
        data = {user.name: b2m(user.store.size) for user in users}
        plt.ylabel('Store size (Mb)')
    elif options.plotfolders:
        # TODO: add mail only flag?
        data = {user.name: len(list(user.store.folders())) for user in users}
        plt.ylabel('Folders')
    elif options.items:
        data ={user.name: sum(folder.count for folder in user.store.folders()) for user in users}
        plt.ylabel('Items')
    else:
        return

    if options.sort:
        ax.plot(sorted(data.values()))
        users = sorted(data, key=data.__getitem__)

    else:
        ax.plot(data.values())
    plt.xlabel('Users')

    # TODO: find a more elegant solution
    labels = [item.get_text() for item in ax.get_xticklabels()]
    for i, user in enumerate(users):
        if options.sort:
            labels[i] = user
        else:
            labels[i] = user.name
    ax.set_xticklabels(labels)

    if options.save:
        plt.savefig(options.save)
    else:
        plt.show()

if __name__ == '__main__':
    main()
