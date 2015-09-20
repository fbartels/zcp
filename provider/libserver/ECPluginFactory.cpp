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

#include <cstring>
#include <iostream>
#include <cerrno>
#include <climits>

#include "ECPluginFactory.h"
#include <zarafa/ECConfig.h>
#include <zarafa/ECLogger.h>
#include "ecversion.h"

#ifdef EMBEDDED_USERPLUGIN
	#include "DBUserPlugin.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

ECPluginFactory::ECPluginFactory(ECConfig *config, ECLogger *logger, IECStatsCollector *lpStatsCollector, bool bHosted, bool bDistributed) {
	m_getUserPluginInstance = NULL;
	m_deleteUserPluginInstance = NULL;
	m_config = config;
	m_logger = logger;
	pthread_mutex_init(&m_plugin_lock, NULL);

	ECPluginSharedData::GetSingleton(&m_shareddata, m_config, m_logger, lpStatsCollector, bHosted, bDistributed);

	m_dl = NULL;
}

ECPluginFactory::~ECPluginFactory() {
#ifndef VALGRIND
	if(m_dl)
		dlclose(m_dl);
#endif
	pthread_mutex_destroy(&m_plugin_lock);

	if (m_shareddata)
		m_shareddata->Release();
}

ECRESULT ECPluginFactory::CreateUserPlugin(UserPlugin **lppPlugin) {
    UserPlugin *lpPlugin = NULL;

#ifdef EMBEDDED_USERPLUGIN
	m_getUserPluginInstance = (UserPlugin* (*)(pthread_mutex_t*, ECPluginSharedData *)) getUserPluginInstance;
#else
    if(m_dl == NULL) {    
        const char *pluginpath = m_config->GetSetting("plugin_path");
        const char *pluginname = m_config->GetSetting("user_plugin");
        char filename[PATH_MAX + 1];

        if (!pluginpath || !strcmp(pluginpath, "")) {
            pluginpath = "";
        }
        if (!pluginname || !strcmp(pluginname, "")) {
            m_logger->Log(EC_LOGLEVEL_FATAL, "User plugin is unavailable.");
            m_logger->Log(EC_LOGLEVEL_FATAL, "Please correct your configuration file and set the 'plugin_path' and 'user_plugin' options.");
            return ZARAFA_E_NOT_FOUND;
        }

        snprintf(filename, PATH_MAX + 1, "%s%c%splugin.%s", 
                 pluginpath, PATH_SEPARATOR, pluginname, SHARED_OBJECT_EXTENSION);
        
        m_dl = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);

        if (!m_dl) {
            m_logger->Log(EC_LOGLEVEL_FATAL, "Failed to load %s: %s", filename, dlerror());
            m_logger->Log(EC_LOGLEVEL_FATAL, "Please correct your configuration file and set the 'plugin_path' and 'user_plugin' options.");
            goto out;
        }

        int (*fngetUserPluginInstance)() = (int (*)()) dlsym(m_dl, "getUserPluginVersion");
        if (fngetUserPluginInstance == NULL) {
            m_logger->Log(EC_LOGLEVEL_FATAL, "Failed to load getUserPluginVersion from plugin: %s", dlerror());
            goto out;
        }
        int version = fngetUserPluginInstance(); 
        if (version != PROJECT_VERSION_REVISION) {
		m_logger->Log(EC_LOGLEVEL_FATAL, "Version of the plugin %s is not the same for the server. Expected %d, plugin %d", filename, PROJECT_VERSION_REVISION, version);
		goto out;
	}
    
        m_getUserPluginInstance = (UserPlugin* (*)(pthread_mutex_t *, ECPluginSharedData *)) dlsym(m_dl, "getUserPluginInstance");
        if (m_getUserPluginInstance == NULL) {
            m_logger->Log(EC_LOGLEVEL_FATAL, "Failed to load getUserPluginInstance from plugin: %s", dlerror());
            goto out;
        }

        m_deleteUserPluginInstance = (void (*)(UserPlugin *)) dlsym(m_dl, "deleteUserPluginInstance");
        if (m_deleteUserPluginInstance == NULL) {
            m_logger->Log(EC_LOGLEVEL_FATAL, "Failed to load deleteUserPluginInstance from plugin: %s", dlerror());
            goto out;
        }
    }
#endif
	try {
		lpPlugin = m_getUserPluginInstance(&m_plugin_lock, m_shareddata);
		lpPlugin->InitPlugin();
	}
	catch (exception &e) {
		m_logger->Log(EC_LOGLEVEL_FATAL, "Cannot instantiate user plugin: %s", e.what());
		return ZARAFA_E_NOT_FOUND;
	}
	
	*lppPlugin = lpPlugin;

	return erSuccess;

 out:
	if (m_dl)
		dlclose(m_dl);
	m_dl = NULL;
	return ZARAFA_E_NOT_FOUND;
}

void ECPluginFactory::SignalPlugins(int signal)
{
	m_shareddata->Signal(signal);
}

// Returns a plugin local to this thread. Works the same as GetThreadLocalDatabase
extern pthread_key_t plugin_key;
ECRESULT GetThreadLocalPlugin(ECPluginFactory *lpPluginFactory, UserPlugin **lppPlugin, ECLogger *lpLogger)
{
	ECRESULT er = erSuccess;
	UserPlugin *lpPlugin = NULL;

	lpPlugin = (UserPlugin *)pthread_getspecific(plugin_key);

	if (lpPlugin == NULL) {
		er = lpPluginFactory->CreateUserPlugin(&lpPlugin);

		if (er != erSuccess) {
			lpPlugin = NULL;
			lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to instantiate user plugin");
			goto exit;
		}

		pthread_setspecific(plugin_key, (void *)lpPlugin);
	}

	*lppPlugin = lpPlugin;

exit:
	return er;
}
