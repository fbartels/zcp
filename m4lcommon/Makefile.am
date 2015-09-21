# -*- Makefile -*-

SUBDIRS = include/zarafa

AM_CPPFLAGS = ${ZCPPFLAGS} $(DEBUGFLAGS) $(BOOST_CPPFLAGS) \
	-DMAPICONFIGDIR=\"${MAPICONFIGDIR}\" \
	-DPKGLIBDIR=\"${pkglibdir}\" \
	-I$(top_srcdir)/mapi4linux/include -I$(top_srcdir)/provider/common \
	-I${top_srcdir}/common -I${top_srcdir}/m4lcommon \
	-I${top_srcdir}/libfreebusy

noinst_LTLIBRARIES = libm4lcommon.la

m4lincludedir = ${includedir}/mapi4linux
pkgincludedir = ${includedir}/zarafa
mapiptrdir = ${pkgincludedir}/mapi_ptr

m4linclude_HEADERS = include/edkguid.h
pkginclude_HEADERS = \
	CommonUtil.h \
	ECDefs.h \
	ECFeatureList.h \
	ECInterfaceDefs.h \
	ECInvariantChecker.h \
	ECIterators.h \
	ECMAPIDebug.h \
	ECMemTable.h \
	EMSAbTag.h \
	HrException.h \
	IECChangeAdviseSink.h \
	IECChangeAdvisor.h \
	IECExchangeModifyTable.h \
	IECExportAddressbookChanges.h \
	IECExportChanges.h \
	IECImportAddressbookChanges.h \
	IECImportContentsChanges.h \
	IECImportHierarchyChanges.h \
	IECMultiStoreTable.h \
	IECSpooler.h \
	IECStatsCollector.h \
	IECTestProtocol.h \
	IMAPIOffline.h \
	MAPIConsoleTable.h \
	MAPINotifSink.h \
	Util.h \
	auto_free.h \
	favoritesutil.h \
	mapi_ptr.h \
	mapicontact.h \
	mapiext.h \
	namedprops.h \
	restrictionutil.h
mapiptr_HEADERS = \
	mapi_ptr/mapi_array_ptr.h \
	mapi_ptr/mapi_memory_ptr.h \
	mapi_ptr/mapi_object_ptr.h \
	mapi_ptr/mapi_rowset_ptr.h
# These were split off libcommon_mapi
libm4lcommon_la_SOURCES = \
	CommonUtil.cpp ECIterators.cpp \
	ECMAPIDebug.cpp ECMemTable.cpp \
	MAPIConsoleTable.cpp \
	MAPINotifSink.cpp Util.cpp \
	favoritesutil.cpp
libm4lcommon_la_LIBADD = \
	../common/libzcp_common_util.la ../common/libzcp_common_mapi.la \
	-lpthread


pkginclude_HEADERS += \
	ECFeatures.h ECRestriction.h \
	RecurrenceState.h userutil.h
# These were split off libcommon_util
libm4lcommon_la_SOURCES += \
	ECFeatures.cpp ECRestriction.cpp \
	RecurrenceState.cpp userutil.cpp

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2

include ../global.am
