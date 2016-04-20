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
#ifndef ECPLUGINFACTORY_H
#define ECPLUGINFACTORY_H

#include <zarafa/zcdefs.h>
#include <zarafa/ZarafaCode.h>
#include <zarafa/ECConfig.h>
#include <zarafa/ECPluginSharedData.h>
#include "plugin.h"
#include <pthread.h>

class ECPluginFactory _zcp_final {
public:
	ECPluginFactory(ECConfig *config, IECStatsCollector *lpStatsCollector, bool bHosted, bool bDistributed);
	~ECPluginFactory();

	ECRESULT	CreateUserPlugin(UserPlugin **lppPlugin);
	void		SignalPlugins(int signal);

private:
	UserPlugin* (*m_getUserPluginInstance)(pthread_mutex_t*, ECPluginSharedData*);
	void (*m_deleteUserPluginInstance)(UserPlugin*);

	ECPluginSharedData *m_shareddata;
	ECConfig *m_config;
	pthread_mutex_t m_plugin_lock;

	DLIB m_dl;
};

extern ECRESULT GetThreadLocalPlugin(ECPluginFactory *lpPluginFactory, UserPlugin **lppPlugin);
#endif
