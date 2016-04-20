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

// based on src/net/smtp/SMTPTransport.cpp, but with additions
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

#include <zarafa/platform.h>
#include <zarafa/stringutil.h>
#include "MAPISMTPTransport.h"
#include "vmime/net/smtp/SMTPResponse.hpp"

#include "vmime/exception.hpp"
#include "vmime/platform.hpp"
#include "vmime/mailboxList.hpp"

#include "vmime/utility/filteredStream.hpp"
#include "vmime/utility/stringUtils.hpp"
#include "vmime/net/defaultConnectionInfos.hpp"

#include <zarafa/ECDebugPrint.h>
#include <zarafa/ECLogger.h>
#include <zarafa/charset/traits.h>

#if VMIME_HAVE_SASL_SUPPORT
	#include "vmime/security/sasl/SASLContext.hpp"
#endif // VMIME_HAVE_SASL_SUPPORT

#if VMIME_HAVE_TLS_SUPPORT
	#include "vmime/net/tls/TLSSession.hpp"
	#include "vmime/net/tls/TLSSecuredConnectionInfos.hpp"
#endif // VMIME_HAVE_TLS_SUPPORT


// Helpers for service properties
#define GET_PROPERTY(type, prop) \
	(getInfos().getPropertyValue <type>(getSession(), \
		dynamic_cast <const SMTPServiceInfos&>(getInfos()).getProperties().prop))
#define HAS_PROPERTY(prop) \
	(getInfos().hasProperty(getSession(), \
		dynamic_cast <const SMTPServiceInfos&>(getInfos()).getProperties().prop))

// register new service, really hacked from (src/net/builtinServices.inl)
#include "serviceRegistration.inl"
REGISTER_SERVICE(smtp::MAPISMTPTransport, mapismtp, TYPE_TRANSPORT);

namespace vmime {
namespace net {
namespace smtp {


MAPISMTPTransport::MAPISMTPTransport(ref <session> sess, ref <security::authenticator> auth, const bool secured)
	: transport(sess, getInfosInstance(), auth), m_socket(NULL),
	  m_authentified(false), m_extendedSMTP(false), m_timeoutHandler(NULL),
	  m_isSMTPS(secured), m_secured(false), m_lpLogger(NULL),
	  m_bDSNRequest(false)
{
}


MAPISMTPTransport::~MAPISMTPTransport()
{
	try
	{
		if (isConnected())
			disconnect();
		else if (m_socket)
			internalDisconnect();
	}
	catch (vmime::exception&)
	{
		// Ignore
	}
	if (m_lpLogger != NULL)
		m_lpLogger->Release();
}


const string MAPISMTPTransport::getProtocolName() const
{
	return "mapismtp";
}


void MAPISMTPTransport::connect()
{
	if (isConnected())
		throw exceptions::already_connected();

	const string address = GET_PROPERTY(string, PROPERTY_SERVER_ADDRESS);
	const port_t port = GET_PROPERTY(port_t, PROPERTY_SERVER_PORT);

	// Create the time-out handler
	if (getTimeoutHandlerFactory())
		m_timeoutHandler = getTimeoutHandlerFactory()->create();

	// Create and connect the socket
	// @note zarafa edit: we don't want a timeout during the connect() call
	// because if we set this, the side-effect is when IPv6 is tried first, it will timeout
	// the handler will break the loop by returning false from the handleTimeOut() function.
	m_socket = getSocketFactory()->create();

#if VMIME_HAVE_TLS_SUPPORT
	if (m_isSMTPS)  // dedicated port/SMTPS
	{
		ref <tls::TLSSession> tlsSession =
			vmime::create <tls::TLSSession>(getCertificateVerifier());

		ref <tls::TLSSocket> tlsSocket =
			tlsSession->getSocket(m_socket);

		m_socket = tlsSocket;

		m_secured = true;
		m_cntInfos = vmime::create <tls::TLSSecuredConnectionInfos>(address, port, tlsSession, tlsSocket);
	}
	else
#endif // VMIME_HAVE_TLS_SUPPORT
	{
		m_cntInfos = vmime::create <defaultConnectionInfos>(address, port);
	}

	if (m_lpLogger && m_lpLogger->Log(EC_LOGLEVEL_DEBUG))
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "SMTP connecting to %s:%d", address.c_str(), port);

	m_socket->connect(address, port);

