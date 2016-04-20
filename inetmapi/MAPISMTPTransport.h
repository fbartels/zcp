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

// based on vmime/messaging/smtp/SMTPTransport.hpp, but with additions
// we cannot use a class derived from SMTPTransport, since that class has alot of privates

//
// VMime library (http://www.vmime.org)
// Copyright (C) 2002-2009 Vincent Richard <vincent@vincent-richard.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//

#ifndef MAPI_NET_SMTP_SMTPTRANSPORT_HPP_INCLUDED
#define MAPI_NET_SMTP_SMTPTRANSPORT_HPP_INCLUDED


#include "vmime/config.hpp"

#include "vmime/net/transport.hpp"
#include "vmime/net/socket.hpp"
#include "vmime/net/timeoutHandler.hpp"

#include "vmime/net/smtp/SMTPServiceInfos.hpp"

#include <inetmapi/inetmapi.h>

namespace vmime {
namespace net {
namespace smtp {


class SMTPResponse;


/** SMTP transport service.
  */

class MAPISMTPTransport : public transport
{
public:

	MAPISMTPTransport(ref <session> sess, ref <security::authenticator> auth, const bool secured = false);
	~MAPISMTPTransport();

	const string getProtocolName() const;

	static const serviceInfos& getInfosInstance();
	const serviceInfos& getInfos() const;

	void connect();
	bool isConnected() const;
	void disconnect();

	void noop();

	void send(const mailbox& expeditor, const mailboxList& recipients, utility::inputStream& is, const utility::stream::size_type size, utility::progressListener* progress = NULL);

	bool isSecuredConnection() const;
	ref <connectionInfos> getConnectionInfos() const;

	// additional functions
	const std::vector<sFailedRecip> &getPermanentFailedRecipients(void) const;
	const std::vector<sFailedRecip> &getTemporaryFailedRecipients(void) const;
	void setLogger(ECLogger *lpLogger);
	void requestDSN(BOOL bRequest, const std::string &strTrackid);

private:

	void sendRequest(const string& buffer, const bool end = true);
	ref <SMTPResponse> readResponse();

	void internalDisconnect();

	void helo();
	void authenticate();
#if VMIME_HAVE_SASL_SUPPORT
	void authenticateSASL();
#endif // VMIME_HAVE_SASL_SUPPORT

#if VMIME_HAVE_TLS_SUPPORT
	void startTLS();
#endif // VMIME_HAVE_TLS_SUPPORT

	ref <socket> m_socket;
	bool m_authentified;

	bool m_extendedSMTP;
	std::map <string, std::vector <string> > m_extensions;

	ref <timeoutHandler> m_timeoutHandler;

	const bool m_isSMTPS;

	bool m_secured;
	ref <connectionInfos> m_cntInfos;


	// Service infos
	static SMTPServiceInfos sm_infos;

	// additional data
	std::vector<sFailedRecip> mTemporaryFailedRecipients;
	std::vector<sFailedRecip> mPermanentFailedRecipients;

	ECLogger *m_lpLogger;
	bool m_bDSNRequest;
	std::string m_strDSNTrackid;
};


} // smtp
} // net
} // vmime


#endif // MAPI_NET_SMTP_SMTPTRANSPORT_HPP_INCLUDED
