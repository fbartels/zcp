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

#include <zarafa/platform.h>
#include <fcntl.h>
#include <mapidefs.h>
#include <mapicode.h>
#include <climits>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/time.h> /* gettimeofday */

#include "TmpPath.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

HRESULT UnixTimeToFileTime(time_t t, FILETIME *ft)
{
    __int64 l;

    l = (__int64)t * 10000000 + NANOSECS_BETWEEN_EPOCHS;
    ft->dwLowDateTime = (unsigned int)l;
    ft->dwHighDateTime = (unsigned int)(l >> 32);

	return hrSuccess;
}

HRESULT FileTimeToUnixTime(const FILETIME &ft, time_t *t)
{
	__int64 l;

	l = ((__int64)ft.dwHighDateTime << 32) + ft.dwLowDateTime;
	l -= NANOSECS_BETWEEN_EPOCHS;
	l /= 10000000;
	
	if(sizeof(time_t) < 8) {
		// On 32-bit systems, we cap the values at MAXINT and MININT
		if(l < (__int64)INT_MIN) {
			l = INT_MIN;
		}
		if(l > (__int64)INT_MAX) {
			l = INT_MAX;
		}
	}

	*t = (time_t)l;

	return hrSuccess;
}

void UnixTimeToFileTime(time_t t, int *hi, unsigned int *lo)
{
	__int64 ll;

	ll = (__int64)t * 10000000 + NANOSECS_BETWEEN_EPOCHS;
	*lo = (unsigned int)ll;
	*hi = (unsigned int)(ll >> 32);
}

time_t FileTimeToUnixTime(unsigned int hi, unsigned int lo)
{
	time_t t = 0;
	FILETIME ft;
	ft.dwHighDateTime = hi;
	ft.dwLowDateTime = lo;
	
	if(FileTimeToUnixTime(ft, &t) != hrSuccess)
		return 0;
	
	return t;
}

static const LONGLONG UnitsPerMinute = 600000000;
static const LONGLONG UnitsPerHalfMinute = 300000000;

void RTimeToFileTime(LONG rtime, FILETIME *pft)
{
	// ASSERT(pft != NULL);
	ULONGLONG q = rtime;
	q *= UnitsPerMinute;
	pft->dwLowDateTime  = q & 0xFFFFFFFF;
	pft->dwHighDateTime = q >> 32;
}
 
void FileTimeToRTime(const FILETIME *pft, LONG *prtime)
{
	// ASSERT(pft != NULL);
	// ASSERT(prtime != NULL);

	ULONGLONG q = pft->dwHighDateTime;
	q <<= 32;
	q |= pft->dwLowDateTime;

	q += UnitsPerHalfMinute;
	q /= UnitsPerMinute;
	*prtime = q & 0x7FFFFFFF;
}

HRESULT RTimeToUnixTime(LONG rtime, time_t *unixtime)
{
	FILETIME ft;

	if (unixtime == NULL)
		return MAPI_E_INVALID_PARAMETER;
	RTimeToFileTime(rtime, &ft);
	FileTimeToUnixTime(ft, unixtime);
	return hrSuccess;
}

HRESULT UnixTimeToRTime(time_t unixtime, LONG *rtime)
{
	FILETIME ft;

	if (rtime == NULL)
		return MAPI_E_INVALID_PARAMETER;
	UnixTimeToFileTime(unixtime, &ft);
	FileTimeToRTime(&ft, rtime);
	return hrSuccess;
}

// time only, not date!
time_t SystemTimeToUnixTime(SYSTEMTIME stime)
{
	return stime.wSecond + (stime.wMinute*60) + ((stime.wHour)*60*60);
}

SYSTEMTIME UnixTimeToSystemTime(time_t unixtime)
{
	SYSTEMTIME stime = {0};
	stime.wSecond = unixtime%60;
	unixtime /= 60;
	stime.wMinute = unixtime%60;
	unixtime /= 60;
	stime.wHour = unixtime;
	return stime;
}

SYSTEMTIME TMToSystemTime(struct tm t)
{
	SYSTEMTIME stime = {0};
	stime.wYear = t.tm_year;
	stime.wMonth = t.tm_mon;
	stime.wDayOfWeek = t.tm_wday;
	stime.wDay = t.tm_mday;
	stime.wHour = t.tm_hour;
	stime.wMinute = t.tm_min;
	stime.wSecond = t.tm_sec;
	stime.wMilliseconds = 0;
	return stime;	
}

