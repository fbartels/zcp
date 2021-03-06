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

#ifndef ECCONFIGCHECK_H
#define ECCONFIGCHECK_H

#include <list>
#include <map>
#include <string>

enum CHECK_STATUS {
	CHECK_OK,
	CHECK_WARNING,
	CHECK_ERROR
};

enum CHECK_FLAGS {
	CONFIG_MANDATORY		= (1 << 0),	/* Configuration option is mandatory */
	CONFIG_HOSTED_USED		= (1 << 1),	/* Configuration option is only usable with hosted enabled */
	CONFIG_HOSTED_UNUSED	= (1 << 2),	/* Configuration option is only usable with hosted disabled */
	CONFIG_MULTI_USED		= (1 << 3),	/* Configuration option is only usable with multi-server enabled */
	CONFIG_MULTI_UNUSED		= (1 << 4)	/* Configuration option is only usable with multi-server disabled */
};

/* Each subclass of ECConfigCheck can register check functions
 * for the configuration options. */
struct config_check_t {
	bool hosted;
	bool multi;
	std::string option1;
	std::string option2;

	std::string value1;
	std::string value2;

	int (*check)(const config_check_t *);
};

class ECConfigCheck {
public:
	ECConfigCheck(const char *lpszName, const char *lpszConfigFile);
	virtual ~ECConfigCheck();

	/* Must be overwritten by subclass */
	virtual void loadChecks() = 0;

	bool isDirty();
	void setHosted(bool hosted);
	void setMulti(bool multi);
	void validate();

	const std::string &getSetting(const std::string &);

protected:
	static void printError(const std::string &, const std::string &);
	static void printWarning(const std::string &, const std::string &);

	void addCheck(const std::string &, unsigned int, int (*)(const config_check_t *) = NULL);
	void addCheck(const std::string &, const std::string &, unsigned int, int (*)(const config_check_t *) = NULL);

private:
	void readConfigFile(const char *lpszConfigFile);

	/* Generic check functions */
	static int testMandatory(const config_check_t *);
	static int testUsedWithHosted(const config_check_t *);
	static int testUsedWithoutHosted(const config_check_t *);
	static int testUsedWithMultiServer(const config_check_t *);
	static int testUsedWithoutMultiServer(const config_check_t *);

protected:
	static int testDirectory(const config_check_t *);
	static int testFile(const config_check_t *);
	static int testCharset(const config_check_t *);
	static int testBoolean(const config_check_t *);
	static int testNonZero(const config_check_t *);

private:
	/* Generic addCheckFunction */
	void addCheck(const config_check_t &, unsigned int);

	/* private variables */
	const char *m_lpszName;
	const char *m_lpszConfigFile;

	std::map<std::string, std::string> m_mSettings;
	std::list<config_check_t> m_lChecks;

	bool m_bDirty;
	bool m_bHosted;
	bool m_bMulti;
};

#endif

