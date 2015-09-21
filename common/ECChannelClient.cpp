/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation with the following
 * additional terms according to sec. 7:
 *
 * "Zarafa" is a registered trademark of Zarafa B.V.
 * The licensing of the Program under the AGPL does not imply a trademark
 * license. Therefore any rights, title and interest in our trademarks
 * remain entirely with us.
 *
 * Our trademark policy (see TRADEMARKS.txt) allows you to use our trademarks
 * in connection with Propagation and certain other acts regarding the Program.
 * In any case, if you propagate an unmodified version of the Program you are
 * allowed to use the term "Zarafa" to indicate that you distribute the Program.
 * Furthermore you may use our trademarks where it is necessary to indicate the
 * intended purpose of a product or service provided you use it in accordance
 * with honest business practices. For questions please contact Zarafa at
 * trademark@zarafa.com.
 *
 * The interactive user interface of the software displays an attribution
 * notice containing the term "Zarafa" and/or the logo of Zarafa.
 * Interactive user interfaces of unmodified and modified versions must
 * display Appropriate Legal Notices according to sec. 5 of the GNU Affero
 * General Public License, version 3, when you propagate unmodified or
 * modified versions of the Program. In accordance with sec. 7 b) of the GNU
 * Affero General Public License, version 3, these Appropriate Legal Notices
 * must retain the logo of Zarafa or display the words "Initial Development
 * by Zarafa" if the display of the logo is not reasonably feasible for
 * technical reasons.
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

#include <zarafa/platform.h>

#include <mapidefs.h>
#include <mapiutil.h>
#include <mapix.h>

#ifdef LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#endif

#include <zarafa/base64.h>
#include <ECChannel.h>
#include <zarafa/ECDefs.h>
#include <stringutil.h>

#include "ECChannelClient.h"

ECChannelClient::ECChannelClient(const char *szPath, const char *szTokenizer)
{
	m_strTokenizer = szTokenizer;

	m_strPath = GetServerNameFromPath(szPath);

	if ((strncmp(szPath, "file", 4) == 0) || (szPath[0] == PATH_SEPARATOR)) {
		m_bSocket = true;
		m_ulPort = 0;
	} else {
		m_bSocket = false;
		m_ulPort = atoi(GetServerPortFromPath(szPath).c_str());
	}

	m_lpChannel = NULL;
	m_ulTimeout = 5;
}

ECChannelClient::~ECChannelClient()
{
	if (m_lpChannel)
		delete m_lpChannel;
}

ECRESULT ECChannelClient::DoCmd(const std::string &strCommand, std::vector<std::string> &lstResponse)
{
	ECRESULT er = erSuccess;
	std::string strResponse;

	er = Connect();
	if (er != erSuccess)
		goto exit;

	er = m_lpChannel->HrWriteLine(strCommand);
	if (er != erSuccess)
		goto exit;

	er = m_lpChannel->HrSelect(m_ulTimeout);
	if (er != erSuccess)
		goto exit;

	// @todo, should be able to read more than 4MB of results
	er = m_lpChannel->HrReadLine(&strResponse, 4*1024*1024);
	if (er != erSuccess)
		goto exit;

	lstResponse = tokenize(strResponse, m_strTokenizer);

	if (!lstResponse.empty() && lstResponse.front() == "OK") {
		lstResponse.erase(lstResponse.begin());
	} else {
		er = ZARAFA_E_CALL_FAILED;
		goto exit;
	}

exit:
	return er;
}

ECRESULT ECChannelClient::Connect()
{
	ECRESULT er = erSuccess;

	if (!m_lpChannel) {
		if (m_bSocket)
			er = ConnectSocket();
		else
			er = ConnectHttp();
	}

	return er;
}

ECRESULT ECChannelClient::ConnectSocket()
{
#ifdef WIN32
	// TODO: named pipe?
	return MAPI_E_NO_SUPPORT;
#else
	ECRESULT er = erSuccess;
	int fd = -1;
	struct sockaddr_un saddr;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, m_strPath.c_str());

	if ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		er = ZARAFA_E_INVALID_PARAMETER;
		goto exit;
	}

	if (connect(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		er = ZARAFA_E_NETWORK_ERROR;
		goto exit;
	}

	m_lpChannel = new ECChannel(fd);
	if (!m_lpChannel) {
		er = ZARAFA_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

exit:
	if (er != erSuccess && fd != -1)
		closesocket(fd);

	return er;
#endif /* WIN32 */
}

ECRESULT ECChannelClient::ConnectHttp()
{
	ECRESULT er = erSuccess;
	int fd = -1;
	struct sockaddr_in saddr;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(m_strPath.c_str());
	saddr.sin_port = htons(m_ulPort);

#ifdef WIN32
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		er = ZARAFA_E_CALL_FAILED;
		goto exit;
	}
#endif

	if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		er = ZARAFA_E_NETWORK_ERROR;
		goto exit;
	}

	if (connect(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		er = ZARAFA_E_NETWORK_ERROR;
		goto exit;
	}

	m_lpChannel = new ECChannel(fd);
	if (!m_lpChannel) {
		er = ZARAFA_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

exit:
	if (er != erSuccess && fd != -1)
		closesocket(fd);

	return er;
}
