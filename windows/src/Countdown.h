#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace doom {
constexpr int64_t witaOffset = 8 * 3600;
struct Date { int year, month, day, hour = 0, minute = 0, second = 0; };
inline bool leap(int year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }
inline int daysInMonth(int year, int month) {
    const int lengths[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    return lengths[month - 1] + (month == 2 && leap(year));
}
inline bool valid(Date d) {
    return d.year >= 1970 && d.year <= 9998 && d.month >= 1 && d.month <= 12 &&
        d.day >= 1 && d.day <= daysInMonth(d.year,d.month) && d.hour >= 0 && d.hour < 24 &&
        d.minute >= 0 && d.minute < 60 && d.second >= 0 && d.second < 60;
}
// Proleptic Gregorian calendar. No dependency on the computer's timezone or DST.
inline int64_t civilDays(int year, int month, int day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = unsigned(year - era * 400);
    const unsigned doy = (153 * unsigned(month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    return int64_t(era) * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}
inline int64_t timestamp(Date d) {
    return civilDays(d.year,d.month,d.day)*86400 + d.hour*3600 + d.minute*60 + d.second - witaOffset;
}
inline Date dateAt(int64_t utc) {
    int64_t seconds = utc + witaOffset;
    int64_t days = seconds / 86400;
    int secondsOfDay = int(seconds % 86400);
    if (secondsOfDay < 0) { secondsOfDay += 86400; --days; }
    const int64_t z = days + 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = unsigned(z - era * 146097);
    const unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    int year = int(yoe) + int(era) * 400;
    const unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);
    const unsigned mp = (5*doy + 2) / 153;
    const int day = int(doy - (153*mp + 2)/5 + 1);
    const int month = int(mp) + (mp < 10 ? 3 : -9);
    year += month <= 2;
    return {year,month,day,secondsOfDay/3600,(secondsOfDay/60)%60,secondsOfDay%60};
}
inline Date addMonths(Date d, int months) {
    int total = d.year*12 + d.month - 1 + months;
    d.year = total/12; d.month = total%12 + 1;
    d.day = std::min(d.day,daysInMonth(d.year,d.month));
    return d;
}
inline int64_t defaultTarget() { return timestamp({2026,12,16}); }
inline std::array<int,5> parts(int64_t now, int64_t target) {
    if (now >= target) return {0,0,0,0,0};
    Date start = dateAt(now), end = dateAt(target);
    int months = (end.year-start.year)*12 + end.month-start.month;
    int64_t cursor = timestamp(addMonths(start,months));
    if (cursor > target) cursor = timestamp(addMonths(start,--months));
    int64_t remaining = target - cursor;
    return {months,int(remaining/86400),int(remaining/3600%24),int(remaining/60%60),int(remaining%60)};
}
constexpr double titleHeight = 74;
constexpr double numeralHeight = titleHeight * 0.70;
inline double desktopScale(int width, int height) {
    return std::min(std::max(width/1920.0,height/1080.0),width/1380.0);
}
} // namespace doom
