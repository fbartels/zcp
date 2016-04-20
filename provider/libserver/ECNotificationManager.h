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

#ifndef ECNOTIFICATIONMANAGER_H
#define ECNOTIFICATIONMANAGER_H

#include <zarafa/zcdefs.h>
#include "ECSession.h"
#include <zarafa/ECLogger.h>
#include <zarafa/ECConfig.h>

#include <map>
#include <set>

/*
 * The notification manager runs in a single thread, servicing notifications to ALL clients
 * that are waiting for a notification. We simply store all waiting soap connection objects together
 * with their getNextNotify() request, and once we are signalled that something has changed for one
 * of those queues, we send the reply, and requeue the soap connection for the next request.
 *
 * So, basically we only handle the SOAP-reply part of the soap request.
 */
 
struct NOTIFREQUEST {
    struct soap *soap;
    time_t ulRequestTime;
};

class ECNotificationManager _zcp_final {
public:
	ECNotificationManager();
	~ECNotificationManager();
    
    // Called by the SOAP handler
    HRESULT AddRequest(ECSESSIONID ecSessionId, struct soap *soap);

    // Called by a session when it has a notification to send
    HRESULT NotifyChange(ECSESSIONID ecSessionId);
    
private:
    // Just a wrapper to Work()
    static void * Thread(void *lpParam);
    void * Work();
    
    bool		m_bExit;
    pthread_t 	m_thread;
    
    unsigned int m_ulTimeout;

    ECLogger *m_lpLogger;

    // A map of all sessions that are waiting for a SOAP response to be sent (an item can be in here for up to 60 seconds)
    std::map<ECSESSIONID, NOTIFREQUEST> 	m_mapRequests;
    // A set of all sessions that have reported notification activity, but are yet to be processed.
    // (a session is in here for only very short periods of time, and contains only a few sessions even if the load is high)
    std::set<ECSESSIONID> 					m_setActiveSessions;
    
    pthread_mutex_t m_mutexRequests;
    pthread_mutex_t m_mutexSessions;
    pthread_cond_t m_condSessions;
};

extern ECSessionManager *g_lpSessionManager;
extern void zarafa_notify_done(struct soap *soap);

#endif
