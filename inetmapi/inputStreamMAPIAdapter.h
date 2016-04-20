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

#ifndef INPUT_STREAM_MAPI_ADAPTER_H
#define INPUT_STREAM_MAPI_ADAPTER_H

#include <mapidefs.h>
#include <vmime/utility/stream.hpp>

class inputStreamMAPIAdapter : public vmime::utility::inputStream {
public:
	inputStreamMAPIAdapter(IStream *lpStream);
	virtual ~inputStreamMAPIAdapter();

	virtual size_type read(value_type* const data, const size_type count);
	virtual size_type skip(const size_type count);
	virtual void reset();
	virtual bool eof() const;

private:
	bool	ateof;
	IStream *lpStream;
};

#endif
