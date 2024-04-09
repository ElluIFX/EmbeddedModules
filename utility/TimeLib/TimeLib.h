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
#ifndef TIMELIB_H
#define TIMELIB_H

/*-------------------------------------------------------------*
 *		Includes and dependencies			*
 *-------------------------------------------------------------*/
#include "TimeLibPort.h"

/*-------------------------------------------------------------*
 *		Library configuration				*
 *-------------------------------------------------------------*/

/**
 * Version string, should be changed in every release
 */
#define TIMELIB_VERSION_STRING "2.1.0"

/**
 * Enable function calls with names from version 1.0.0. Names where changed on
 * 1.1.0 to adhere to naming conventions. Define this macro to enable macros
 * that allow us to use old function names comment it to use only new API names.
 */
// #define CONFIG_TIMELIB_LEGACY_API

/*-------------------------------------------------------------*
 *		Macros and definitions				*
 *-------------------------------------------------------------*/
#define TIMELIB_SECS_PER_DAY (86400UL)
#define TIMELIB_SECS_PER_HOUR (3600UL)
#define TIMELIB_SECS_PER_MINUTE (60UL)
#define TIMELIB_DAYS_PER_WEEK (7UL)
#define TIMELIB_SECS_PER_WEEK (TIMELIB_SECS_PER_DAY * TIMELIB_DAYS_PER_WEEK)
#define TIMELIB_SECS_PER_YEAR (TIMELIB_SECS_PER_WEEK * 52UL)
#define TIMELIB_SECS_YEAR_2K (946684800UL)

/*-------------------------------------------------------------*
 *		Typedefs enums & structs			*
 *-------------------------------------------------------------*/
typedef uint64_t timelib_t;

/**
 * @brief Stores human readable time and date information
 *
 * Simplified structure to store human readable components of time /date similar
 * to the standard C structure for time information.
 */
struct timelib_tm {
  uint8_t tm_sec;   //!< Seconds
  uint8_t tm_min;   //!< Minutes
  uint8_t tm_hour;  //!< Hours
  uint8_t tm_wday;  //!< Day of week, sunday is day 1
  uint8_t tm_mday;  //!< Day of the month
  uint8_t tm_mon;   //!< Month
  uint8_t tm_year;  //!< Year offset from 1970;
};

/**
 * @brief Enumeration defines the current state of the system time
 */
enum time_status {
  E_TIME_NOT_SET = 0,  //!< Time has not been set
  E_TIME_NEEDS_SYNC,   //!< Time was set, but needs to be synced with timebase
  E_TIME_OK,           //!< Time is valid and in sync with time source
};

/**
 * @brief Type definition for the function pointer that gets precise time
 *
 * Pointer to function that gets time from external time source or device:
 * GPS, NTP, RTC, etc. This is needed to automatically sync time with
 * an external source.
 */
typedef timelib_t (*timelib_callback_t)();

/*-------------------------------------------------------------*
 *		Function prototypes				*
 *-------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Sets the current system time
 *
 * This function sets the time keeping system variable to the given value.
 * The time is stored and maintained as an integer value representing the
 * number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC (a Unix
 * timestamp).
 *
 * @param now Unix timestamp representing the number of seconds elapsed since
 * 00:00 hours, Jan 1, 1970 UTC to the present date.
 */
void timelib_set(timelib_t now);

/**
 * @brief Gets the current system time
 *
 * This function reads the value of the time keeping system variable.
 * The time is stored and maintained as an integer value representing the
 * number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC (a Unix
 * timestamp).
 *
 * @return A Unix timestamp representing the number of seconds elapsed since
 * 00:00 hours, Jan 1, 1970 UTC to the present date.
 */
timelib_t timelib_get();

/**
 * @brief Stops the time counter
 *
 * This function is intended to temporary stop the operation of the system
 * clock, however the underlying time base operates normally.
 */
void timelib_halt_clock();

/**
 * @brief Starts the time counter
 *
 * This function resumes the normal operation of the system clock.
 */
void timelib_resume_clock();

/**
 * @brief Gets the status of the system time
 *
 * This function helps the application to determine if the system time is
 * accurate or if the time has been set by the user.
 *
 * See enumeration time_status for details about return codes
 *
 * @return Returns a code that determines the time status.
 */
uint8_t timelib_get_status();

/**
 * Compute the second at a given timestamp
 *
 * @param time The timestamp to calculate the second for
 *
 * @return The elapsed seconds
 */
uint8_t timelib_second_t(timelib_t time);

/**
 * Compute the minute at a given timestamp
 *
 * @param time The timestamp to calculate the minute for
 *
 * @return The elapsed minutes
 */
