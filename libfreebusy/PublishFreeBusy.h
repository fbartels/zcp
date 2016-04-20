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

#ifndef _PublishFreeBusy_H_
#define _PublishFreeBusy_H_

#include <mapix.h>
#include <mapidefs.h>
#include <ctime>

#include "freebusy.h"
#include <zarafa/CommonUtil.h>

typedef struct{
	ULONG ulType;
	time_t tsTime;
	ULONG ulStatus;
}TSARRAY;

HRESULT HrPublishDefaultCalendar(IMAPISession *lpSession, IMsgStore *lpDefStore, time_t tsStart, ULONG ulMonths, ECLogger *lpLogger);

class PublishFreeBusy
{
public:
	PublishFreeBusy(IMAPISession *lpSession, IMsgStore *lpDefStore, time_t tsStart, ULONG ulMonths, ECLogger *lpLogger);
	~PublishFreeBusy();
	
	HRESULT HrInit();
	HRESULT HrGetResctItems(IMAPITable **lppTable);
	HRESULT HrProcessTable(IMAPITable *lpTable , FBBlock_1 **lppfbBlocks, ULONG *lpcValues);
	HRESULT HrMergeBlocks(FBBlock_1 **lppfbBlocks,ULONG *cValues);
	HRESULT HrPublishFBblocks(FBBlock_1 *lpfbBlocks ,ULONG cValues);
	
private:

	IMAPISession *m_lpSession;
	IMsgStore *m_lpDefStore;
	ECLogger *m_lpLogger;
	FILETIME m_ftStart;
	FILETIME m_ftEnd;
	time_t m_tsStart;
	time_t m_tsEnd;

	PROPMAP_START
	PROPMAP_DEF_NAMED_ID (APPT_STARTWHOLE)
	PROPMAP_DEF_NAMED_ID (APPT_ENDWHOLE)
	PROPMAP_DEF_NAMED_ID (APPT_CLIPEND)
	PROPMAP_DEF_NAMED_ID (APPT_ISRECURRING)
	PROPMAP_DEF_NAMED_ID (APPT_FBSTATUS)
	PROPMAP_DEF_NAMED_ID (APPT_RECURRINGSTATE)
	PROPMAP_DEF_NAMED_ID (APPT_TIMEZONESTRUCT)

};

#endif //_PublishFreeBusy_H_
