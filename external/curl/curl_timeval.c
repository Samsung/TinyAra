/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2017, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "curl_timeval.h"

#if defined(WIN32) && !defined(MSDOS)

struct curltime Curl_now(void)
{
  /*
  ** GetTickCount() is available on _all_ Windows versions from W95 up
  ** to nowadays. Returns milliseconds elapsed since last system boot,
  ** increases monotonically and wraps once 49.7 days have elapsed.
  */
  struct curltime now;
#if !defined(_WIN32_WINNT) || !defined(_WIN32_WINNT_VISTA) || \
    (_WIN32_WINNT < _WIN32_WINNT_VISTA)
  DWORD milliseconds = GetTickCount();
  now.tv_sec = milliseconds / 1000;
  now.tv_usec = (milliseconds % 1000) * 1000;
#else
  ULONGLONG milliseconds = GetTickCount64();
  now.tv_sec = (time_t) (milliseconds / 1000);
  now.tv_usec = (unsigned int) (milliseconds % 1000) * 1000;
#endif

  return now;
}

#elif defined(HAVE_CLOCK_GETTIME_MONOTONIC)

struct curltime Curl_now(void)
{
  /*
  ** clock_gettime() is granted to be increased monotonically when the
  ** monotonic clock is queried. Time starting point is unspecified, it
  ** could be the system start-up time, the Epoch, or something else,
  ** in any case the time starting point does not change once that the
  ** system has started up.
  */
  struct timeval now;
  struct curltime cnow;
  struct timespec tsnow;
  if(0 == clock_gettime(CLOCK_MONOTONIC, &tsnow)) {
    cnow.tv_sec = tsnow.tv_sec;
    cnow.tv_usec = (unsigned int)(tsnow.tv_nsec / 1000);
  }
  /*
  ** Even when the configure process has truly detected monotonic clock
  ** availability, it might happen that it is not actually available at
  ** run-time. When this occurs simply fallback to other time source.
  */
#ifdef HAVE_GETTIMEOFDAY
  else {
    (void)gettimeofday(&now, NULL);
    cnow.tv_sec = now.tv_sec;
    cnow.tv_usec = (unsigned int)now.tv_usec;
  }
#else
  else {
    cnow.tv_sec = time(NULL);
    cnow.tv_usec = 0;
  }
#endif
  return cnow;
}

#elif defined(HAVE_MACH_ABSOLUTE_TIME)

#include <stdint.h>
#include <mach/mach_time.h>

struct curltime Curl_now(void)
{
  /*
  ** Monotonic timer on Mac OS is provided by mach_absolute_time(), which
  ** returns time in Mach "absolute time units," which are platform-dependent.
  ** To convert to nanoseconds, one must use conversion factors specified by
  ** mach_timebase_info().
  */
  static mach_timebase_info_data_t timebase;
  struct curltime cnow;
  uint64_t usecs;

  if(0 == timebase.denom)
    (void) mach_timebase_info(&timebase);

  usecs = mach_absolute_time();
  usecs *= timebase.numer;
  usecs /= timebase.denom;
  usecs /= 1000;

  cnow.tv_sec = usecs / 1000000;
  cnow.tv_usec = usecs % 1000000;

  return cnow;
}

#elif defined(HAVE_GETTIMEOFDAY)

struct curltime Curl_now(void)
{
  /*
  ** gettimeofday() is not granted to be increased monotonically, due to
  ** clock drifting and external source time synchronization it can jump
  ** forward or backward in time.
  */
  struct timeval now;
  struct curltime ret;
  (void)gettimeofday(&now, NULL);
  ret.tv_sec = now.tv_sec;
  ret.tv_usec = now.tv_usec;
  return ret;
}

#else

struct curltime Curl_now(void)
{
  /*
  ** time() returns the value of time in seconds since the Epoch.
  */
  struct curltime now;
  now.tv_sec = time(NULL);
  now.tv_usec = 0;
  return now;
}

#endif

#if SIZEOF_TIME_T < 8
#define TIME_MAX INT_MAX
#define TIME_MIN INT_MIN
#else
#define TIME_MAX 9223372036854775807LL
#define TIME_MIN -9223372036854775807LL
#endif

/*
 * Returns: time difference in number of milliseconds. For too large diffs it
 * returns max value.
 *
 * @unittest: 1323
 */
timediff_t Curl_timediff(struct curltime newer, struct curltime older)
{
  timediff_t diff = newer.tv_sec-older.tv_sec;
  if(diff >= (TIME_MAX/1000))
    return TIME_MAX;
  else if(diff <= (TIME_MIN/1000))
    return TIME_MIN;
  return diff * 1000 + (newer.tv_usec-older.tv_usec)/1000;
}

/*
 * Returns: time difference in number of microseconds. For too large diffs it
 * returns max value.
 */
timediff_t Curl_timediff_us(struct curltime newer, struct curltime older)
{
  timediff_t diff = newer.tv_sec-older.tv_sec;
  if(diff >= (TIME_MAX/1000000))
    return TIME_MAX;
  else if(diff <= (TIME_MIN/1000000))
    return TIME_MIN;
  return diff * 1000000 + newer.tv_usec-older.tv_usec;
}