struct tm SystemTimeToTM(SYSTEMTIME stime)
{
	// not quite, since we miss tm_yday
	struct tm t = {0};
	t.tm_year = stime.wYear;
	t.tm_mon = stime.wMonth;
	t.tm_wday = stime.wDayOfWeek;
	t.tm_mday = stime.wDay;
	t.tm_hour = stime.wHour;
	t.tm_min = stime.wMinute;
	t.tm_sec = stime.wSecond;
	t.tm_isdst = -1;
	return t;	
}

/* The 'IntDate' and 'IntTime' date and time encoding are used for some CDO calculations. They
 * are basically a date or time encoded in a bitshifted way, packed so that it uses the least amount
 * of bits. Eg. a date (day,month,year) is encoded as 5 bits for the day (1-31), 4 bits for the month (1-12),
 * and the rest of the bits (32-4-5 = 23) for the year. The same goes for time, with seconds and minutes
 * each using 6 bits and 32-6-6=20 bits for the hours.
 *
 * For dates, everything is 1-index (1st January is 1-1) and year is full (2008)
 */
 
/**
 * Create INT date
 *
 * @param[in] day Day of month 1-31
 * @param[in] month Month of year 1-12
 * @param[in] year Full year (eg 2008)
 * @return ULONG Calculated INT date
 */
ULONG   CreateIntDate(ULONG day, ULONG month, ULONG year)
{
	return day + month * 32 + year * 32 * 16;
}

/**
 * Create INT time
 *
 * @param[in] seconds Seconds 0-59
 * @param[in] minutes Minutes 0-59
 * @param[in] hours Hours
 * @return Calculated INT time
 */
ULONG   CreateIntTime(ULONG seconds, ULONG minutes, ULONG hours)
{
	return seconds + minutes * 64 + hours * 64 * 64;
}

/**
 * Create INT date from filetime
 *
 * Discards time information from the passed FILETIME stamp, and returns the date
 * part as an INT date. The passed FILETIME is interpreted in GMT.
 *
 * @param[in] ft FileTime to convert
 * @return Converted DATE part of the file time.
 */
ULONG FileTimeToIntDate(FILETIME &ft)
{
	struct tm date;
	time_t t;
	FileTimeToUnixTime(ft, &t);
	gmtime_safe(&t, &date);
	
	return CreateIntDate(date.tm_mday, date.tm_mon+1, date.tm_year+1900);
}

/**
 * Create INT time from offset in seconds
 *
 * Creates an INT time value for the moment at which the passed amount of seconds
 * has passed on a day.
 *
 * @param[in] seconds Number of seconds since beginning of day
 * @return Converted INT time
 */
ULONG SecondsToIntTime(ULONG seconds)
{
	ULONG hours = seconds / (60*60);
	seconds -= hours * 60 * 60;
	ULONG minutes = seconds / 60;
	seconds -= minutes * 60;
	
	return CreateIntTime(seconds, minutes, hours);
}

bool operator ==(const FILETIME &a, const FILETIME &b)
{
	return a.dwLowDateTime == b.dwLowDateTime && a.dwHighDateTime == b.dwHighDateTime;
}

bool operator >(const FILETIME &a, const FILETIME &b)
{
	return ((a.dwHighDateTime > b.dwHighDateTime) ||
		((a.dwHighDateTime == b.dwHighDateTime) &&
		 (a.dwLowDateTime > b.dwLowDateTime)));
}

bool operator >=(const FILETIME &a, const FILETIME &b)
{
	return a > b || a == b;
}

bool operator <(const FILETIME &a, const FILETIME &b)
{
	return ((a.dwHighDateTime < b.dwHighDateTime) ||
		((a.dwHighDateTime == b.dwHighDateTime) &&
		 (a.dwLowDateTime < b.dwLowDateTime)));
}

bool operator <=(const FILETIME &a, const FILETIME &b)
{
	return a < b || a == b;
}

time_t operator -(const FILETIME &a, const FILETIME &b)
{
	time_t aa, bb;

	FileTimeToUnixTime(a, &aa);
	FileTimeToUnixTime(b, &bb);

	return aa - bb;
}

#ifndef HAVE_TIMEGM
time_t timegm(struct tm *t) {
	time_t convert;
	char *tz = NULL;
	char *s_tz = NULL;

	tz = getenv("TZ");
	if(tz)
		s_tz = strdup(tz);

	// SuSE 9.1 segfaults when putenv() is used in a detached thread on the next getenv() call.
	// so use setenv() on linux, putenv() on others.

#ifdef LINUX
	setenv("TZ", "UTC0", 1);
	tzset();
#else
	if(putenv("TZ=UTC0") != -1)
		_tzset();
#endif

	convert = mktime(t);

#ifdef LINUX
	if (s_tz) {
		setenv("TZ", s_tz, 1);
		tzset();
	} else {
		unsetenv("TZ");
		tzset();
	}
#else
	if (s_tz) {
		putenv(s_tz);
		_tzset();
	} else {
		putenv("TZ=");
		_tzset();
	}
#endif
	free(s_tz);
	return convert;
}
#endif

