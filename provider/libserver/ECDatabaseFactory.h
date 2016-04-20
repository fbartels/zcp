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

#ifndef ECDATABASEFACTORY_H
#define ECDATABASEFACTORY_H

#include "ECDatabase.h"
#include <zarafa/ECConfig.h>
#include <zarafa/ECLogger.h>

// The ECDatabaseFactory creates database objects connected to the server database. Which
// database is returned is chosen by the database_engine configuration setting.

class ECDatabaseFactory _zcp_final {
public:
	ECDatabaseFactory(ECConfig *lpConfig);
	
	ECRESULT		CreateDatabaseObject(ECDatabase **lppDatabase, std::string &ConnectError);
	ECRESULT		CreateDatabase();
	ECRESULT		UpdateDatabase(bool bForceUpdate, std::string &strError);

private:
	ECRESULT GetDatabaseFactory(ECDatabase **lppDatabase);

	ECConfig*		m_lpConfig;
};

ECRESULT	GetThreadLocalDatabase(ECDatabaseFactory *lpFactory, ECDatabase **lppDatabase);

#endif // ECDATABASEFACTORY_H
