%module(directors="1") MAPICore

%{
#undef LOCK_WRITE
#include <zarafa/platform.h>
#include <mapi.h>
#include <mapidefs.h>
#include <mapicode.h>
#include <mapiutil.h>
#include <zarafa/IECServiceAdmin.h>
#include "IECSpooler.h"
#include "IECTestProtocol.h"
#include "IECMultiStoreTable.h"
#include "IECExportChanges.h"
#include <zarafa/IECLicense.h>
#include <zarafa/mapi_ptr.h>

// DIRTIEST HACK IN THE WORLD WARNING: we need to fix the broken swig output for mapi_wrap.h .....
#pragma include_alias( "mapi_wrap.h", "mapi_wrap_fixed.h" )

/*
 * perl: CORE/thread.h can define PTHREAD_CREATE_JOINABLE to a value. This clashes with
 * the windows pthread implementation, where it's used in an enum.
 */
#ifdef PTHREAD_CREATE_JOINABLE
	#undef PTHREAD_CREATE_JOINABLE
#endif

#include "MAPINotifSink.h"
#include <zarafa/director_util.h>

HRESULT MAPIInitialize_Multithreaded() {
	MAPIINIT_0 init = {0, MAPI_MULTITHREAD_NOTIFICATIONS};

	return MAPIInitialize(&init);
}

%}

%include <zarafa/typemap.i>

HRESULT MAPIInitialize_Multithreaded();

#if SWIGPYTHON
%exception {
    try {
		mark_call_from_python();
		$action
		unmark_call_from_python();
    } catch (const Swig::DirectorException &) {
		unmark_call_from_python();
		SWIG_fail;
	}
}
#endif

class IUnknown {
public:
virtual HRESULT QueryInterface(const IID& USE_IID_FOR_OUTPUT, void **OUTPUT_USE_IID) = 0;
};

%extend IUnknown {
~IUnknown() { self->Release(); }
};

#define STGM_DIRECT             0x00000000L
#define STGM_TRANSACTED         0x00010000L
#define STGM_SIMPLE             0x08000000L

#define STGM_READ               0x00000000L
#define STGM_WRITE              0x00000001L
#define STGM_READWRITE          0x00000002L

#define STGM_SHARE_DENY_NONE    0x00000040L
#define STGM_SHARE_DENY_READ    0x00000030L
#define STGM_SHARE_DENY_WRITE   0x00000020L
#define STGM_SHARE_EXCLUSIVE    0x00000010L

#define STGM_PRIORITY           0x00040000L
#define STGM_DELETEONRELEASE    0x04000000L
#define STGM_NOSCRATCH          0x00100000L

#define STGM_CREATE             0x00001000L
#define STGM_CONVERT            0x00020000L
#define STGM_FAILIFTHERE        0x00000000L

#define STGM_NOSNAPSHOT         0x00200000L
#define STGM_DIRECT_SWMR        0x00400000L

typedef enum tagSTGTY {
    STGTY_STORAGE       = 1,
    STGTY_STREAM        = 2,
    STGTY_LOCKBYTES     = 3,
    STGTY_PROPERTY      = 4
} STGTY;

typedef enum tagSTREAM_SEEK {
    STREAM_SEEK_SET     = 0,
    STREAM_SEEK_CUR     = 1,
    STREAM_SEEK_END     = 2
} STREAM_SEEK;

typedef enum tagLOCKTYPE {
    LOCK_WRITE          = 1,
    LOCK_EXCLUSIVE      = 2,
    LOCK_ONLYONCE       = 4
} LOCKTYPE;

typedef enum tagSTATFLAG {
    STATFLAG_DEFAULT    = 0,
    STATFLAG_NONAME     = 1,
    STATFLAG_NOOPEN     = 2
} STATFLAG;

