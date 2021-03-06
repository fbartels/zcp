#!/bin/bash
# -*- Mode: sh -*-
# bash completion for Zarafa admin command

shopt -q extglob
_zarafa_old_extglob=$?
shopt -s extglob

_zarafa__hosts()
{
	local host_completes
	host_completes='file:///var/run/zarafad/server.sock http://localhost:236/zarafa https://localhost:237/zarafa'
	COMPREPLY=( $( compgen -W "$host_completes" -- ${COMP_WORDS[COMP_CWORD]} ) )
	return 0
}

_zarafa__userlist()
{
	# what todo with maybe passed --host ?
	# what todo with maybe passed -I company, --type ... ?

	local users
	users=$(zarafa-admin -l | grep -v ^User | grep -v username | grep -v SYSTEM | grep -v -- -- | awk {'print $1'})
	COMPREPLY=( $( compgen -W "$users" -- ${COMP_WORDS[COMP_CWORD]} ) )
	return 0
}

_zarafa__grouplist()
{
	# what todo with maybe passed --host ?
	# what todo with maybe passed -I company, --type ... ?

	local groups
	groups=$(zarafa-admin -L | grep -v ^Group | grep -v groupname | grep -v Everyone | grep -v -- -- | awk {'print $1'})
	COMPREPLY=( $( compgen -W "$groups" -- ${COMP_WORDS[COMP_CWORD]} ) )
	return 0
}

_zarafa__companylist()
{
	# what todo with maybe passed --host ?

	local users
	users=$(zarafa-admin --list-companies | tail -n +4 | awk {'print $1'})
	COMPREPLY=( $( compgen -W "$users" -- ${COMP_WORDS[COMP_CWORD]} ) )
	return 0
}

_zarafa_stats_options()
{
	list='--system --session --users --company --top --user -u --host -h'
	COMPREPLY=( $( compgen -W "$list" -- ${COMP_WORDS[COMP_CWORD]} ) )
}
complete -F _zarafa_stats_options zarafa-stats

_zarafa_admin_options()
{
	local cur prev short_actions long_actions possiblelist

	short_actions='-V -l -L -s -c -u -d -U -P -p -f -e -a -h -g -G -b -B -i -I -n'
	long_actions='--create-store --remove-store --hook-store --unhook-store --copyto-public --details --type --help --host --sync --qh --qs --qw --qo --udqh --udqs --udqw --udqo --lang --mr-accept --mr-decline-conflict --mr-decline-recurring --add-sendas --del-sendas --list-sendas --list-orphans --create-company --update-company --delete-company --list-companies --add-to-viewlist --del-from-viewlist --list-view --add-to-adminlist --del-from-adminlist --list-admin --set-system-admin --add-userquota-recipient --del-userquota-recipient --list-userquota-recipient --add-companyquota-recipient --del-companyquota-recipient --list-companyquota-recipient --purge-softdelete --purge-deferred --config --enable-feature --disable-feature --clear-cache --user-count --force-resync --reset-folder-count --node'

	COMPREPLY=()
	cur=${COMP_WORDS[COMP_CWORD]}
	prev=${COMP_WORDS[COMP_CWORD-1]}

	case "$prev" in
		-@(h|-host))
		# host completion is quite senseless, since you need admin rights, and no ssl cert can be used in zarafa-admin
		_zarafa__hosts
		return 0
		;;

		-@(-qo|-qdqo|-mr-*|a|n))
		possiblelist="yes no"
		;;

		--config)
		_filedir
		return 0
		;;

		-@(-lang|-create-company|c|U|p|f|e|g|-qh|-qs|-qw|-udqh|-udqs|-udqw|-list-admin|-list-userquota-recipient|-list-companyquota-recipient|-purge-softdelete))
		# opt req, no hints
		return 0
		;;

		-@(G|i))
		_zarafa__grouplist
		return 0
		;;

		-@(I|-list-view|-add-to-viewlist|-del-from-viewlist))
		_zarafa__companylist
		return 0
		;;

		-@(-details|-unhook-store|-create-store|u|d|b|B|-list-sendas|-add-sendas|-del-sendas|-add-to-adminlist|-del-from-adminlist|-set-system-admin|-add-userquota-recipient|-del-userquota-recipient||-add-companyquota-recipient|-del-companyquota-recipient|-force-resync|-reset-folder-count))
		_zarafa__userlist
		return 0
		;;

		--type)
		possiblelist="user group company"
		;;
		
		-@(-en|-dis)able-feature)
		possiblelist="imap pop3"
		;;
	esac

	if [ -z "$possiblelist" ]; then
		possiblelist="$short_actions $long_actions"
	fi

	COMPREPLY=( $( compgen -W "$possiblelist" -- "$cur" ) )
	return 0
}
complete -F _zarafa_admin_options $filenames zarafa-admin

