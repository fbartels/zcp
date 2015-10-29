dist_sbin_SCRIPTS = zarafa-search zarafa-search-xapian-compact.py

EXTRA_DIST = setup.py		\
	zarafa_search/__init__.py \
	zarafa_search/plaintext.py \
	zarafa_search/magic.py \
	zarafa_search/plugin_solr.py  \
	zarafa_search/plugin_xapian.py  \
	zarafa_search/xmltotext.xslt

install-exec-local:
	cd ${top_srcdir}/ECtools/search/ && ${PYTHON} setup.py install --root="${DESTDIR}" --prefix="${prefix}" --install-purelib ${pythondir}