	if (m_lpLogger && m_lpLogger->Log(EC_LOGLEVEL_DEBUG))
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "SMTP server connected.");

	// Connection
	//
	// eg:  C: <connection to server>
	// ---  S: 220 smtp.domain.com Service ready

	ref <SMTPResponse> resp;

	if ((resp = readResponse())->getCode() != 220)
	{
		internalDisconnect();
		throw exceptions::connection_greeting_error(resp->getText());
	}

	// Identification
	helo();

#if VMIME_HAVE_TLS_SUPPORT
	// Setup secured connection, if requested
	const bool tls = HAS_PROPERTY(PROPERTY_CONNECTION_TLS)
		&& GET_PROPERTY(bool, PROPERTY_CONNECTION_TLS);
	const bool tlsRequired = HAS_PROPERTY(PROPERTY_CONNECTION_TLS_REQUIRED)
		&& GET_PROPERTY(bool, PROPERTY_CONNECTION_TLS_REQUIRED);

	if (!m_isSMTPS && tls)  // only if not SMTPS
	{
		try
		{
			startTLS();
		}
		// Non-fatal error
		catch (exceptions::command_error&)
		{
			if (tlsRequired)
			{
				throw;
			}
			else
			{
				// TLS is not required, so don't bother
			}
		}
		// Fatal error
		catch (...)
		{
			throw;
		}

		// Must reissue a EHLO command [RFC-2487, 5.2]
		helo();
	}
#endif // VMIME_HAVE_TLS_SUPPORT