_zarafa_backup_options()
{
	local cur short_actions long_actions possiblelist

	short_actions='-a -p -P -u -h -o -c -s -i -J -N -v'
	long_actions='--all --user --public --company --company-public --store --skip-junk --skip-public --output --host --config --verbose --help'

	COMPREPLY=()
	cur=${COMP_WORDS[COMP_CWORD]}
	prev=${COMP_WORDS[COMP_CWORD-1]}

	case "$prev" in
		-@(h|-host))
		_zarafa__hosts
		return 0
		;;

		# actually the (.*).zbk.index part ?
		-@(u|-user|f|-from))
		_zarafa__userlist
		;;

		-@(s|-company|P|-company-public))
		_zarafa__companylist
		;;

		-@(i|-restorefile|c|-config))
		_filedir
		return 0
		;;

		-@(o|-output))
		_filedir -d
		return 0
		;;

		-@(i|-store))
		# opt req, no hints
		return 0
		;;

		# empty parameters, ok with help and read??
		--@(all|public|skip-junk|skip-public|verbose|help))
		;;
		-@(a|p|J|N|v))
		;;
	esac

	if [ -z "$possiblelist" ]; then
		possiblelist="$short_actions $long_actions"
	fi
	
	COMPREPLY=( $( compgen -W "$possiblelist" -- "$cur" ) )
	return 0
}
complete -F _zarafa_backup_options $filenames zarafa-backup

_zarafa_restore_options()
{
	local cur short_actions long_actions possiblelist

	short_actions='-u -f -h -c -i -r -p -a -b -v'
	long_actions='--user --public --from --restorefile --recurse --not-before --not-after --host --config --verbose --help'

	COMPREPLY=()
	cur=${COMP_WORDS[COMP_CWORD]}
	prev=${COMP_WORDS[COMP_CWORD-1]}

	case "$prev" in
		-@(h|-host))
		_zarafa__hosts
		return 0
		;;

		# actually the (.*).zbk.index part ?
		-@(u|-user|f|-from))
		_zarafa__userlist
		;;

		-@(i|-restorefile|c|-config|a))
		_filedir
		return 0
		;;

		-@(-not-before|b))
		# opt req, no hints
		return 0
		;;

		# empty parameters, ok with help and read??
		--@(help|public|recurse|verbose))
		;;
		-@(p|r|v))
		;;
	esac

	if [ -z "$POSSIBLELIST" ]; then
		possiblelist="$short_actions $long_actions"
	fi
	
	COMPREPLY=( $( compgen -W "$possiblelist" -- "$cur" ) )
	
	return 0
}
complete -F _zarafa_restore_options $filenames zarafa-restore

