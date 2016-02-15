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
#include <zarafa/ECPluginSharedData.h>

ECPluginSharedData *ECPluginSharedData::m_lpSingleton = NULL;
pthread_mutex_t ECPluginSharedData::m_SingletonLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ECPluginSharedData::m_CreateConfigLock = PTHREAD_MUTEX_INITIALIZER;

ECPluginSharedData::ECPluginSharedData(ECConfig *lpParent, ECLogger *lpLogger, IECStatsCollector *lpStatsCollector, bool bHosted, bool bDistributed)
{
	m_ulRefCount = 0;
	m_lpConfig = NULL;
	m_lpDefaults = NULL;
	m_lpszDirectives = NULL;
	m_lpParentConfig = lpParent;
	m_lpLogger = lpLogger;
	m_lpLogger->AddRef();
	m_lpStatsCollector = lpStatsCollector;
	m_bHosted = bHosted;
	m_bDistributed = bDistributed;
}

ECPluginSharedData::~ECPluginSharedData()
{
	delete m_lpConfig;
	if (m_lpDefaults) {
		for (int n = 0; m_lpDefaults[n].szName; n++) {
			free(const_cast<char *>(m_lpDefaults[n].szName));
			free(const_cast<char *>(m_lpDefaults[n].szValue));
		}
		delete [] m_lpDefaults;
	}
	if (m_lpszDirectives) {
		for (int n = 0; m_lpszDirectives[n]; n++)
			free(m_lpszDirectives[n]);
		delete [] m_lpszDirectives;
	}
	m_lpLogger->Release();
}

void ECPluginSharedData::GetSingleton(ECPluginSharedData **lppSingleton, ECConfig *lpParent, ECLogger *lpLogger,
									  IECStatsCollector *lpStatsCollector, bool bHosted, bool bDistributed)
{
	pthread_mutex_lock(&m_SingletonLock);

	if (!m_lpSingleton)
		m_lpSingleton = new ECPluginSharedData(lpParent, lpLogger, lpStatsCollector, bHosted, bDistributed); 

	m_lpSingleton->m_ulRefCount++;
	*lppSingleton = m_lpSingleton;

	pthread_mutex_unlock(&m_SingletonLock);
}

void ECPluginSharedData::AddRef()
{
	pthread_mutex_lock(&m_SingletonLock);
	m_ulRefCount++;
	pthread_mutex_unlock(&m_SingletonLock);
}

void ECPluginSharedData::Release()
{
	pthread_mutex_lock(&m_SingletonLock);
	if (!--m_ulRefCount) {
		delete m_lpSingleton;
		m_lpSingleton = NULL;
	}
	pthread_mutex_unlock(&m_SingletonLock);
}

ECConfig *ECPluginSharedData::CreateConfig(const configsetting_t *lpDefaults,
    const char *const *lpszDirectives)
{
	pthread_mutex_lock(&m_CreateConfigLock);

	if (!m_lpConfig)
	{
		int n;
		/*
		 * Store all the defaults and directives in the singleton,
		 * so it isn't removed from memory when the plugin unloads.
		 */
		if (lpDefaults) {
			for (n = 0; lpDefaults[n].szName; n++) ;
			m_lpDefaults = new configsetting_t[n+1];
			for (n = 0; lpDefaults[n].szName; n++) {
				m_lpDefaults[n].szName = strdup(lpDefaults[n].szName);
				m_lpDefaults[n].szValue = strdup(lpDefaults[n].szValue);
				m_lpDefaults[n].ulFlags = lpDefaults[n].ulFlags;
				m_lpDefaults[n].ulGroup = lpDefaults[n].ulGroup;
			}
			m_lpDefaults[n].szName = NULL;
			m_lpDefaults[n].szValue = NULL;
		}

		if (lpszDirectives) {
			for (n = 0; lpszDirectives[n]; n++) ;
			m_lpszDirectives = new char*[n+1];
			for (n = 0; lpszDirectives[n]; n++)
				m_lpszDirectives[n] = strdup(lpszDirectives[n]);
			m_lpszDirectives[n] = NULL;
		}

		m_lpConfig = ECConfig::Create(m_lpDefaults, m_lpszDirectives);
		if (!m_lpConfig->LoadSettings(m_lpParentConfig->GetSetting("user_plugin_config")))
			ec_log_err("Failed to open plugin configuration file, using defaults.");
		if (m_lpConfig->HasErrors() || m_lpConfig->HasWarnings()) {
			LogConfigErrors(m_lpConfig);
			if(m_lpConfig->HasErrors()) {
				delete m_lpConfig;
				m_lpConfig = NULL;
			}
		}
	}

	pthread_mutex_unlock(&m_CreateConfigLock);

	return m_lpConfig;
}

ECLogger* ECPluginSharedData::GetLogger()
{
	return m_lpLogger;
}

IECStatsCollector* ECPluginSharedData::GetStatsCollector()
{
	return m_lpStatsCollector;
}

bool ECPluginSharedData::IsHosted()
{
	return m_bHosted;
}

bool ECPluginSharedData::IsDistributed()
{
	return m_bDistributed;
}

void ECPluginSharedData::Signal(int signal)
{
#ifdef LINUX
	if (!m_lpConfig)
		return;

	switch (signal) {
	case SIGHUP:
		if (!m_lpConfig->ReloadSettings())
			ec_log_crit("Unable to reload plugin configuration file, continuing with current settings.");
			
		if (m_lpConfig->HasErrors()) {
			ec_log_err("Unable to reload plugin configuration file.");
			LogConfigErrors(m_lpConfig);
		}
		break;
	default:
		break;
	}
#endif
}

