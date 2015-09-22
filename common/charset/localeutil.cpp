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
#include <string>
#include <iostream>
#include <cstring>
#include "localeutil.h"

locale_t createUTF8Locale()
{
	locale_t loc;

	// this trick only works on newer distro's
	loc = createlocale(LC_CTYPE, "C.UTF-8");
	if (loc)
		return loc;

	std::string new_locale;
	char *cur_locale = setlocale(LC_CTYPE, NULL);
	char *dot = strchr(cur_locale, '.');
	if (dot) {
		if (strcmp(dot+1, "UTF-8") == 0 || strcmp(dot+1, "utf8") == 0) {
			loc = createlocale(LC_CTYPE, cur_locale);
			goto exit;
		}
		// strip current charset selector, to be replaced
		*dot = '\0';
	}
	new_locale = std::string(cur_locale) + ".UTF-8";
	loc = createlocale(LC_CTYPE, new_locale.c_str()); 
	if (loc)
		return loc;

	loc = createlocale(LC_CTYPE, "en_US.UTF-8");

exit:
	// too bad, but I don't want to return an unusable object
	if (!loc)
		loc = createlocale(LC_CTYPE, "C");

	return loc;
}

/**
 * Initializes the locale to the current language, forced in UTF-8.
 * 
 * @param[in]	bOutput	Print errors during init to stderr
 * @param[out]	lpstrLocale Last locale trying to set (optional)
 * @retval	true	successfully initialized
 * @retval	false	error during initialization
 */
bool forceUTF8Locale(bool bOutput, std::string *lpstrLastSetLocale)
{
	std::string new_locale;
	char *old_locale = setlocale(LC_CTYPE, "");
	if (!old_locale) {
		if (bOutput)
			std::cerr << "Unable to initialize locale" << std::endl;
		return false;
	}
	char *dot = strchr(old_locale, '.');
	if (dot) {
		*dot = '\0';
		if (strcmp(dot+1, "UTF-8") == 0 || strcmp(dot+1, "utf8") == 0) {
			if (lpstrLastSetLocale)
				*lpstrLastSetLocale = old_locale;
			return true; // no need to force anything
		}
	}
	if (bOutput) {
		std::cerr << "Warning: Terminal locale not UTF-8, but UTF-8 locale is being forced." << std::endl;
		std::cerr << "         Screen output may not be correctly printed." << std::endl;
	}
	new_locale = std::string(old_locale) + ".UTF-8";
	if (lpstrLastSetLocale)
		*lpstrLastSetLocale = new_locale;
	old_locale = setlocale(LC_CTYPE, new_locale.c_str());
	if (!old_locale) {
		new_locale = "en_US.UTF-8";
		if (lpstrLastSetLocale)
			*lpstrLastSetLocale = new_locale;
		old_locale = setlocale(LC_CTYPE, new_locale.c_str());
	}
	if (!old_locale) {
		if (bOutput)
			std::cerr << "Unable to set locale '" << new_locale << "'" << std::endl;
		return false;
	}
	return true;
}
