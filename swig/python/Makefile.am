# -*- Makefile -*-

SUBDIRS = include/zarafa zarafa

SWIG_CXXFLAGS = -Wall -python -c++ -threads \
	-I${top_srcdir}/swig/include -I${srcdir}/include

lib_LTLIBRARIES = libzcp_pyconv.la libzcp_pydirector.la
pyexec_LTLIBRARIES = _MAPICore.la _inetmapi.la _icalmapi.la _licenseclient.la
noinst_LTLIBRARIES = _libcommon.la _archiver.la _libfreebusy.la _RecurrenceState.la
noinst_DATA = libzcp_pyconv.ldd _MAPICore.ldd _inetmapi.ldd _icalmapi.ldd \
	_libcommon.ldd _archiver.ldd _libfreebusy.ldd _RecurrenceState.ldd \
	_licenseclient.ldd

# disable warnings since code is mostly generated
AM_CPPFLAGS = ${ZCPPFLAGS} $(DEBUGFLAGS) \
	-I$(top_srcdir)/mapi4linux/include -I$(top_srcdir)/provider/common \
	-I${top_srcdir}/common -I${top_srcdir}/m4lcommon \
	$(PYTHON_INCLUDES) -I${top_srcdir}/swig -w \
	-DSWIG_PYTHON_OUTPUT_TUPLE -DSWIG_TYPE_TABLE=MAPI

libzcp_pyconv_la_SOURCES = conversion.cpp conversion.h
libzcp_pyconv_la_LDFLAGS = -Wl,--version-script=${top_builddir}/default.sym
libzcp_pyconv_la_LIBADD  = ${PYTHON_LIBS} \
	${top_builddir}/mapi4linux/src/libmapi.la
libzcp_pydirector_la_SOURCES = ../director_util.cpp ../director_util.h
libzcp_pydirector_la_LDFLAGS = -Wl,--version-script=${top_builddir}/default.sym
libzcp_pydirector_la_LIBADD  = -lpthread

_MAPICore_la_CPPFLAGS = ${AM_CPPFLAGS} -I${top_srcdir}/zarafa-libsync
_MAPICore_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version
_MAPICore_la_LIBADD = libzcp_pyconv.la libzcp_pydirector.la \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libzcp_common_mapi.la \
	${top_builddir}/common/libzcp_common_util.la \
	${top_builddir}/zarafa-libsync/libzarafasync.la \
	$(XML2_LIBS) $(PROG_LIBS) $(PYTHON_LIBS)

_inetmapi_la_CPPFLAGS = ${AM_CPPFLAGS} -I${top_srcdir}/inetmapi
_inetmapi_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version
_inetmapi_la_LIBADD = libzcp_pyconv.la \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libzcp_common_mapi.la \
	${top_builddir}/common/libzcp_common_util.la \
	${top_builddir}/inetmapi/libinetmapi.la \
	$(PROG_LIBS) $(PYTHON_LIBS)

_icalmapi_la_CPPFLAGS = ${AM_CPPFLAGS} -I${top_srcdir}/libicalmapi -I${top_srcdir}/libfreebusy
_icalmapi_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version
_icalmapi_la_LIBADD = libzcp_pyconv.la \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/libicalmapi/libicalmapi.la \
	${top_builddir}/common/libzcp_common_mapi.la \
	${top_builddir}/common/libzcp_common_util.la \
	${top_builddir}/libfreebusy/libfreebusy.la \
	$(ICAL_LIBS) $(PROG_LIBS) $(PYTHON_LIBS)

_archiver_la_CPPFLAGS = ${AM_CPPFLAGS} -I${top_srcdir}/ECtools/archiver
_archiver_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version -rpath '$(libdir)'
_archiver_la_LIBADD = \
	${top_builddir}/ECtools/libzarafa-archiver.la \
	${top_builddir}/ECtools/libzarafa-archiver-core.la \
	$(PROG_LIBS) $(PYTHON_LIBS)