_zarafa_dagent_options()
{
	local cur short_actions long_actions possiblelist

	short_actions='-c -j -f -d -h -a -F -P -p -q -s -v -e -n -C -V -r -R -l -N'
	long_actions='--help --config --junk --file --host --daemonize --listen --folder --public --create --read --do-not-notify --add-imap-data'

	COMPREPLY=()
	cur=${COMP_WORDS[COMP_CWORD]}
	prev=${COMP_WORDS[COMP_CWORD-1]}

	case "$prev" in
		-@(h|-host))
		_zarafa__hosts
		return 0
		;;

		-@(f|-file|c|-config|a))
		_filedir
		return 0
		;;

		-@(P|-public|F|-folder|p))
		# opt req, no hints
		return 0
		;;

		# empty parameters, ok with help and read??
		--@(help|junk|daemonize|listen|read|do-not-notify))
		;;
		-@(j|d|q|s|v|e|n|r|l|R|N))
		;;
	esac

	if [ -z "$possiblelist" ]; then
		possiblelist="$short_actions $long_actions"
	fi
	
	COMPREPLY=( $( compgen -W "$possiblelist" -- "$cur" ) )
	
	return 0
}
complete -F _zarafa_dagent_options $filenames zarafa-dagent

_zarafa_fsck_options()
{
	local cur short_actions long_actions possiblelist

	short_actions='-u -p -P -h -a'
	long_actions='--help --host --pass --user --calendar --contact --task --all --autofix --autodel --checkonly'

	COMPREPLY=()
	cur=${COMP_WORDS[COMP_CWORD]}
	prev=${COMP_WORDS[COMP_CWORD-1]}

	case "$prev" in
		-@(h|-host))
		_zarafa__hosts
		return 0
		;;

		-@(u|-user))
		_zarafa__userlist
		return 0
		;;

		-@(p|-pass))
		# opt req, no hints
		return 0
		;;

		# empty parameters
		--@(calendar|contact|task|all|autofix|autodel|checkonly))
		;;
		-@(P|a))
		;;
	esac

	if [ -z "$possiblelist" ]; then
		possiblelist="$short_actions $long_actions"
	fi
	
	COMPREPLY=( $( compgen -W "$possiblelist" -- "$cur" ) )
	
	return 0
}
complete -F _zarafa_fsck_options zarafa-fsck

_zarafa_passwd_options()
{
	local cur short_actions long_actions possiblelist

	short_actions='-u -p -h -o -V'
	long_actions='--help --host'

	COMPREPLY=()
	cur=${COMP_WORDS[COMP_CWORD]}
	prev=${COMP_WORDS[COMP_CWORD-1]}

	case "$prev" in
		-@(h|-host))
		_zarafa__hosts
		return 0
		;;

		-@(u|a))
		_zarafa__userlist
		return 0
		;;

		-@(p|o|V))
		# empty parameters
		;;
	esac

	if [ -z "$possiblelist" ]; then
		possiblelist="$short_actions $long_actions"
	fi
	
	COMPREPLY=( $( compgen -W "$possiblelist" -- "$cur" ) )
	
	return 0
}
complete -F _zarafa_passwd_options zarafa-passwd

_zarafa_archiver_options()
{
	local cur short_actions long_actions possiblelist

	short_actions='-u -l -A -w -c'
	long_actions='--list --archive --local-only --attach-to --detach-from --archive-folder --archive-server --no-folder --write --config --help'

	COMPREPLY=()
	cur=${COMP_WORDS[COMP_CWORD]}
	prev=${COMP_WORDS[COMP_CWORD-1]}

	case "$prev" in
		-@(u|-attach-to|-detach-from))
		_zarafa__userlist
		return 0
		;;

		--archive-server)
		_zarafa__hosts
		return 0
		;;

		-@(c|-config))
		_filedir
		return 0
		;;

		-@(l|w|-list|-local-only|-archive-folder|-no-folder|-writable|-help))
		# empty parameters
		;;
	esac

	if [ -z "$possiblelist" ]; then
		possiblelist="$short_actions $long_actions"
	fi
	
	COMPREPLY=( $( compgen -W "$possiblelist" -- "$cur" ) )
	
	return 0
}
complete -F _zarafa_archiver_options zarafa-archiver

if test "$_zarafa_old_extglob" != 0; then
	shopt -u extglob
fi
