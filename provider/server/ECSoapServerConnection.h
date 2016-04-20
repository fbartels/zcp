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

#ifndef ECSOAPSERVERCONNECTION_H
#define ECSOAPSERVERCONNECTION_H

#include <zarafa/zcdefs.h>
#include <set>
#include <zarafa/ZarafaCode.h>
#include "ECThreadManager.h"
#include "soapH.h"
#include <zarafa/ECLogger.h>
#include <zarafa/ECConfig.h>

class ECSoapServerConnection _zcp_final {
public:
	ECSoapServerConnection(ECConfig* lpConfig, ECLogger* lpLogger);
	~ECSoapServerConnection();

	ECRESULT ListenTCP(const char* lpServerName, int nServerPort, bool bEnableGET);
	ECRESULT ListenSSL(const char* lpServerName, int nServerPort, bool bEnableGET, const char* lpszKeyFile, const char* lpszKeyPass,
						const char* lpszCAFile, const char* lpszCAPath);
	ECRESULT ListenPipe(const char* lpPipeName, bool bPriority = false);

	ECRESULT MainLoop();
	
	// These can be called asynchronously from MainLoop();
	ECRESULT NotifyDone(struct soap *soap);
	ECRESULT ShutDown();
	ECRESULT DoHUP();
	ECRESULT GetStats(unsigned int *lpulQueueLength, double *lpdblAge, unsigned int *lpulThreadCount, unsigned int *lpulIdleThreads);

	static SOAP_SOCKET CreatePipeSocketCallback(void *lpParam);

private:
    // Main thread handler
    ECDispatcher *m_lpDispatcher;

	ECConfig*	m_lpConfig;
	ECLogger*	m_lpLogger;
	std::string	m_strPipeName;
};

#endif // #ifndef ECSOAPSERVERCONNECTION_H
