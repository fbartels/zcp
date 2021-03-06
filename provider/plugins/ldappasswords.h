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

// -*- Mode: c++ -*-

#ifndef LDAPPASSWORDS_H
#define LDAPPASSWORDS_H

/**
 * @defgroup userplugin_password Password validation
 * @ingroup userplugin
 * @{
 */


enum {
	PASSWORD_CRYPT,
	PASSWORD_MD5,
	PASSWORD_SMD5,
	PASSWORD_SHA,
	PASSWORD_SSHA
};

/**
 * Encrypt passwird using requested encryption type
 *
 * The returned array must be deleted with delete []
 *
 * @param[in]	type
 *					The encryption type (CRYPT, MD5, SMD5, SSHA)
 * @param[in]	password
 *					The password which should be encrypted
 * @return The encrypted password
 */
extern char *encryptPassword(int type, const char *password);

/**
 * Compare unencrypted password with encrypted password with the
 * requested encryption type
 *
 * @param[in]	type
 *					The encryption type (CRYPT, MD5, SMD5, SSHA)
 * @param[in]	password
 *					The unencryped password which should match crypted
 * @param[in]	crypted
 *					The encrypted password which should match password
 * @return 0 when the passwords match
 */
extern int checkPassword(int type, const char *password, const char *crypted);

/** @} */
#endif
