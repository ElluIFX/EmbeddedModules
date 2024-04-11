/*	TimeLib - Time management library for embedded devices
        Copyright (C) 2014 Jesus Ruben Santa Anna Zamudio.

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        Author website: http://www.geekfactory.mx
        Author e-mail: ruben at geekfactory dot mx
 */
#include "TimeLib.h"

/* Flag used to "freeze" the clock value */
bool halt = false;

/* Stores the day count for each month */
const unsigned char month_length[] = {31, 28, 31, 30, 31, 30,
                                      31, 31, 30, 31, 30, 31};

/* Unix like time counter, keeps track of absolute time */
timelib_t sys_time = 0;

/* The interval in seconds after which the system time variable should be
 * updated (synced) with an external (accurate) time base */
timelib_t sync_interval = 86400;

/* Unix timestamp when the sync should be done. */
timelib_t sync_next = 0;

/* Cache for current time */
timelib_t tcache;
struct timelib_tm telements;

/* Variable used to keep track of the last time the "seconds" or sys_time
 * counter was updated in "tick" units */
timelib_t last_update = 0;

/* Keeps the status of the system time (ok, needs sync, not set, etc). */
enum time_status tstatus = E_TIME_NOT_SET;

/**
 * Stores a pointer to a function that returns a precise Unix timestamp to set
 * the internal clock to the returned timestamp. This update occurs at the
 * interval given when the callback is configured.
 */
timelib_callback_t timelib_provider_callback = 0;

/**
 * @brief Computes if the given year is a leap year
 *
 * @param year The year to check
 *
 * @return Returns true if "year" is leap, false otherwise
 */
static int timelib_is_leap(unsigned int year) {
  // Must be divisible by 4 but not by 100, unless it is divisible by 400
  return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
}

/**
 * Updates the time structure if time has changed
 *
 * @param time The timestamp now.
 */
static void timelib_update(timelib_t time) {
  if (tcache != time) {
    tcache = time;
    timelib_break(time, &telements);
  }
}

/*-------------------------------------------------------------*
 *	Public API, check TimeLib.h for documentation		*
 *-------------------------------------------------------------*/
void timelib_set(timelib_t now) {
  sys_time = now;
  sync_next = now + sync_interval;
  tstatus = E_TIME_OK;
  last_update = tick_get();
}

timelib_t timelib_get() {
  timelib_t now = 0;

  // Clock halted, return always the same value (no update)
  if (halt == true) return sys_time;

  // Check if time needs sync to timebase
  if (sync_next <= sys_time) {
    // Null pointer check
    if (timelib_provider_callback != 0) {
      // Invoke callback function
      now = timelib_provider_callback();
      // Got time from callback?
      if (now != 0) {
        timelib_set(now);
      } else {
        sync_next = sys_time + sync_interval;
        tstatus =
            (tstatus == E_TIME_NOT_SET) ? E_TIME_NOT_SET : E_TIME_NEEDS_SYNC;
      }
    }
  }

  // Check how many seconds have elapsed (if any) since the last call
  // and update the timestamp counter
  timelib_t delta = tick_get() - last_update;
  // Increment timestamp
  sys_time += delta / TICK_SECOND;
  last_update += delta;

  return sys_time;
}

void timelib_halt_clock() { halt = true; }

void timelib_resume_clock() { halt = false; }

uint8_t timelib_get_status() {
  timelib_get();
  return tstatus;
}

uint8_t timelib_second_t(timelib_t time) {
  timelib_update(time);
  return telements.tm_sec;
}

uint8_t timelib_minute_t(timelib_t time) {
  timelib_update(time);
  return telements.tm_min;
}

uint8_t timelib_hour_t(timelib_t time) {
  timelib_update(time);
  return telements.tm_hour;
}

uint8_t timelib_wday_t(timelib_t time) {
  timelib_update(time);
  return telements.tm_wday;
}

uint8_t timelib_day_t(timelib_t time) {
  timelib_update(time);
  return telements.tm_mday;
}

uint8_t timelib_month_t(timelib_t time) {
  timelib_update(time);
  return telements.tm_mon;
}