/* IStream Interface */
class ISequentialStream : public IUnknown {
public:
	// Hard to typemap so using other method below
    // virtual HRESULT Read(void *OUTPUT, ULONG cb, ULONG *OUTPUT) = 0;
    virtual HRESULT Write(const void *pv, ULONG cb, ULONG *OUTPUT) = 0;
	%extend {
		HRESULT Read(ULONG cb, char **lpOutput, ULONG *ulRead) {
			HRESULT hr = hrSuccess;
			char *buffer;
			StreamPtr ptrStream;

			if (self->QueryInterface(ptrStream.iid, &ptrStream) == hrSuccess) {
				const LARGE_INTEGER liMove = {0, 0};
				ULARGE_INTEGER liPosition;
				STATSTG statbuf;

				hr = ptrStream->Seek(liMove, SEEK_CUR, &liPosition);
				if (hr != hrSuccess)
					goto exit;

				hr = ptrStream->Stat(&statbuf, 0);
				if (hr != hrSuccess)
					goto exit;

				if ((statbuf.cbSize.QuadPart - liPosition.QuadPart) < cb)
					cb = (ULONG)(statbuf.cbSize.QuadPart - liPosition.QuadPart);
			}

			hr = MAPIAllocateBuffer(cb, (void **)&buffer);
			if (hr != hrSuccess)
				goto exit;

			self->Read(buffer, cb, ulRead);

			*lpOutput = buffer;
		exit:
			return hr;
		}
    }
};

%feature("notabstract") IStream;

class IStream : public ISequentialStream {
public:
    virtual HRESULT Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) = 0;
    virtual HRESULT SetSize(ULARGE_INTEGER libNewSize) = 0;
    virtual HRESULT CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) = 0;
    virtual HRESULT Commit(DWORD grfCommitFlags) = 0;
    virtual HRESULT Revert() = 0;
    virtual HRESULT LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) = 0;
    virtual HRESULT UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) = 0;
    virtual HRESULT Stat(STATSTG *OUTPUT, DWORD grfStatFlag) = 0;
    virtual HRESULT Clone(IStream **ppstm) = 0;

	%extend {
		IStream() {
			IStream *lpStream = NULL;
			HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &lpStream);
			if(hr == hrSuccess)
				return lpStream;
			return NULL;
		}
		~IStream() {
			self->Release();
		}
	}
};

%include "mapidefs.i"
%include "mapix.i"
%include "mapicode.i"
%include "mapinotifsink.i"
%include "mapiutil.i"
%include "edkmdb.i"
%include "IECServiceAdmin.i"
%include "IECSpooler.i"
%include "IECTestProtocol.i"
%include "IECMultiStoreTable.i"
%include "IECLicense.i"
%include "IECExportChanges.i"
%include "helpers.i"
%include "ecdefs.i"


#define MAPI_ORIG	0x00000000
#define MAPI_TO		0x00000001
#define MAPI_CC		0x00000002
#define MAPI_BCC	0x00000003
#define MAPI_P1		0x10000000

#define MAPI_SUBMITTED	0x80000000

#define MAPI_UNREAD             0x00000001
#define MAPI_RECEIPT_REQUESTED  0x00000002
#define MAPI_SENT               0x00000004

#define MAPI_LOGON_UI           0x00000001
#define MAPI_PASSWORD_UI        0x00020000

#define MAPI_NEW_SESSION        0x00000002
#define MAPI_FORCE_DOWNLOAD     0x00001000
#define MAPI_EXTENDED           0x00000020

#define MAPI_DIALOG             0x00000008
#define MAPI_USE_DEFAULT		0x00000040

#define MAPI_UNREAD_ONLY        0x00000020
#define MAPI_GUARANTEE_FIFO     0x00000100
#define MAPI_LONG_MSGID         0x00004000

#define MAPI_PEEK               0x00000080
#define MAPI_SUPPRESS_ATTACH    0x00000800
#define MAPI_ENVELOPE_ONLY      0x00000040
#define MAPI_BODY_AS_FILE       0x00000200
#define MAPI_AB_NOMODIFY        0x00000400