	// Authentication
	if (GET_PROPERTY(bool, PROPERTY_OPTIONS_NEEDAUTH))
		authenticate();
	else
		m_authentified = true;
}


void MAPISMTPTransport::helo()
{
	// First, try Extended SMTP (ESMTP)
	//
	// eg:  C: EHLO thismachine.ourdomain.com
	//      S: 250-smtp.theserver.com
	//      S: 250-AUTH CRAM-MD5 DIGEST-MD5
	//      S: 250-PIPELINING
	//      S: 250 SIZE 2555555555

	sendRequest("EHLO " + platform::getHandler()->getHostName());

	ref <SMTPResponse> resp;

	if ((resp = readResponse())->getCode() != 250)
	{
		// Next, try "Basic" SMTP
		//
		// eg:  C: HELO thismachine.ourdomain.com
		//      S: 250 OK

		sendRequest("HELO " + platform::getHandler()->getHostName());

		if ((resp = readResponse())->getCode() != 250)
		{
			internalDisconnect();
			throw exceptions::connection_greeting_error(resp->getLastLine().getText());
		}

		m_extendedSMTP = false;
		m_extensions.clear();
	}
	else
	{
		m_extendedSMTP = true;
		m_extensions.clear();

		// Get supported extensions from SMTP response
		// One extension per line, format is: EXT PARAM1 PARAM2...
		for (int i = 1, n = resp->getLineCount() ; i < n ; ++i)
		{
			const string line = resp->getLineAt(i).getText();
			std::istringstream iss(line);

			string ext;
			iss >> ext;

			std::vector <string> params;
			string param;

			// Special case: some servers send "AUTH=MECH [MECH MECH...]"
			if (ext.length() >= 5 && utility::stringUtils::toUpper(ext.substr(0, 5)) == "AUTH=")
			{
				params.push_back(utility::stringUtils::toUpper(ext.substr(5)));
				ext = "AUTH";
			}

			while (iss >> param)
				params.push_back(utility::stringUtils::toUpper(param));

			m_extensions[ext] = params;
		}
	}
}


void MAPISMTPTransport::authenticate()
{
	if (!m_extendedSMTP)
	{
		internalDisconnect();
		throw exceptions::command_error("AUTH", "ESMTP not supported.");
	}

	getAuthenticator()->setService(thisRef().dynamicCast <service>());

#if VMIME_HAVE_SASL_SUPPORT
	// First, try SASL authentication
	if (GET_PROPERTY(bool, PROPERTY_OPTIONS_SASL))
	{
		try
		{
			authenticateSASL();

			m_authentified = true;
			return;
		}
		catch (exceptions::authentication_error& e)
		{
			if (!GET_PROPERTY(bool, PROPERTY_OPTIONS_SASL_FALLBACK))
			{
				// Can't fallback on normal authentication
				internalDisconnect();
				throw e;
			}
			else
			{
				// Ignore, will try normal authentication
			}
		}
		catch (exception& e)
		{
			internalDisconnect();
			throw e;
		}
	}
#endif // VMIME_HAVE_SASL_SUPPORT

	// No other authentication method is possible
	throw exceptions::authentication_error("All authentication methods failed");
}


#if VMIME_HAVE_SASL_SUPPORT

void MAPISMTPTransport::authenticateSASL()
{
	if (!getAuthenticator().dynamicCast <security::sasl::SASLAuthenticator>())
		throw exceptions::authentication_error("No SASL authenticator available.");

	// Obtain SASL mechanisms supported by server from ESMTP extensions
	const std::vector <string> saslMechs =
		(m_extensions.find("AUTH") != m_extensions.end())
			? m_extensions["AUTH"] : std::vector <string>();

	if (saslMechs.empty())
		throw exceptions::authentication_error("No SASL mechanism available.");

	std::vector <ref <security::sasl::SASLMechanism> > mechList;

	ref <security::sasl::SASLContext> saslContext =
		vmime::create <security::sasl::SASLContext>();

	for (unsigned int i = 0 ; i < saslMechs.size() ; ++i)
	{
		try
		{
			mechList.push_back
				(saslContext->createMechanism(saslMechs[i]));
		}
		catch (exceptions::no_such_mechanism&)
		{
			// Ignore mechanism
		}
	}

	if (mechList.empty())
		throw exceptions::authentication_error("No SASL mechanism available.");

	// Try to suggest a mechanism among all those supported
	ref <security::sasl::SASLMechanism> suggestedMech =
		saslContext->suggestMechanism(mechList);

	if (!suggestedMech)
		throw exceptions::authentication_error("Unable to suggest SASL mechanism.");

	// Allow application to choose which mechanisms to use
	mechList = getAuthenticator().dynamicCast <security::sasl::SASLAuthenticator>()->
		getAcceptableMechanisms(mechList, suggestedMech);

	if (mechList.empty())
		throw exceptions::authentication_error("No SASL mechanism available.");

	// Try each mechanism in the list in turn
	for (unsigned int i = 0 ; i < mechList.size() ; ++i)
	{
		ref <security::sasl::SASLMechanism> mech = mechList[i];

		ref <security::sasl::SASLSession> saslSession =
			saslContext->createSession("smtp", getAuthenticator(), mech);

		saslSession->init();

		sendRequest("AUTH " + mech->getName());

		for (bool cont = true ; cont ; )
		{
			ref <SMTPResponse> response = readResponse();

			switch (response->getCode())
			{
			case 235:
			{
				m_socket = saslSession->getSecuredSocket(m_socket);
				return;
			}
			case 334:
			{
				byte_t* challenge = 0;
				int challengeLen = 0;

				byte_t* resp = 0;
				int respLen = 0;

				try
				{
					// Extract challenge
					saslContext->decodeB64(response->getText(), &challenge, &challengeLen);

					// Prepare response
					saslSession->evaluateChallenge
						(challenge, challengeLen, &resp, &respLen);

					// Send response
					sendRequest(saslContext->encodeB64(resp, respLen));
				}
				catch (exceptions::sasl_exception& e)
				{
					delete[] challenge;
					challenge = NULL;
					delete[] resp;
					resp = NULL;
					// Cancel SASL exchange
					sendRequest("*");
				}
				catch (...)
				{
					delete[] challenge;
					delete[] resp;
					throw;
				}

				delete[] challenge;
				delete[] resp;
				break;
			}
			default:

				cont = false;
				break;
			}
		}
	}

	throw exceptions::authentication_error
		("Could not authenticate using SASL: all mechanisms failed.");
}

#endif // VMIME_HAVE_SASL_SUPPORT


#if VMIME_HAVE_TLS_SUPPORT

void MAPISMTPTransport::startTLS()
{
	try
	{
		sendRequest("STARTTLS");

		ref <SMTPResponse> resp = readResponse();

		if (resp->getCode() != 220)
			throw exceptions::command_error("STARTTLS", resp->getText());

		ref <tls::TLSSession> tlsSession =
			vmime::create <tls::TLSSession>(getCertificateVerifier());

		ref <tls::TLSSocket> tlsSocket =
			tlsSession->getSocket(m_socket);

		tlsSocket->handshake(m_timeoutHandler);

		m_socket = tlsSocket;

		m_secured = true;
		m_cntInfos = vmime::create <tls::TLSSecuredConnectionInfos>
			(m_cntInfos->getHost(), m_cntInfos->getPort(), tlsSession, tlsSocket);
	}
	catch (exceptions::command_error&)
	{
		// Non-fatal error
		throw;
	}
	catch (exception&)
	{
		// Fatal error
		internalDisconnect();
		throw;
	}
}

#endif // VMIME_HAVE_TLS_SUPPORT


bool MAPISMTPTransport::isConnected() const
{
	return (m_socket && m_socket->isConnected() && m_authentified);
}


bool MAPISMTPTransport::isSecuredConnection() const
{
	return m_secured;
}


ref <connectionInfos> MAPISMTPTransport::getConnectionInfos() const
{
	return m_cntInfos;
}


void MAPISMTPTransport::disconnect()
{
	if (!isConnected())
		throw exceptions::not_connected();

	internalDisconnect();
}


void MAPISMTPTransport::internalDisconnect()
{
	try
	{
		sendRequest("QUIT");
	}
	catch (exception&)
	{
		// Not important
	}

	m_socket->disconnect();
	m_socket = NULL;

	m_timeoutHandler = NULL;

	m_authentified = false;
	m_extendedSMTP = false;

	m_secured = false;
	m_cntInfos = NULL;
}


void MAPISMTPTransport::noop()
{
	if (!isConnected())
		throw exceptions::not_connected();

	sendRequest("NOOP");

	ref <SMTPResponse> resp = readResponse();

	if (resp->getCode() != 250)
		throw exceptions::command_error("NOOP", resp->getText());
}


//                             
// Only this function is altered, to return per recipient failure.
//                             
void MAPISMTPTransport::send(const mailbox& expeditor, const mailboxList& recipients,
                         utility::inputStream& is, const utility::stream::size_type size,
                         utility::progressListener* progress)
{
	if (!isConnected())
		throw exceptions::not_connected();

	// If no recipient/expeditor was found, throw an exception
	if (recipients.isEmpty())
		throw exceptions::no_recipient();
	else if (expeditor.isEmpty())
		throw exceptions::no_expeditor();

	// Emit the "MAIL" command
	ref <SMTPResponse> resp;
	string strSend;
	bool bDSN = m_bDSNRequest;
	
	if(bDSN && m_extensions.find("DSN") == m_extensions.end()) {
		if (m_lpLogger) m_lpLogger->Log(EC_LOGLEVEL_NOTICE, "SMTP server does not support Delivery Status Notifications (DSN)");
		bDSN = false; // Disable DSN because the server does not support this.
	}

	strSend = "MAIL FROM: <" + expeditor.getEmail() + ">";
	if (bDSN) {
		strSend += " RET=HDRS";
		if (!m_strDSNTrackid.empty())
			strSend += " ENVID=" + m_strDSNTrackid;
	}

	sendRequest(strSend);

	resp = readResponse();
	if (resp->getCode() / 10 != 25) {
		internalDisconnect();
		throw exceptions::command_error("MAIL", resp->getText());
	}

	// Emit a "RCPT TO" command for each recipient
	mTemporaryFailedRecipients.clear();
	mPermanentFailedRecipients.clear();
	for (int i = 0 ; i < recipients.getMailboxCount() ; ++i)
	{
		const mailbox& mbox = *recipients.getMailboxAt(i);
		unsigned int code;

		strSend = "RCPT TO: <" + mbox.getEmail() + ">";
		if (bDSN)
			 strSend += " NOTIFY=SUCCESS,DELAY";

		sendRequest(strSend);
		resp = readResponse();
		code = resp->getCode();

		sFailedRecip entry;
		entry.strRecipName = (WCHAR*)mbox.getName().getConvertedText(charset(CHARSET_WCHAR)).c_str(); // does this work?, or convert to utf-8 then wstring?
		entry.strRecipEmail = mbox.getEmail();
		entry.ulSMTPcode = code;
		entry.strSMTPResponse = resp->getText();

		if (code / 10 == 25) {
			continue;
		} else if (code == 421) {
			/* 421 4.7.0 localhorse.lh Error: too many errors */
			ec_log_err("RCPT line gave SMTP error: %d %s. (and now?)",
				resp->getCode(), resp->getText().c_str());
			break;
		} else if (code / 100 == 5) {
			/*
			 * Example Postfix codes:
			 * 501 5.1.3 Bad recipient address syntax  (RCPT TO: <with spaces>)
			 * 550 5.1.1 <fox>: Recipient address rejected: User unknown in virtual mailbox table
			 * 550 5.7.1 REJECT action without code by means of e.g. /etc/postfix/header_checks
			 */
			mPermanentFailedRecipients.push_back(entry);
			ec_log_err("RCPT line gave SMTP error %d %s. (no retry)",
				resp->getCode(), resp->getText().c_str());
			continue;
		} else if (code / 100 != 4) {
			mPermanentFailedRecipients.push_back(entry);
			ec_log_err("RCPT line gave unexpected SMTP reply %d %s. (no retry)",
				resp->getCode(), resp->getText().c_str());
			continue;
		}

		/* Other 4xx codes (disk full, ... ?) */
		mTemporaryFailedRecipients.push_back(entry);
		ec_log_err("RCPT line gave SMTP error: %d %s. (will be retried)",
			resp->getCode(), resp->getText().c_str());
	}

	// Send the message data
	sendRequest("DATA");

	// we also stop here if all recipients failed before
	if ((resp = readResponse())->getCode() != 354)
	{
		internalDisconnect();
		throw exceptions::command_error("DATA", format("%d %s", resp->getCode(), resp->getText().c_str()));
	}

	// Stream copy with "\n." to "\n.." transformation
	utility::outputStreamSocketAdapter sos(*m_socket);
	utility::dotFilteredOutputStream fos(sos);

	utility::bufferedStreamCopy(is, fos, size, progress);

	fos.flush();

	// Send end-of-data delimiter
	m_socket->sendRaw("\r\n.\r\n", 5);

	if ((resp = readResponse())->getCode() != 250)
	{
		internalDisconnect();
		throw exceptions::command_error("DATA", format("%d %s", resp->getCode(), resp->getText().c_str()));
	} else {
		// postfix: 2.0.0 Ok: queued as B36E73608E
		// qmail: ok 1295860788 qp 29154
		// exim: OK id=1PhIZ9-0002Ko-Q8
		if (!m_lpLogger->Log(EC_LOGLEVEL_DEBUG)) // prevent double logging
			m_lpLogger->Log(EC_LOGLEVEL_WARNING, "SMTP: %s", resp->getText().c_str());
	}
}

const std::vector<sFailedRecip> &
MAPISMTPTransport::getPermanentFailedRecipients(void) const
{
	return mPermanentFailedRecipients;
}

const std::vector<sFailedRecip> &
MAPISMTPTransport::getTemporaryFailedRecipients(void) const
{
	return mTemporaryFailedRecipients;
}

void MAPISMTPTransport::setLogger(ECLogger *lpLogger)
{                              
	if (m_lpLogger != NULL)
		m_lpLogger->Release();
	m_lpLogger = lpLogger;
	if (m_lpLogger != NULL)
		m_lpLogger->AddRef();
}                              

void MAPISMTPTransport::requestDSN(BOOL bRequest, const std::string &strTrackid)
{
	m_bDSNRequest = bRequest;
	m_strDSNTrackid = strTrackid;
}

void MAPISMTPTransport::sendRequest(const string& buffer, const bool end)
{
	if (m_lpLogger && m_lpLogger->Log(EC_LOGLEVEL_DEBUG))
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "< %s", buffer.c_str());
	if (end)
		m_socket->send(buffer + "\r\n");
	else
		m_socket->send(buffer);
}


ref <SMTPResponse> MAPISMTPTransport::readResponse()
{
	ref <SMTPResponse> resp = SMTPResponse::readResponse(m_socket, m_timeoutHandler);
	if (m_lpLogger && m_lpLogger->Log(EC_LOGLEVEL_DEBUG))
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "> %d %s", resp->getCode(), resp->getText().c_str());
	return resp;
}



// Service infos

SMTPServiceInfos MAPISMTPTransport::sm_infos(false);


const serviceInfos& MAPISMTPTransport::getInfosInstance()
{
	return sm_infos;
}


const serviceInfos& MAPISMTPTransport::getInfos() const
{
	return sm_infos;
}


} // smtp
} // net
} // vmime
