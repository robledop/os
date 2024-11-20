#include <printf.h>
#include <string.h>
#include <time.h>

// Arrays of month and weekday names
static const char *month_names[] = {"January",
                                    "February",
                                    "March",
                                    "April",
                                    "May",
                                    "June",
                                    "July",
                                    "August",
                                    "September",
                                    "October",
                                    "November",
                                    "December"};

static const char *weekday_names[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

int is_leap_year(const int year)
{
    if (year % 4 != 0) {
        return 0; // Not divisible by 4
    }
    if (year % 100 != 0) {
        return 1; // Divisible by 4 and not by 100
    }
    if (year % 400 != 0) {
        return 0; // Divisible by 100 and not by 400
    }
    return 1; // Divisible by 400
}

size_t strftime(const char *format, const struct tm *tm, char *buffer, size_t maxsize)
{
    size_t written  = 0;
    const char *ptr = format;
    char temp[100];

    while (*ptr != '\0' && written < maxsize - 1) {
        if (*ptr == '%') {
            ptr++;
            if (*ptr == '\0') {
                break;
            }

            switch (*ptr) {
            case 'Y': // Year with century
                written += snprintf(temp, sizeof(temp), "%04d", tm->tm_year + 1900);
                strncat(buffer, temp, maxsize - strlen(buffer) - 1);
                break;
            case 'm': // Month as a decimal number [01,12]
                written += snprintf(temp, sizeof(temp), "%02d", tm->tm_mon + 1);
                strncat(buffer, temp, maxsize - strlen(buffer) - 1);
                break;
            case 'd': // Day of the month [01,31]
                written += snprintf(temp, sizeof(temp), "%02d", tm->tm_mday);
                strncat(buffer, temp, maxsize - strlen(buffer) - 1);
                break;
            case 'H': // Hour (24-hour clock) [00,23]
                written += snprintf(temp, sizeof(temp), "%02d", tm->tm_hour);
                strncat(buffer, temp, maxsize - strlen(buffer) - 1);
                break;
            case 'M': // Minute [00,59]
                written += snprintf(temp, sizeof(temp), "%02d", tm->tm_min);
                strncat(buffer, temp, maxsize - strlen(buffer) - 1);
                break;
            case 'S': // Second [00,59]
                written += snprintf(temp, sizeof(temp), "%02d", tm->tm_sec);
                strncat(buffer, temp, maxsize - strlen(buffer) - 1);
                break;
            case 'A': // Full weekday name
                written += snprintf(temp, sizeof(temp), "%s", weekday_names[tm->tm_wday]);
                strncat(buffer, temp, maxsize - strlen(buffer) - 1);
                break;
            case 'B': // Full month name
                written += snprintf(temp, sizeof(temp), "%s", month_names[tm->tm_mon]);
                strncat(buffer, temp, maxsize - strlen(buffer) - 1);
                break;
            case 'c': // Date and time representation
                written += snprintf(temp,
                                    sizeof(temp),
                                    "%04d-%02d-%02d %02d:%02d:%02d",
                                    tm->tm_year + 1900,
                                    tm->tm_mon + 1,
                                    tm->tm_mday,
                                    tm->tm_hour,
                                    tm->tm_min,
                                    tm->tm_sec);
                strncat(buffer, temp, maxsize - strlen(buffer) - 1);
                break;
            case '%': // A literal '%' character
                if (written < maxsize - 1) {
                    buffer[written++] = '%';
                    buffer[written]   = '\0';
                }
                break;
            default:
                // Unknown specifier, copy as is
                if (written < maxsize - 1) {
                    buffer[written++] = '%';
                    buffer[written++] = *ptr;
                    buffer[written]   = '\0';
                }
                break;
            }
        } else {
            // Regular character, copy to buffer
            if (written < maxsize - 1) {
                buffer[written++] = *ptr;
                buffer[written]   = '\0';
            }
        }
        ptr++;
    }

    // Ensure null-termination
    buffer[written] = '\0';
    return written;
}

int days_in_month(const int year, const int month)
{
    static const int month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 1) { // February
        return is_leap_year(year) ? 29 : 28;
    }
    if (month >= 0 && month <= 11) {
        return month_days[month];
    }
    // Invalid month
    return 0;
}

