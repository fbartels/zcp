dist_data_DATA = 
datadir = ${docdir}

BUILT_SOURCES=zarafa.7
allpages=\
	zarafa-admin.8 \
	zarafa-archiver.8 zarafa-archiver.cfg.5 \
	zarafa-autorespond.8 \
	zarafa-dagent.8 zarafa-dagent.cfg.5 \
	zarafa-mr-accept.8 zarafa-mr-process.8 \
	zarafa-fsck.1 \
	zarafa-gateway.8 zarafa-gateway.cfg.5 \
	zarafa-ical.8 zarafa-ical.cfg.5 \
	zarafa-ldap.cfg.5 \
	zarafa-mailbox-permissions.8 \
	zarafa-monitor.8 zarafa-monitor.cfg.5 \
	zarafa-passwd.1 \
	zarafa-search.8 zarafa-search.cfg.5 \
	zarafa-server.8 zarafa-server.cfg.5 \
	zarafa-set-oof.1 \
	zarafa-spooler.8 zarafa-spooler.cfg.5 \
	zarafa-stats.1 \
	zarafa-unix.cfg.5


CLEANFILES = zarafa-cfgchecker.1

man_MANS=$(BUILT_SOURCES) $(allpages)

# Since the manual.xml file is in UTF-8, we need to call 'xmlto' in UTF-8 mode as well.
$(BUILT_SOURCES): manual.xml
	@if [ -z $(PROG_XMLTO) ]; then \
		echo Please install xmlto to generate manual pages; \
	else \
		LC_ALL=en_US.UTF-8 $(PROG_XMLTO) man ${top_srcdir}/doc/manual.xml; \
	fi

# force allpages to wait on output from xmlto
$(allpages): $(BUILT_SOURCES)

EXTRA_DIST = $(BUILT_SOURCES) $(allpages) manual.xml