_licenseclient_la_CPPFLAGS = ${AM_CPPFLAGS} -I${top_srcdir}/provider/common -I${top_builddir}/provider/soap -I${top_srcdir}/provider/soap ${GSOAP_CFLAGS}
_licenseclient_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version
_licenseclient_la_LIBADD = ${top_builddir}/provider/common/libzarafacommon.la \
						${GSOAP_LIBS} \
						${top_builddir}/provider/soap/libzarafa_soapclient.la \
						${top_builddir}/common/libzcp_common_mapi.la \
						${top_builddir}/common/libzcp_common_util.la \
						${top_builddir}/common/libzcp_common_ssl.la \
						${top_builddir}/mapi4linux/src/libmapi.la \
						$(PROG_LIBS) $(SSL_LIBS) $(PYTHON_LIBS)


# noinst_ and rpath will generate the _libcommon.so file
_libcommon_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version -rpath '$(libdir)'
_libcommon_la_LIBADD = libzcp_pyconv.la ${top_builddir}/mapi4linux/src/libmapi.la \
					   ${top_builddir}/common/libzcp_common_mapi.la \
					   ${top_builddir}/common/libzcp_common_util.la \
				  	   $(PROG_LIBS) $(PYTHON_LIBS)

_RecurrenceState_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version -rpath '$(libdir)'
_RecurrenceState_la_LIBADD = libzcp_pyconv.la \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libzcp_common_mapi.la \
	${top_builddir}/common/libzcp_common_util.la \
	$(PROG_LIBS) $(PYTHON_LIBS)

_libfreebusy_la_CPPFLAGS = ${AM_CPPFLAGS} -I${top_srcdir}/libfreebusy
_libfreebusy_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version
_libfreebusy_la_LIBADD = libzcp_pyconv.la ${top_builddir}/mapi4linux/src/libmapi.la \
						 ${top_builddir}/libfreebusy/libfreebusy.la \
						 ${top_builddir}/common/libzcp_common_mapi.la \
                         ${top_builddir}/common/libzcp_common_util.la \
                         $(PROG_LIBS) $(PYTHON_LIBS)


BUILT_SOURCES = mapi_wrap.cxx \
				mapi_wrap.h \
				inetmapi_wrap.cxx \
				icalmapi_wrap.cxx \
				archiver_wrap.cxx \
				libcommon_wrap.cxx \
				licenseclient_wrap.cxx \
				libfreebusy_wrap.cxx \
				RecurrenceState_wrap.cxx


CLEANFILES = $(BUILT_SOURCES) \
			 MAPICore.py \
			 inetmapi.py \
			 icalmapi.py \
			 archiver.py \
			 libcommon.py \
			 licenseclient.py \
			 libfreebusy.py


# wait for full output of swig
MAPICore.py: $(BUILT_SOURCES)
inetmapi.py: $(BUILT_SOURCES)
icalmapi.py: $(BUILT_SOURCES)
archiver.py: $(BUILT_SOURCES)
libcommon.py: $(BUILT_SOURCES)
licenseclient.py: $(BUILT_SOURCES)
libfreebusy.py: $(BUILT_SOURCES)
RecurrenceState.py: $(BUILT_SOURCES)


_MAPICore_la_SOURCES = mapi_wrap.cxx mapi_wrap.h scl.h
_inetmapi_la_SOURCES = inetmapi_wrap.cxx
_icalmapi_la_SOURCES = icalmapi_wrap.cxx
_archiver_la_SOURCES = archiver_wrap.cxx archiver_conv.h archiver_conv.cpp
_libcommon_la_SOURCES = libcommon_wrap.cxx
_RecurrenceState_la_SOURCES = RecurrenceState_wrap.cxx
_licenseclient_la_SOURCES = licenseclient_wrap.cxx licenseclient_conv.cpp licenseclient_conv.h
_libfreebusy_la_SOURCES = libfreebusy_wrap.cxx libfreebusy_conv.h libfreebusy_conv.cpp