time_t mktime(const struct tm *tm)
{
    const int year  = tm->tm_year + 1900; // tm_year is years since 1900
    const int month = tm->tm_mon;         // tm_mon is months since January [0,11]
    const int day   = tm->tm_mday;        // Day of the month [1,31]
    const int hour  = tm->tm_hour;        // Hours since midnight [0,23]
    const int min   = tm->tm_min;         // Minutes after the hour [0,59]
    const int sec   = tm->tm_sec;         // Seconds after the minute [0,59]

    // Total days relative to 1970-01-01
    int64_t total_days = 0;

    if (year >= 1970) {
        // Years after 1970
        for (int y = 1970; y < year; y++) {
            total_days += is_leap_year(y) ? 366 : 365;
        }
    } else {
        // Years before 1970
        for (int y = 1969; y >= year; y--) {
            total_days -= is_leap_year(y) ? 366 : 365;
        }
    }

    // Months in the current year
    for (int m = 0; m < month; m++) {
        total_days += days_in_month(year, m);
    }

    // Days in the current month
    total_days += day - 1; // tm_mday starts from 1

    const int64_t total_seconds = total_days * 86400LL + hour * 3600LL + min * 60LL + sec;

    return total_seconds;
}

void unix_timestamp_to_tm(const time_t timestamp, struct tm *tm)
{
    constexpr int64_t SECONDS_PER_DAY = 86400LL;
    constexpr int DAYS_PER_WEEK       = 7;
    constexpr int EPOCH_YEAR          = 1970;
    constexpr int TM_YEAR_BASE        = 1900;

    // Calculate total days and remaining seconds
    int64_t days              = timestamp / SECONDS_PER_DAY;
    int64_t remaining_seconds = timestamp % SECONDS_PER_DAY;

    // Adjust for negative timestamps
    if (remaining_seconds < 0) {
        remaining_seconds += SECONDS_PER_DAY;
        days -= 1;
    }

    // Calculate current time (hour, minute, second)
    tm->tm_hour = remaining_seconds / 3600;
    remaining_seconds %= 3600;
    tm->tm_min = remaining_seconds / 60;
    tm->tm_sec = remaining_seconds % 60;

    // Calculate day of the week (Unix epoch was a Thursday, which is day 4)
    // For negative days, adjust the weekday calculation
    int64_t wday = (days + 4) % DAYS_PER_WEEK;
    if (wday < 0) {
        wday += DAYS_PER_WEEK;
    }
    tm->tm_wday = wday; // [0,6], where 0 = Sunday

    // Calculate current year
    int year = EPOCH_YEAR;
    int64_t days_in_year;

    if (days >= 0) {
        // For dates after or on 1970-01-01
        while (1) {
            days_in_year = is_leap_year(year) ? 366 : 365;
            if (days >= days_in_year) {
                days -= days_in_year;
                year++;
            } else {
                break;
            }
        }
    } else {
        // For dates before 1970-01-01
        year = EPOCH_YEAR - 1;
        while (1) {
            days_in_year = is_leap_year(year) ? 366 : 365;
            days += days_in_year;
            if (days >= 0) {
                break;
            }
            year--;
        }
    }

    tm->tm_year = year - TM_YEAR_BASE; // Adjust for struct tm

    // Days in each month
    int month_days[12] = {
        31, // January
        28, // February
        31, // March
        30, // April
        31, // May
        30, // June
        31, // July
        31, // August
        30, // September
        31, // October
        30, // November
        31  // December
    };

    // Adjust for leap year in February
    if (is_leap_year(year)) {
        month_days[1] = 29;
    }

    // Calculate the month and day
    int month = 0;
    while (month < 12 && days >= month_days[month]) {
        days -= month_days[month];
        month++;
    }
    tm->tm_mon  = month;    // Months since January [0,11]
    tm->tm_mday = days + 1; // Day of the month [1,31]

    // Calculate day of the year
    tm->tm_yday = 0;
    for (int i = 0; i < month; i++) {
        tm->tm_yday += month_days[i];
    }
    tm->tm_yday += tm->tm_mday - 1; // Days since January 1 [0,365]

    // Set Daylight Saving Time flag (not handled here)
    tm->tm_isdst = 0; // 0 for no DST, 1 for DST, -1 for unknown
}