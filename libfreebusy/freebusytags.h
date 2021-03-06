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


#ifndef FREEBUSYTAG_INCLUDED
#define FREEBUSYTAG_INCLUDED

#define PR_FREEBUSY_ALL_EVENTS			PROP_TAG(PT_MV_BINARY, 0x6850)
#define PR_FREEBUSY_ALL_MONTHS			PROP_TAG(PT_MV_LONG, 0x684F)
#define PR_FREEBUSY_BUSY_EVENTS			PROP_TAG(PT_MV_BINARY, 0x6854)
#define PR_FREEBUSY_BUSY_MONTHS			PROP_TAG(PT_MV_LONG, 0x6853)

#define PR_FREEBUSY_EMAIL_ADDRESS		PROP_TAG(PT_TSTRING, 0x6849)  // PR_FREEBUSY_EMA
#define PR_FREEBUSY_END_RANGE			PROP_TAG(PT_LONG, 0x6848)
#define PR_FREEBUSY_LAST_MODIFIED		PROP_TAG(PT_SYSTIME, 0x6868)
#define PR_FREEBUSY_NUM_MONTHS			PROP_TAG(PT_LONG, 0x6869)  
#define PR_FREEBUSY_OOF_EVENTS			PROP_TAG(PT_MV_BINARY, 0x6856)
#define PR_FREEBUSY_OOF_MONTHS			PROP_TAG(PT_MV_LONG, 0x6855)
#define PR_FREEBUSY_START_RANGE			PROP_TAG(PT_LONG, 0x6847)
#define PR_FREEBUSY_TENTATIVE_EVENTS	PROP_TAG(PT_MV_BINARY, 0x6852)
#define PR_FREEBUSY_TENTATIVE_MONTHS	PROP_TAG(PT_MV_LONG, 0x6851)

#define PR_PERSONAL_FREEBUSY			PROP_TAG(PT_BINARY, 0x686C) // PR_FREEBUSY_DATA
//#define PR_RECALCULATE_FREEBUSY			PROP_TAG(PT_BOOLEAN, 0x10F2) PR_DISABLE_FULL_FIDELITY

//localfreebusy properties
/*
 * PR_PROCESS_MEETING_REQUESTS, PR_DECLINE_CONFLICTING_MEETING_REQUESTS, and
 * PR_DECLINE_RECURRING_MEETING_REQUESTS are already defined by
 * Zarafa's <mapiext.h>
 */

#endif // FREEBUSYTAG_INCLUDED
