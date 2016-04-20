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

#include <zarafa/zcdefs.h>
#include <string>

class ECConfig;

class TmpPath _zcp_final {
	private:
		std::string path;

	public:
		TmpPath();
		~TmpPath();

		static TmpPath *getInstance();

		bool OverridePath(ECConfig *const ec);

		const std::string & getTempPath() const;
};

extern TmpPath *tmpPath;
