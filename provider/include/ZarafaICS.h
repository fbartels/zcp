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

#ifndef ZARAFAICS_H
#define ZARAFAICS_H

// Flags for ns__getChanges and ns__setSyncStatus
#define ICS_SYNC_CONTENTS	1
#define ICS_SYNC_HIERARCHY	2
#define ICS_SYNC_AB			3

#define ICS_MESSAGE			0x1000
#define ICS_FOLDER			0x2000
#define ICS_AB				0x4000

#define ICS_ACTION_MASK		0x000F

#define ICS_NEW				0x0001
#define ICS_CHANGE			0x0002
#define ICS_FLAG			0x0003
#define	ICS_SOFT_DELETE		0x0004
#define ICS_HARD_DELETE		0x0005
#define ICS_MOVED			0x0006 //not used

#define ICS_CHANGE_FLAG_NEW 		(1 << (ICS_NEW))
#define ICS_CHANGE_FLAG_CHANGE		(1 << (ICS_CHANGE))
#define ICS_CHANGE_FLAG_FLAG		(1 << (ICS_FLAG))
#define ICS_CHANGE_FLAG_SOFT_DELETE	(1 << (ICS_SOFT_DELETE))
#define ICS_CHANGE_FLAG_HARD_DELETE (1 << (ICS_HARD_DELETE))
#define ICS_CHANGE_FLAG_MOVED		(1 << (ICS_MOVED))

#define ICS_MESSAGE_CHANGE		(ICS_MESSAGE | ICS_CHANGE)
#define ICS_MESSAGE_FLAG		(ICS_MESSAGE | ICS_FLAG)
#define ICS_MESSAGE_SOFT_DELETE	(ICS_MESSAGE | ICS_SOFT_DELETE)
#define ICS_MESSAGE_HARD_DELETE	(ICS_MESSAGE | ICS_HARD_DELETE)
#define ICS_MESSAGE_NEW			(ICS_MESSAGE | ICS_NEW)

#define ICS_FOLDER_CHANGE		(ICS_FOLDER | ICS_CHANGE)
#define ICS_FOLDER_SOFT_DELETE	(ICS_FOLDER | ICS_SOFT_DELETE)
#define ICS_FOLDER_HARD_DELETE	(ICS_FOLDER | ICS_HARD_DELETE)
#define ICS_FOLDER_NEW			(ICS_FOLDER | ICS_NEW)

#define ICS_AB_CHANGE			(ICS_AB | ICS_CHANGE)
#define ICS_AB_NEW				(ICS_AB | ICS_NEW)
#define ICS_AB_DELETE			(ICS_AB | ICS_HARD_DELETE)

#define ICS_ACTION(x)			((x) & ICS_ACTION_MASK)

#endif

