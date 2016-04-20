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

#include <zarafa/platform.h>
#include <new>

#include <mapidefs.h>
#include <mapiutil.h>
#include <mapix.h>

#ifdef LINUX
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#endif

#include <zarafa/base64.h>
#include <zarafa/ECChannel.h>
#include <zarafa/ECDefs.h>
#include <zarafa/stringutil.h>

#include "ECChannelClient.h"

ECChannelClient::ECChannelClient(const char *szPath, const char *szTokenizer)
{
	m_strTokenizer = szTokenizer;

	m_strPath = GetServerNameFromPath(szPath);

	if (strncmp(szPath, "file", 4) == 0 || szPath[0] == PATH_SEPARATOR) {
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
	delete m_lpChannel;
}

ECRESULT ECChannelClient::DoCmd(const std::string &strCommand, std::vector<std::string> &lstResponse)
{
	ECRESULT er;
	std::string strResponse;

	er = Connect();
	if (er != erSuccess)
		return er;

	er = m_lpChannel->HrWriteLine(strCommand);
	if (er != erSuccess)
		return er;

	er = m_lpChannel->HrSelect(m_ulTimeout);
	if (er != erSuccess)
		return er;

	// @todo, should be able to read more than 4MB of results
	er = m_lpChannel->HrReadLine(&strResponse, 4*1024*1024);
	if (er != erSuccess)
		return er;

	lstResponse = tokenize(strResponse, m_strTokenizer);

	if (!lstResponse.empty() && lstResponse.front() == "OK")
		lstResponse.erase(lstResponse.begin());
	else
		return ZARAFA_E_CALL_FAILED;
	return erSuccess;
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

	m_lpChannel = new(std::nothrow) ECChannel(fd);
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
	int fd = -1, ret;
	struct addrinfo *sock_res, sock_hints;
	const struct addrinfo *sock_addr;
	char port_string[sizeof("65536")];

#ifdef WIN32
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		er = ZARAFA_E_CALL_FAILED;
		goto exit;
	}
#endif

	snprintf(port_string, sizeof(port_string), "%u", m_ulPort);
	memset(&sock_hints, 0, sizeof(sock_hints));
	sock_hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(m_strPath.c_str(), port_string, &sock_hints,
	      &sock_res);
	if (ret != 0) {
		er = ZARAFA_E_NETWORK_ERROR;
		goto exit;
	}

	for (sock_addr = sock_res; sock_addr != NULL;
	     sock_addr = sock_addr->ai_next)
	{
		fd = socket(sock_addr->ai_family, sock_addr->ai_socktype,
		     sock_addr->ai_protocol);
		if (fd < 0)
			continue;

		if (connect(fd, sock_addr->ai_addr,
		    sock_addr->ai_addrlen) < 0) {
			int saved_errno = errno;
			closesocket(fd);
			fd = -1;
			errno = saved_errno;
			continue;
		}
	}
	if (fd < 0) {
		er = ZARAFA_E_NETWORK_ERROR;
		goto exit;
	}

	m_lpChannel = new(std::nothrow) ECChannel(fd);
	if (!m_lpChannel) {
		er = ZARAFA_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

exit:
	if (sock_res != NULL)
		freeaddrinfo(sock_res);
	if (er != erSuccess && fd != -1)
		closesocket(fd);

	return er;
}