%{
#include <zarafa/ECGuid.h>
#include <edkguid.h>

swig_type_info *TypeFromIID(REFIID iid)
{
#define TYPECASE(x) if(iid == IID_##x) return SWIGTYPE_p_##x;
  TYPECASE(IUnknown)
  TYPECASE(IStream)
  TYPECASE(IMAPIProp)
  TYPECASE(IMessage)
  TYPECASE(IMAPIContainer)
  TYPECASE(IMAPIFolder)
  TYPECASE(IMAPITable)
  TYPECASE(IABContainer)
  TYPECASE(IMailUser)
  TYPECASE(IDistList)
  TYPECASE(IMsgStore)
  if (iid == IID_ECMsgStoreOnline || iid == IID_ECMsgStoreOffline) return SWIGTYPE_p_IMsgStore;
  TYPECASE(IExchangeExportChanges)
  TYPECASE(IECExportChanges)
  TYPECASE(IExchangeImportContentsChanges)
  TYPECASE(IExchangeImportHierarchyChanges)
  TYPECASE(IExchangeManageStore)
  TYPECASE(IExchangeModifyTable)
  TYPECASE(IECServiceAdmin)
  TYPECASE(IECTestProtocol)
  TYPECASE(IECMultiStoreTable)
  TYPECASE(IECSpooler)
  TYPECASE(IECChangeAdvisor)
  TYPECASE(IECChangeAdviseSink)
  TYPECASE(IECSingleInstance)
  TYPECASE(IECLicense)
  TYPECASE(IProxyStoreObject)
  TYPECASE(IECImportContentsChanges)
  TYPECASE(IECImportHierarchyChanges)
  return NULL;
}

LPCIID IIDFromType(const char *type)
{
#define IIDCASE(x) if(strstr(type, #x) != NULL) return &IID_##x;
  IIDCASE(IUnknown)
  IIDCASE(IStream)
  IIDCASE(IMAPIProp)
  IIDCASE(IMessage)
  IIDCASE(IMAPIContainer)
  IIDCASE(IMAPIFolder)
  IIDCASE(IMAPITable)
  IIDCASE(IABContainer)
  IIDCASE(IMailUser)
  IIDCASE(IDistList)
  IIDCASE(IMsgStore)
  IIDCASE(IExchangeExportChanges)
  IIDCASE(IECExportChanges)
  IIDCASE(IExchangeImportContentsChanges)
  IIDCASE(IExchangeImportHierarchyChanges)
  IIDCASE(IExchangeManageStore)
  IIDCASE(IExchangeModifyTable)
  IIDCASE(IECServiceAdmin)
  IIDCASE(IECTestProtocol)
  IIDCASE(IECMultiStoreTable)
  IIDCASE(IECChangeAdvisor)
  IIDCASE(IECChangeAdviseSink)
  IIDCASE(IECSingleInstance)
  IIDCASE(IECLicense)
  IIDCASE(IProxyStoreObject)
  IIDCASE(IECImportContentsChanges)
  IIDCASE(IECImportHierarchyChanges)
  return &IID_IUnknown;
}
%}

#if SWIGPYTHON
%include "ECLogger.i"

// Directors for IStream

%{
#include <zarafa/swig_iunknown.h>

typedef IUnknownImplementor<IStream> Stream;
%}


%feature("director") Stream;
%feature("nodirector") Stream::QueryInterface;
class Stream : public IStream {
public:
	Stream(ULONG cInterfaces, LPCIID lpInterfaces);
    virtual HRESULT Read(void *OUTPUT, ULONG cb, ULONG *cbOUTPUT) = 0;
    virtual HRESULT Write(const void *pv, ULONG cb, ULONG *OUTPUT) = 0;

	%extend {
		virtual ~Stream() { self->Release(); };
	}
};


#ifndef WIN32
%include "zarafasync.i"
#endif

#endif
