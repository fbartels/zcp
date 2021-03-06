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

// ECNotification.h: interface for the ECNotification class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ECNOTIFICATION
#define ECNOTIFICATION

#include <zarafa/zcdefs.h>
#include "soapH.h"

class ECNotification _zcp_final {
public:
	ECNotification();
	virtual ~ECNotification();
	ECNotification(const ECNotification &x);
	ECNotification& operator=(const ECNotification &x);

	ECNotification(notification &notification);
	ECNotification& operator=(const notification &srcNotification);

	void SetConnection(unsigned int ulConnection);

	void GetCopy(struct soap *, notification &) const;

	unsigned int GetObjectSize(void) const;

protected:
	void Init();

private:
	notification	*m_lpsNotification;

};

#endif // #ifndef ECNOTIFICATION
