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

#ifndef SOAPSOCK_H
#define SOAPSOCK_H

#ifdef WIN32
// For WSAIoctl
#include <Winsock2.h>
#include <Mstcpip.h>
#include <Wincrypt.h>
#include <Cryptuiapi.h>
#endif

#include <openssl/ssl.h>
#include "soapZarafaCmdProxy.h"

int ssl_verify_callback_zarafa_silent(int ok, X509_STORE_CTX *store);
int ssl_verify_callback_zarafa(int ok, X509_STORE_CTX *store);
int ssl_verify_callback_zarafa_control(int ok, X509_STORE_CTX *store, BOOL bShowDlg);

HRESULT LoadCertificatesFromRegistry();

HRESULT CreateSoapTransport(ULONG ulUIFlags,
	const char *strServerPath,
	const char *strSSLKeyFile,
	const char *strSSLKeyPass,
	ULONG ulConnectionTimeOut,
	const char *strProxyHost,
	WORD wProxyPort,
	const char *strProxyUserName,
	const char *strProxyPassword,
	ULONG ulProxyFlags,
	int				iSoapiMode,
	int				iSoapoMode,
	ZarafaCmd **lppCmd);


VOID DestroySoapTransport(ZarafaCmd *lpCmd);
#endif