EXTRA_DIST = helpers_python.i setup.py $(BUILT_SOURCES) MAPICore.py inetmapi.py icalmapi.py licenseclient.py MAPI/Defs.py  MAPI/__init__.py  MAPI/Struct.py  MAPI/Tags.py  MAPI/Time.py MAPI/Util/__init__.py MAPI/Util/Generators.py MAPI/Util/Iterators.py MAPI/Util/AddressBook.py MAPI/Util/codepage.py archiver_python.i
datadir = ${pythondir}
data_DATA = MAPICore.py inetmapi.py icalmapi.py licenseclient.py
noinst_datadir = libcommon.py RecurrenceState.py


mapi_wrap.h: mapi_wrap.cxx
mapi_wrap.cxx: ../mapi.i ../mapiutil.i ../mapidefs.i ../mapicode.i ../mapix.i ../mapinotifsink.i ../mapiutil.i ../include/zarafa/typemap.i ../helpers.i ../edkmdb.i ../IECServiceAdmin.i ../ecdefs.i include/zarafa/typemap_python.i helpers_python.i
	if test "x$(SWIG_EXEC)" = "x"; then echo "swig is required for 'make dist'"; exit 1; fi
	${SWIG_EXEC} -I${top_srcdir}/swig -I${top_srcdir}/zarafa-libsync ${SWIG_FLAGS} ${SWIG_CXXFLAGS} -o $@ $<

inetmapi_wrap.cxx: ../inetmapi.i ../include/zarafa/typemap.i ../inetmapi.i include/zarafa/typemap_python.i helpers_python.i
	${SWIG_EXEC} -I${top_srcdir}/swig ${SWIG_CXXFLAGS} -o $@ $<

icalmapi_wrap.cxx: ../icalmapi.i ../include/zarafa/typemap.i include/zarafa/typemap_python.i helpers_python.i ../../libicalmapi/icalmapi.h
	${SWIG_EXEC} -I${top_srcdir}/libicalmapi -I${top_srcdir}/libfreebusy -I${top_srcdir/swig} ${SWIG_CXXFLAGS} -o $@ $<

archiver_wrap.cxx: ../archiver.i archiver_python.i
	${SWIG_EXEC} -I${top_srcdir}/ECtools/archiver ${SWIG_CXXFLAGS} -o $@ $<

libcommon_wrap.cxx: ../libcommon.i
	${SWIG_EXEC} -I${top_srcdir}/common -I${top_srcdir}/m4lcommon -I${top_srcdir/swig} ${SWIG_CXXFLAGS} -o $@ $<

RecurrenceState_wrap.cxx: ../RecurrenceState.i
	${SWIG_EXEC} -I${top_srcdir}/common -I${top_srcdir}/m4lcommon -I${top_srcdir/swig} ${SWIG_CXXFLAGS} -o $@ $<

licenseclient_wrap.cxx: ../licenseclient.i
	${SWIG_EXEC} -I${top_srcdir}/provider/common -I${top_srcdir}/common -I${top_srcdir}/m4lcommon -I${top_srcdir/swig} ${SWIG_CXXFLAGS} -o $@ $<

libfreebusy_wrap.cxx: ../libfreebusy.i
	${SWIG_EXEC} -I${top_srcdir}/libfreebusy -I${top_srcdir/swig} ${SWIG_CXXFLAGS} -o $@ $<

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2

install-exec-local:
	test "$(DISTRO)" = "suse" -o "$(DISTRO)" = "sles" && pyrpm="--record-rpm=INSTALLED_FILES"; \
	cd ${srcdir} && ${PYTHON} setup.py install -f \
		$${DESTDIR+--root=${DESTDIR}} \
		--prefix="${prefix}" --install-purelib ${pythondir} $${pyrpm}
	rm -Rf build

include ../../global.am
