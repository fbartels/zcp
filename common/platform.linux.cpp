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
#include <zarafa/platform.h>
#include <zarafa/ECLogger.h>

#include <sys/select.h>
#include <sys/time.h>
#include <iconv.h>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <ctime>
#include <dirent.h>
#include <mapicode.h>			// return codes
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cstdlib>
#include <cerrno>
#include <climits>

#include <string>
#include <map>
#include <vector>

#ifndef HAVE_UUID_CREATE
#	include <uuid/uuid.h>
#else
#	include <uuid.h>
#endif
#if defined(__linux__) && defined(__GLIBC__)
#	include <cxxabi.h>
#	include <execinfo.h>
#endif
#include "TmpPath.h"

#ifdef __APPLE__
// bsd
#define ICONV_CONST const
#elif OPENBSD
// bsd
#define ICONV_CONST const
#else
// linux
#define ICONV_CONST
#endif

static bool rand_init_done = false;

bool operator!=(const GUID &a, const GUID &b)
{
	return memcmp(&a, &b, sizeof(GUID)) != 0;
}

bool operator==(REFIID a, const GUID &b)
{
	return memcmp(&a, &b, sizeof(GUID)) == 0;
}

HRESULT CoCreateGuid(LPGUID pNewGUID) {
	if (!pNewGUID)
		return MAPI_E_INVALID_PARAMETER;

#if HAVE_UUID_CREATE
#ifdef OPENBSD
	uuid_t *g = NULL;
	void *vp = NULL;
	size_t n = 0;
	// error codes are not checked!
	uuid_create(&g);
	uuid_make(g, UUID_MAKE_V1);
	uuid_export(g, UUID_FMT_BIN, &vp, &n);
	memcpy(pNewGUID, &vp, UUID_LEN_BIN);
	uuid_destroy(g);
#else
	uuid_t g;
	uint32_t uid_ret;
	uuid_create(&g, &uid_ret);
	memcpy(pNewGUID, &g, sizeof(g));
#endif // OPENBSD
#else
	uuid_t g;
	uuid_generate(g);
	memcpy(pNewGUID, g, sizeof(g));
#endif

	return S_OK;
}

void strupr(char* a) {
	while (*a != '\0') {
		*a = toupper (*a);
		++a;
	}
}

__int64_t Int32x32To64(ULONG a, ULONG b) {
	return (__int64_t)a*(__int64_t)b;
}

void GetSystemTimeAsFileTime(FILETIME *ft) {
	struct timeval now;
	__int64_t l;
	gettimeofday(&now,NULL); // null==timezone
	l = ((__int64_t)now.tv_sec * 10000000) + ((__int64_t)now.tv_usec * 10) + (__int64_t)NANOSECS_BETWEEN_EPOCHS;
	ft->dwLowDateTime = (unsigned int)(l & 0xffffffff);
	ft->dwHighDateTime = l >> 32;
}

/** 
 * copies the path of the temp directory, including trailing /, into
 * given buffer.
 * 
 * @param[in] inLen size of buffer, inclusive \0 char
 * @param[in,out] lpBuffer buffer to place path in
 * 
 * @return length used or what would've been required if it would fit in lpBuffer
 */
DWORD GetTempPath(DWORD inLen, char *lpBuffer) {
	unsigned int outLen = snprintf(lpBuffer, inLen, "%s/", TmpPath::getInstance() -> getTempPath().c_str());

	if (outLen > inLen)
		return 0;

	return outLen;
}

void Sleep(unsigned int msec) {
	struct timespec ts;
	unsigned int rsec;
	ts.tv_sec = msec/1000;
	rsec = msec - (ts.tv_sec*1000);
	ts.tv_nsec = rsec*1000*1000;
	nanosleep(&ts, NULL);
}

static void rand_fail(void)
{
	fprintf(stderr, "Cannot access/use /dev/urandom, this is fatal (%s)\n", strerror(errno));
	kill(0, SIGTERM);
	exit(1);
}
	
void rand_get(char *p, int n)
{
	int fd = open("/dev/urandom", O_RDONLY);

	if (fd == -1)
		rand_fail();
	
	// handle EINTR
	while(n > 0)
	{
		int rc = read(fd, p, n);

		if (rc == 0)
			rand_fail();

		if (rc == -1)
		{
			if (errno == EINTR)
				continue;

			rand_fail();
		}

		p += rc;
		n -= rc;
	}

		close(fd);
	}
	
