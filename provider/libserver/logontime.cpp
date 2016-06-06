/*
 *	Logon/logoff time manager and asynchronous updater
 */
#include <map>
#include <cstdio>
#include <pthread.h>
#include <zarafa/ZarafaCode.h>
#include "logontime.hpp"
#include "ECDatabaseMySQL.h"
#include "ECSecurity.h"
#include "ECSession.h"
#include "edkmdb.h"

namespace kcsrv {

struct ltm_static {
	pthread_mutex_t ontime_mutex, offtime_mutex;
	ltm_static(void) {
		pthread_mutex_init(&ontime_mutex, NULL);
		pthread_mutex_init(&offtime_mutex, NULL);
	}
};

static std::map<unsigned int, time_t> ltm_ontime_cache, ltm_offtime_cache;
static struct ltm_static ltm_static;

static ECRESULT ltm_sync_time(ECDatabase *db,
    const std::pair<unsigned int, time_t> &e, bool dir)
{
	FILETIME ft;
	UnixTimeToFileTime(e.second, &ft);
	std::string query = "SELECT hierarchy_id FROM stores WHERE stores.user_id=" + stringify(e.first);
	DB_RESULT result = NULL;
	ECRESULT ret = db->DoSelect(query, &result);
	if (ret != erSuccess)
		return ret;
	if (db->GetNumRows(result) == 0) {
		db->FreeResult(result);
		return erSuccess;
	}
	DB_ROW row = db->FetchRow(result);
	if (row == NULL) {
		db->FreeResult(result);
		return erSuccess;
	}
	unsigned int store_id = strtoul(row[0], NULL, 0);
	unsigned int prop = dir ? PR_LAST_LOGON_TIME : PR_LAST_LOGOFF_TIME;
	db->FreeResult(result);
	query = "REPLACE INTO properties (tag, type, hierarchyid, val_hi, val_lo) VALUES(" +
                stringify(PROP_ID(prop)) + "," + stringify(PROP_TYPE(prop)) + "," +
                stringify(store_id) + "," + stringify(ft.dwHighDateTime) + "," +
                stringify(ft.dwLowDateTime) + ")";
	return db->DoInsert(query);
}

void sync_logon_times(ECDatabase *db)
{
	/*
	 * Switchgrab the global map, so that we can run it to the database
	 * without holdings locks.
	 */
	bool failed = false;
	pthread_mutex_lock(&ltm_static.ontime_mutex);
	__typeof__(ltm_ontime_cache) logon_time;
	std::swap(logon_time, ltm_ontime_cache);
	pthread_mutex_unlock(&ltm_static.ontime_mutex);
	pthread_mutex_lock(&ltm_static.offtime_mutex);
	__typeof__(ltm_offtime_cache) logoff_time;
	std::swap(logoff_time, ltm_offtime_cache);
	pthread_mutex_unlock(&ltm_static.offtime_mutex);

	for (std::map<unsigned int, time_t>::const_iterator i = logon_time.begin();
	     i != logon_time.end(); ++i)
		failed |= ltm_sync_time(db, *i, 0) != erSuccess;
	for (std::map<unsigned int, time_t>::const_iterator i = logoff_time.begin();
	     i != logoff_time.end(); ++i)
		failed |= ltm_sync_time(db, *i, 1) != erSuccess;
}

/*
 * Save the current time as the last logon time for the logged-on user of
 * @ses.
 */
void record_logon_time(ECSession *ses, bool logon)
{
	unsigned int uid = ses->GetSecurity()->GetUserId();
	time_t now = time(NULL);
	if (logon) {
		pthread_mutex_lock(&ltm_static.ontime_mutex);
		ltm_ontime_cache[uid] = now;
		pthread_mutex_unlock(&ltm_static.ontime_mutex);
	} else {
		pthread_mutex_lock(&ltm_static.offtime_mutex);
		ltm_offtime_cache[uid] = now;
		pthread_mutex_unlock(&ltm_static.offtime_mutex);
	}
}

} /* namespace kcsrv */
