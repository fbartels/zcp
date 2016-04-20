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

#ifndef ICONV_H
#define ICONV_H

#include <zarafa/zcdefs.h>
#include <zarafa/charset/convert.h>
#include <string>

class ECIConv _zcp_final {
public:
    ECIConv(const std::string &strToCharset, const std::string &strFromCharset);
    ~ECIConv();
    
	bool canConvert();
    std::string convert(const std::string &input);

private:
	typedef details::iconv_context<std::string, std::string> context_t;
	context_t *m_lpContext;
};

#endif