uint8_t timelib_minute_t(timelib_t time);

/**
 * Compute the hour at a given timestamp
 *
 * @param time The timestamp to calculate the hour for
 *
 * @return The elapsed hours
 */
uint8_t timelib_hour_t(timelib_t time);

/**
 * Compute the day of the week at a given timestamp
 *
 * @param time The timestamp to calculate the day of the week for
 *
 * @return The day of the week (1-7)
 */
uint8_t timelib_wday_t(timelib_t time);

/**
 * Compute the day of the month at a given timestamp
 *
 * @param time The timestamp to calculate the day of the month for
 *
 * @return The day of the month (1-31)
 */
uint8_t timelib_day_t(timelib_t time);

/**
 * Compute the month at a given timestamp
 *
 * @param time The timestamp to calculate the month for
 *
 * @return The month (1-12)
 */
uint8_t timelib_month_t(timelib_t time);

/**
 * Compute the year at a given timestamp
 *
 * @param time The timestamp to calculate the year for
 *
 * @return The year (1970 - 201X).
 */
uint16_t timelib_year_t(timelib_t time);

/**
 * Gets the current second
 *
 * @return The elapsed seconds
 */
uint8_t timelib_second();

/**
 * Gets the current minute
 *
 * @return The elapsed minutes
 */
uint8_t timelib_minute();

/**
 * Gets the current hour
 *
 * @return The elapsed hours
 */
uint8_t timelib_hour();

/**
 * Compute the current day of the week
 *
 * @return The day of the week (1-7)
 */
uint8_t timelib_wday();

/**
 * Compute the current day of the month
 *
 * @return The day of the month (1-31)
 */
uint8_t timelib_day();

/**
 * Compute the current month
 *
 * @return The month (1-12)
 */
uint8_t timelib_month();

/**
 * Compute the current year
 *
 * @return The year (1970 - 201X)
 */
uint16_t timelib_year();

/**
 * @brief Generates a Unix Timestamp from the given time/date components
 *
 * This function generates the corresponding Unix timestamp for the provided
 * date and time information. The timestamp is an integral value representing
 * the number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC.
 *
 * See tm structure fore more details about each struct field.
 *
 * @param timeinfo A structure containing the human readable elements of the
 * date and time to convert to a UNIX timestamp.
 *
 * @return The UNIX Timestamp for the given time/date components
 */
timelib_t timelib_make(struct timelib_tm* timeinfo);

/**
 * @brief Get human readable time from Unix time
 *
 * This function performs the conversion from Unix timestamp to human readable
 * time components and places the information on a standard time structure.
 *
 * @param timeinput The timestamp to convert
 * @param timeinfo Pointer to tm structure to store the resulting time
 */
void timelib_break(timelib_t timeinput, struct timelib_tm* timeinfo);

/**
 * @brief Sets the callback function that obtains precise time
 *
 * This function sets a callback that runs to keep internal CPU time in sync
 * with an accurate time reference. This function also sets the time interval
 * for time synchronization.
 *
 * @param callback The callback to get precise time information, this callback
 * should return a timestamp.
 *
 * @param timespan The interval in seconds when this function should be called
 */
void timelib_set_provider(timelib_callback_t callback, timelib_t timespan);

#ifdef __cplusplus
}
#endif

/*-------------------------------------------------------------*
 *		Function like macros				*
 *-------------------------------------------------------------*/
/**
 * Alias for time_get() function
 */
#define tlnow() timelib_get()

/**
 * Alias for time_second() function
 */
#define tlsecond() timelib_second()

/**
 * Alias for time_minute() function
 */
#define tlminute() timelib_minute()

/**
 * Alias for time_hour() function
 */
#define tlhour() timelib_hour()

/**
 * Alias for time_wday() function
 */
#define tlwday() timelib_wday()

/**
 *  Alias for time_day() function
 */
#define tlday() timelib_day()

/**
 * Alias for time_month() function
 */
#define tlmonth() timelib_month()

/**
 * Alias for time_year() function
 */
#define tlyear() timelib_year()

/**
 * Converts year in tm struct to calendar year
 */
#define timelib_tm2calendar(y) ((y) + 1970)

/**
 * Converts calendar year to tm struct year
 */
#define timelib_calendar2tm(y) ((y)-1970)

/**
 * Converts tm struct year to year 2000 based time
 */
#define timelib_tm2y2k(y) ((y)-30)

/**
 * Converts year 2000 to tm struct year
 */
#define timelib_y2k2tm(y) ((y) + 30)

/**
 * Computes the day of the week. Sunday is day 1 and saturday is 7
 */
#define timelib_dow(t) \
  (((t / TIMELIB_SECS_PER_DAY + 4) % TIMELIB_DAYS_PER_WEEK) + 1)

