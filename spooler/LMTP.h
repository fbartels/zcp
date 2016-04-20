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

#ifndef LMTP_H
#define LMTP_H

#include <string>
#include <vector>
#include <zarafa/ECChannel.h>
#include <zarafa/ECLogger.h>
#include <zarafa/ECConfig.h>

enum LMTP_Command {LMTP_Command_LHLO, LMTP_Command_MAIL_FROM, LMTP_Command_RCPT_TO, LMTP_Command_DATA, LMTP_Command_RSET, LMTP_Command_QUIT };

class LMTP {
public:
	LMTP(ECChannel *lpChan, const char *szServerPath, ECLogger *lpLog, ECConfig *lpConf);
	~LMTP();

	HRESULT HrGetCommand(const std::string &strCommand, LMTP_Command &eCommand);
	HRESULT HrResponse(const std::string &strResponse);

	HRESULT HrCommandLHLO(const std::string &strInput, std::string & nameOut);
	HRESULT HrCommandMAILFROM(const std::string &strFrom, std::string *const strAddress);
	HRESULT HrCommandRCPTTO(const std::string &to_address, std::string *mail_address_unsolved);
	HRESULT HrCommandDATA(FILE *tmp);
	HRESULT HrCommandQUIT();

private:
	HRESULT HrParseAddress(const std::string &strAddress, std::string *strEmail);

	ECChannel		*m_lpChannel;
	ECLogger		*m_lpLogger;
	ECConfig		*m_lpConfig;
	std::string		m_strPath;
};

#endif

