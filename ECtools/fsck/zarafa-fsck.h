/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ZARAFA_FSCK
#define ZARAFA_FSCK

#include <zarafa/platform.h>
#include <string>
#include <list>
#include <set>
using namespace std;

#include <mapidefs.h>

/*
 * Global configuration
 */
extern string auto_fix;
extern string auto_del;

class ZarafaFsck {
private:
	ULONG ulFolders;
	ULONG ulEntries;
	ULONG ulProblems;
	ULONG ulFixed;
	ULONG ulDeleted;

	virtual HRESULT ValidateItem(LPMESSAGE lpMessage, const std::string &strClass) = 0;

public:
	ZarafaFsck();
	virtual ~ZarafaFsck() { }

	HRESULT ValidateMessage(LPMESSAGE lpMessage, const std::string &strName, const std::string &strClass);
	HRESULT ValidateFolder(LPMAPIFOLDER lpFolder, const std::string &strName);

	HRESULT AddMissingProperty(LPMESSAGE lpMessage, const std::string &strName, ULONG ulTag, __UPV Value);
	HRESULT ReplaceProperty(LPMESSAGE lpMessage, const std::string &strName, ULONG ulTag, const std::string &strError, __UPV Value);

	HRESULT DeleteRecipientList(LPMESSAGE lpMessage, std::list<unsigned int> &mapiReciptDel, bool &bChanged);

	HRESULT DeleteMessage(LPMAPIFOLDER lpFolder,
			      LPSPropValue lpItemProperty);

	HRESULT ValidateRecursiveDuplicateRecipients(LPMESSAGE lpMessage, bool &bChanged);
	HRESULT ValidateDuplicateRecipients(LPMESSAGE lpMessage, bool &bChanged);

	void PrintStatistics(const std::string &title);
};

class ZarafaFsckCalendar : public ZarafaFsck {
private:
	HRESULT ValidateItem(LPMESSAGE lpMessage, const std::string &strClass);
	HRESULT ValidateMinimalNamedFields(LPMESSAGE lpMessage);
	HRESULT ValidateTimestamps(LPMESSAGE lpMessage);
	HRESULT ValidateRecurrence(LPMESSAGE lpMessage);
};

class ZarafaFsckContact : public ZarafaFsck {
private:
	HRESULT ValidateItem(LPMESSAGE lpMessage, const std::string &strClass);
	HRESULT ValidateContactNames(LPMESSAGE lpMessage);
};

class ZarafaFsckTask : public ZarafaFsck {
private:
	HRESULT ValidateItem(LPMESSAGE lpMessage, const std::string &strClass);
	HRESULT ValidateMinimalNamedFields(LPMESSAGE lpMessage);
	HRESULT ValidateTimestamps(LPMESSAGE lpMessage);
	HRESULT ValidateCompletion(LPMESSAGE lpMessage);
};

/*
 * Helper functions.
 */
HRESULT allocNamedIdList(ULONG ulSize, LPMAPINAMEID **lpppNameArray);
void freeNamedIdList(LPMAPINAMEID *lppNameArray);

HRESULT ReadProperties(LPMESSAGE lpMessage, ULONG ulCount,
		       ULONG *lpTag, LPSPropValue *lppPropertyArray);
HRESULT ReadNamedProperties(LPMESSAGE lpMessage, ULONG ulCount,
			    LPMAPINAMEID *lppTag,
			    LPSPropTagArray *lppPropertyTagArray,
			    LPSPropValue *lppPropertyArray);

#endif /* ZARAFA_FSCK */