void rand_init() {
	unsigned int seed = 0;
	rand_get((char *)&seed, sizeof(seed));
	srand(seed);

	rand_init_done = true;
}

int rand_mt() {
	int dummy = 0;
	rand_get((char *)&dummy, sizeof dummy);

	if (dummy == INT_MIN)
		dummy = INT_MAX;
	else
		dummy = abs(dummy);

	// this gives a slighly bias to the value 0
	// also RAND_MAX is never returned which the
	// regular rand() does do
	return dummy % RAND_MAX;
}

void rand_free() {
	//Nothing to free
}

char * get_password(const char *prompt) {
	return getpass(prompt);
}

HGLOBAL GlobalAlloc(UINT uFlags, ULONG ulSize)
{
	// always returns NULL, as required by CreateStreamOnHGlobal implementation in mapi4linux/src/mapiutil.cpp
	return NULL;
}

time_t GetProcessTime()
{
	time_t t;

	time(&t);

	return t;
}

#if DEBUG_PTHREADS

class Lock _zcp_final {
public:
	Lock() : locks(0), busy(0), dblTime(0) {}
       ~Lock() {};

       std::string strLocation;
       unsigned int locks;
       unsigned int busy;
       double dblTime;
};

static std::map<std::string, Lock> my_pthread_map;
static pthread_mutex_t my_mutex;
static int init = 0;

#undef pthread_mutex_lock
int my_pthread_mutex_lock(const char *file, unsigned int line, pthread_mutex_t *__mutex)
{
       char s[1024];
       snprintf(s, sizeof(s), "%s:%d", file, line);
       double dblTime;
       int err = 0;

       if(!init) {
               init = 1;
               pthread_mutex_init(&my_mutex, NULL);
       }

       pthread_mutex_lock(&my_mutex);
       my_pthread_map[s].strLocation = s;
       ++my_pthread_map[s].locks;
       pthread_mutex_unlock(&my_mutex);

       if(( err = pthread_mutex_trylock(__mutex)) == EBUSY) {
               pthread_mutex_lock(&my_mutex);
               ++my_pthread_map[s].busy;
               pthread_mutex_unlock(&my_mutex);
               dblTime = GetTimeOfDay();
               err = pthread_mutex_lock(__mutex);
               pthread_mutex_lock(&my_mutex);
               my_pthread_map[s].dblTime += GetTimeOfDay() - dblTime;
               pthread_mutex_unlock(&my_mutex);
       }

       return err;
}

#undef pthread_rwlock_rdlock
int my_pthread_rwlock_rdlock(const char *file, unsigned int line, pthread_rwlock_t *__mutex)
{
       char s[1024];
       snprintf(s, sizeof(s), "%s:%d", file, line);
       double dblTime;
       int err = 0;

       if(!init) {
               init = 1;
               pthread_mutex_init(&my_mutex, NULL);
       }

       pthread_mutex_lock(&my_mutex);
       my_pthread_map[s].strLocation = s;
       ++my_pthread_map[s].locks;
       pthread_mutex_unlock(&my_mutex);

       if(( err = pthread_rwlock_tryrdlock(__mutex)) == EBUSY) {
               pthread_mutex_lock(&my_mutex);
               ++my_pthread_map[s].busy;
               pthread_mutex_unlock(&my_mutex);
               dblTime = GetTimeOfDay();
               err = pthread_rwlock_rdlock(__mutex);
               pthread_mutex_lock(&my_mutex);
               my_pthread_map[s].dblTime += GetTimeOfDay() - dblTime;
               pthread_mutex_unlock(&my_mutex);
       }

       return err;
}

