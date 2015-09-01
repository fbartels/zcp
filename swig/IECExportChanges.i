%{
#include "IECExportChanges.h"
%}

class IECExportChanges : public IExchangeExportChanges{
public:
	virtual HRESULT GetChangeCount(ULONG *lpcChanges) = 0;
	virtual HRESULT SetMessageInterface(IID refiid) = 0;
	virtual HRESULT ConfigSelective(ULONG ulPropTag, LPENTRYLIST lpEntries, LPENTRYLIST lpParents, ULONG ulFlags, IUnknown *lpUnk, LPSPropTagArray lpIncludeProps, LPSPropTagArray lpExcludeProps, ULONG ulBufferSize);
#ifndef WIN32
	virtual HRESULT SetLogger(ECLogger *lpLogger) = 0;
#endif
	%extend {
		~IECExportChanges() { self->Release(); };
	}
};