uint16_t timelib_year_t(timelib_t time) {
  timelib_update(time);
  return telements.tm_year;
}

uint8_t timelib_second() { return timelib_second_t(timelib_get()); }

uint8_t timelib_minute() { return timelib_minute_t(timelib_get()); }

uint8_t timelib_hour() { return timelib_hour_t(timelib_get()); }

uint8_t timelib_wday() { return timelib_wday_t(timelib_get()); }

uint8_t timelib_day() { return timelib_day_t(timelib_get()); }

uint8_t timelib_month() { return timelib_month_t(timelib_get()); }

uint16_t timelib_year() { return timelib_year_t(timelib_get()); }

timelib_t timelib_make(struct timelib_tm* timeinfo) {
  int i;
  timelib_t tstamp;

  // Compute the number of seconds since the year 1970 to the begining of
  // the given year on the structure, add to the output value
  tstamp = timeinfo->tm_year * (TIMELIB_SECS_PER_DAY * 365);
  // Add the seconds corresponding to leap years (add extra days)
  for (i = 0; i < timeinfo->tm_year; i++) {
    if (timelib_is_leap(i + 1970)) tstamp += (timelib_t)TIMELIB_SECS_PER_DAY;
  }
  // Add seconds for the months elapsed
  for (i = 1; i < timeinfo->tm_mon; i++) {
    if (i == 2 && timelib_is_leap(timeinfo->tm_year + 1970))
      tstamp += (timelib_t)TIMELIB_SECS_PER_DAY * 29;
    else
      tstamp += (timelib_t)TIMELIB_SECS_PER_DAY * month_length[i - 1];
  }
  // Add seconds for past days
  tstamp +=
      (timelib_t)(timeinfo->tm_mday - 1) * (timelib_t)TIMELIB_SECS_PER_DAY;
  // Add seconds on this day
  tstamp += (timelib_t)timeinfo->tm_hour * (timelib_t)TIMELIB_SECS_PER_HOUR;
  tstamp += (timelib_t)timeinfo->tm_min * (timelib_t)TIMELIB_SECS_PER_MINUTE;
  tstamp += (timelib_t)timeinfo->tm_sec;

  return tstamp;
}

void timelib_break(timelib_t timeinput, struct timelib_tm* timeinfo) {
  uint8_t year;
  uint8_t month, monthLength;
  uint32_t time;
  unsigned long days;

  time = (uint32_t)timeinput;
  timeinfo->tm_sec = time % 60;

  time /= 60;  // now it is minutes
  timeinfo->tm_min = time % 60;

  time /= 60;  // now it is hours
  timeinfo->tm_hour = time % 24;

  time /= 24;                                // now it is days
  timeinfo->tm_wday = ((time + 4) % 7) + 1;  // Sunday is day 1

  year = 0;
  days = 0;
  while ((unsigned)(days += (timelib_is_leap(1970 + year) ? 366 : 365)) <= time)
    year++;

  timeinfo->tm_year = year;  // year is offset from 1970

  days -= timelib_is_leap(1970 + year) ? 366 : 365;
  time -= days;  // now it is days in this year, starting at 0

  days = 0;
  month = 0;
  monthLength = 0;
  for (month = 0; month < 12; month++) {
    // Check
    if (month == 1) {
      if (timelib_is_leap(1970 + year)) {
        monthLength = 29;
      } else {
        monthLength = 28;
      }
    } else {
      monthLength = month_length[month];
    }

    if (time >= monthLength) {
      time -= monthLength;
    } else {
      break;
    }
  }
  timeinfo->tm_mon = month + 1;  // jan is month 1
  timeinfo->tm_mday = time + 1;  // day of month
}

void timelib_set_provider(timelib_callback_t callback, timelib_t timespan) {
  // Check null pointer
  if (callback == 0) return;
  // Set new callback
  timelib_provider_callback = callback;
  // Enforce sync interval restrictions
  sync_interval = (timespan == 0) ? TIMELIB_SECS_PER_DAY : timespan;
  // Set next sync time to actual time
  sync_next = sys_time;
  // Force time sync
  timelib_get();
}