#undef pthread_rwlock_wrlock
int my_pthread_rwlock_wrlock(const char *file, unsigned int line, pthread_rwlock_t *__mutex)
{
       char s[1024];
       snprintf(s, sizeof(s), "%s:%d", file, line);
       double dblTime;
       int err = 0;

       if(!init) {
               init = 1;
               pthread_mutex_init(&my_mutex, NULL);
       }

       pthread_mutex_lock(&my_mutex);
       my_pthread_map[s].strLocation = s;
       ++my_pthread_map[s].locks;
       pthread_mutex_unlock(&my_mutex);

       if(( err = pthread_rwlock_trywrlock(__mutex)) == EBUSY) {
               pthread_mutex_lock(&my_mutex);
               ++my_pthread_map[s].busy;
               pthread_mutex_unlock(&my_mutex);
               dblTime = GetTimeOfDay();
               err = pthread_rwlock_wrlock(__mutex);
               pthread_mutex_lock(&my_mutex);
               my_pthread_map[s].dblTime += GetTimeOfDay() - dblTime;
               pthread_mutex_unlock(&my_mutex);
       }

       return err;
}

std::string dump_pthread_locks()
{
	std::map<std::string, Lock>::const_iterator i;
       std::string strLog;
       char s[2048];

       for (i = my_pthread_map.begin(); i!= my_pthread_map.end(); ++i) {
               snprintf(s,sizeof(s), "%s\t\t%d\t\t%d\t\t%f\n", i->second.strLocation.c_str(), i->second.locks, i->second.busy, (float)i->second.dblTime);
               strLog += s;
       }

       return strLog;
}
#endif

std::vector<std::string> get_backtrace(void)
{
#define BT_MAX 256
	std::vector<std::string> result;
	void *addrlist[BT_MAX];
	int addrlen = backtrace(addrlist, BT_MAX);
	if (addrlen == 0)
		return result;
	char **symbollist = backtrace_symbols(addrlist, addrlen);
	for (int i = 0; i < addrlen; ++i)
		result.push_back(symbollist[i]);
	free(symbollist);
	return result;
#undef BT_MAX
}

static void dump_fdtable_summary(pid_t pid)
{
	char procdir[64];
	snprintf(procdir, sizeof(procdir), "/proc/%ld/fd", static_cast<long>(pid));
	DIR *dh = opendir(procdir);
	if (dh == NULL)
		return;
	std::string msg;
	struct dirent de_space, *de = NULL;
	while (readdir_r(dh, &de_space, &de) == 0 && de != NULL) {
		if (de->d_type != DT_LNK)
			continue;
		std::string de_name(std::string(procdir) + "/" + de->d_name);
		struct stat sb;
		if (stat(de_name.c_str(), &sb) < 0) {
			msg += " ?";
		} else switch (sb.st_mode & S_IFMT) {
			case S_IFREG:  msg += " ."; break;
			case S_IFSOCK: msg += " s"; break;
			case S_IFDIR:  msg += " d"; break;
			case S_IFIFO:  msg += " p"; break;
			case S_IFCHR:  msg += " c"; break;
			default:       msg += " O"; break;
		}
		msg += de->d_name;
	}
	closedir(dh);
	ec_log_debug("FD map:%s", msg.c_str());
}

/* ALERT! Big hack!
 *
 * This function relocates an open file descriptor to a new file descriptor above 1024. The
 * reason we do this is because, although we support many open FDs up to FD_SETSIZE, libraries
 * that we use may not (most notably libldap). This means that if a new socket is allocated within
 * libldap as socket 1025, libldap will fail because it was compiled with FD_SETSIZE=1024. To fix
 * this problem, we make sure that most FDs under 1024 are free for use by external libraries, while
 * we use the range 1024 -> \infty.
 */
int ec_relocate_fd(int fd)
{
	static const int typical_limit = 1024;

	int relocated = fcntl(fd, F_DUPFD, typical_limit);
	if (relocated >= 0) {
		close(fd);
		return relocated;
	}
	if (errno == EINVAL) {
		/*
		 * The range start (typical_limit) was already >=RLIMIT_NOFILE.
		 * Just stay silent.
		 */
		static bool warned_once;
		if (warned_once)
			return fd;
		warned_once = true;
		ec_log_warn("F_DUPFD yielded EINVAL\n");
		return fd;
	}
	static time_t warned_last;
	time_t now = time(NULL);
	if (warned_last + 60 > now)
		return fd;
	ec_log_notice(
		"Relocation of FD %d into high range (%d+) could not be completed: "
		"%s. Keeping old number.\n", fd, typical_limit, strerror(errno));
	dump_fdtable_summary(getpid());
	return fd;
}
