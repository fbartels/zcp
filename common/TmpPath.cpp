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

#include <cstdlib>
#include <cstring>
#include <memory>
//
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

#include "TmpPath.h"
#include <zarafa/ECConfig.h>

TmpPath::TmpPath() {
#ifdef WIN32
	char dummy[MAX_PATH + 1] = { 0 };

	// this should only happen when the environment variables
	// have been tampered with
	// note: >= because GetTempPath retuns count WITHOUT 0x00!
	if (GetTempPathA(sizeof dummy, dummy) >= sizeof dummy) {
		// this is a difficult situation. let's assume that
		// we can at least write in the current directory
		// but really if GetTempPath fails, something is
		// very wrong
		dummy[0] = 0x00;
	}

	path = dummy;
#else
	const char *dummy = NULL;

	if (path.empty()) {
		dummy = getenv("TMP");
		if (dummy)
			path = dummy;
	}

	if (path.empty()) {
		dummy = getenv("TEMP");
		if (dummy)
			path = dummy;
	}

	if (!path.empty()) {
		struct stat st;
		if (stat(path.c_str(), &st) == -1)
			path = "/tmp"; // what to do if we can't access that path either? FIXME
	}

	if (path.empty())
		path = "/tmp";
#endif
}

TmpPath::~TmpPath() {
}

bool TmpPath::OverridePath(ECConfig *const ec) {
	bool rc = true;
	const char *newPath = ec->GetSetting("tmp_path");

	if (newPath && newPath[0]) {
		path = newPath;

		size_t s = path.size();
		if (path.at(s - 1) == '/' && s > 1)
			path = path.substr(0, s - 1);

#ifndef WIN32
		struct stat st;
		if (stat(path.c_str(), &st) == -1) {
			path = "/tmp"; // what to do if we can't access that path either? FIXME
			rc = false;
		}

		setenv("TMP", newPath, 1);
		setenv("TEMP", newPath, 1);
#endif
	}

	return rc;
}

const std::string & TmpPath::getTempPath() const {
	return path;
}

TmpPath *TmpPath::getInstance()
{
        static std::auto_ptr<TmpPath> instance(new TmpPath);
        return instance.get();
}