struct tm* gmtime_safe(const time_t* timer, struct tm *result)
{
	struct tm *tmp = NULL;
	tmp = gmtime_r(timer, result);
	if(tmp == NULL)
		memset(result, 0, sizeof(struct tm));

	return tmp;
}

double timespec2dbl(const struct timespec &t)
{
    return (double)t.tv_sec + t.tv_nsec/1000000000.0;
}

struct timespec GetDeadline(unsigned int ulTimeoutMs)
{
	struct timespec	deadline;
	struct timeval	now;
	gettimeofday(&now, NULL);

	now.tv_sec += ulTimeoutMs / 1000;
	now.tv_usec += 1000 * (ulTimeoutMs % 1000);
	if (now.tv_usec >= 1000000) {
		++now.tv_sec;
		now.tv_usec -= 1000000;
	}

	deadline.tv_sec = now.tv_sec;
	deadline.tv_nsec = now.tv_usec * 1000;

	return deadline;
}

// Does mkdir -p <path>
int CreatePath(const char *createpath)
{
	struct stat s;
	char *path = strdup(createpath);

	// Remove trailing slashes
	size_t len = strlen(path);
	while (len > 0 && (path[len-1] == '/' || path[len-1] == '\\'))
		path[--len] = 0;

	if(stat(path, &s) == 0) {
		if(s.st_mode & S_IFDIR) {
			free(path);
			return 0; // Directory is already there
		} else {
			free(path);
			return -1; // Item is not a directory
		}
	} else {
		// We need to create the directory

		// First, create parent directories
		char *trail = strrchr(path, '/') > strrchr(path, '\\') ? strrchr(path, '/') : strrchr(path, '\\');

		if(!trail) {
		    // Should only happen if you're trying to create /path/to/dir in win32
		    // or \path\to\dir in linux
		    free(path);
		    return -1;
		}
		
		*trail = 0;

		if(CreatePath(path) != 0) {
			free(path);
			return -1;
		}

		// Create the actual directory
		int ret = CreateDir(createpath, 0700);
		free(path);
		return ret;
	}
}

double GetTimeOfDay()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (double)tv.tv_sec + ((double)tv.tv_usec / 1000000); // usec = microsec = 1 millionth of a second
}

void set_thread_name(pthread_t tid, const std::string & name)
{
#ifdef HAVE_PTHREAD_SETNAME_NP_2
	if (name.size() > 15)
		pthread_setname_np(tid, name.substr(0, 15).c_str());
	else
		pthread_setname_np(tid, name.c_str());
#endif
}

ssize_t read_retry(int fd, void *data, size_t len)
{
	char *buf = static_cast<char *>(data);
	size_t tread = 0;

	while (len > 0) {
		ssize_t ret = read(fd, buf, len);
		if (ret < 0 && (errno == EINTR || errno == EAGAIN))
			continue;
		if (ret < 0)
			return ret;
		if (ret == 0)
			break;
		len -= ret;
		buf += ret;
		tread += ret;
	}
	return tread;
}

ssize_t write_retry(int fd, const void *data, size_t len)
{
	const char *buf = static_cast<const char *>(data);
	size_t twrote = 0;

	while (len > 0) {
		ssize_t ret = write(fd, buf, len);
		if (ret < 0 && (errno == EINTR || errno == EAGAIN))
			continue;
		if (ret < 0)
			return ret;
		if (ret == 0)
			break;
		len -= ret;
		buf += ret;
		twrote += ret;
	}
	return twrote;
}

bool force_buffers_to_disk(const int fd)
{
	if (fsync(fd) == -1)
	    return false;
	return true;
}

void my_readahead(const int fd)
{
#ifndef WIN32
	struct stat st;

	if (fstat(fd, &st) == 0)
		(void)readahead(fd, 0, st.st_size);
#endif
}

void give_filesize_hint(const int fd, const off_t len)
{
#ifndef WIN32
	// this helps preventing filesystem fragmentation as the
	// kernel can now look for the best disk allocation
	// pattern as it knows how much date is going to be
	// inserted
	posix_fallocate(fd, 0, len);
#endif
}