/**
 * Computes the number of elapsed days for the given timestamp
 */
#define timelib_elapsed_days(t) (t / TIMELIB_SECS_PER_DAY)

/**
 * Computes the number of elapsed seconds since midnight today
 */
#define timelib_seconds_today(t) (t % TIMELIB_SECS_PER_DAY)

/**
 * Calculates the timestamp of the previous midnight for the given time
 */
#define timelib_prev_midnight(t)                              \
  (uint32_t)(((uint32_t)t / (uint32_t)TIMELIB_SECS_PER_DAY) * \
             (uint32_t)TIMELIB_SECS_PER_DAY)

/**
 * Calculates the timestamp of the next midnight for the given time
 */
#define timelib_next_midnight(t) \
  (timelib_prev_midnight(t) + TIMELIB_SECS_PER_DAY)

/**
 * Calculates the timestamp of the previous hour for the given time
 */
#define timelib_prev_hour(t)                                   \
  (uint32_t)(((uint32_t)t / (uint32_t)TIMELIB_SECS_PER_HOUR) * \
             (uint32_t)TIMELIB_SECS_PER_HOUR)

/**
 * Calculates the timestamp of the next hour for the given time
 */
#define timelib_next_hour(t) (timelib_prev_midnight(t) + TIMELIB_SECS_PER_HOUR)

/**
 * Calculates the number of seconds elapsed since the start of the week
 */
#define timelib_secs_this_week(t) \
  (timelib_seconds_today(t) + ((timelib_dow(t) - 1) * TIMELIB_SECS_PER_DAY))

/**
 * Calculates the timestamp at midnight of the last Sunday
 */
#define timelib_prev_sunday(t) (t - timelib_secs_this_week(t))

/**
 * Calculates the timestamp at the beginning of the next Sunday
 */
#define timelib_next_sunday(t) (timelib_prev_sunday(t) + TIMELIB_SECS_PER_WEEK)

/*-------------------------------------------------------------*
 *		Legacy API macros				*
 *-------------------------------------------------------------*/
#if defined(CONFIG_TIMELIB_LEGACY_API)

#define TIME_SECS_PER_DAY TIMELIB_SECS_PER_DAY
#define TIME_SECS_PER_HOUR TIMELIB_SECS_PER_HOUR
#define TIME_SECS_PER_MINUTE TIMELIB_SECS_PER_MINUTE
#define TIME_DAYS_PER_WEEK TIMELIB_DAYS_PER_WEEK
#define TIME_SECS_PER_WEEK TIMELIB_SECS_PER_WEEK
#define TIME_SECS_PER_YEAR TIMELIB_SECS_PER_YEAR
#define TIME_SECS_YEAR_2K TIMELIB_SECS_YEAR_2K

#define now() tlnow()
#define time_set(x) timelib_set(x)
#define time_get() timelib_get()
#define time_halt_clock() timelib_halt_clock()
#define time_resume_clock() timelib_resume_clock()
#define time_get_status() timelib_get_status()
#define time_second_t(x) timelib_second_t(x)
#define time_minute_t(x) timelib_minute_t(x)
#define time_hour_t(x) timelib_hour_t(x)
#define time_wday_t(x) timelib_wday_t(x)
#define time_day_t(x) timelib_day_t(x)
#define time_month_t(x) timelib_month_t(x)
#define time_year_t(x) timelib_year_t(x)
#define time_second() timelib_second()
#define time_minute() timelib_minute()
#define time_hour() timelib_hour()
#define time_wday() timelib_wday()
#define time_day() timelib_day()
#define time_month() timelib_month()
#define time_year() timelib_year()
#define time_make(x) timelib_make(x)
#define time_break(x, y) timelib_break(x, y)
#define time_set_provider(x, y) timelib_set_provider(x, y)
#define time_tm2calendar(x) timelib_tm2calendar(x)
#define time_calendar2tm(x) timelib_calendar2tm(x)
#define time_tm2y2k(x) timelib_tm2y2k(x)
#define time_y2k2tm(x) timelib_y2k2tm(x)
#define time_dow(x) timelib_dow(x)
#define time_elapsed_days(x) timelib_elapsed_days(x)
#define time_seconds_today(x) timelib_seconds_today(x)
#define time_prev_midnight(x) timelib_prev_midnight(x)
#define time_next_midnight(x) timelib_next_midnight(x)
#define time_secs_this_week(x) timelib_secs_this_week(x)
#define time_prev_sunday(x) timelib_prev_sunday(x)
#define time_next_sunday(x) timelib_next_sunday(x)

#endif

#endif
// End of Header file
