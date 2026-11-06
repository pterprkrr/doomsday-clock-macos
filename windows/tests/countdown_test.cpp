#include "../src/Countdown.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

static int checks = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << label << '\n'; std::exit(1); }
}
int main(int argc, char** argv) {
    using namespace doom;
    check(defaultTarget() == civilDays(2026,12,15)*86400 + 16*3600,"WITA equals UTC+8");
    check(parts(timestamp({2026,11,5}),defaultTarget()) == std::array<int,5>{1,11,0,0,0},"calendar months");
    check(parts(defaultTarget()-1,defaultTarget()) == std::array<int,5>{0,0,0,0,1},"last second");
    check(parts(defaultTarget(),defaultTarget()) == std::array<int,5>{},"zero");
    check(parts(defaultTarget()+1000,defaultTarget()) == std::array<int,5>{},"expired");
    check(parts(timestamp({2024,1,31}),timestamp({2024,2,29})) == std::array<int,5>{1,0,0,0,0},"leap month clamp");
    check(parts(timestamp({2025,1,31}),timestamp({2025,2,28})) == std::array<int,5>{1,0,0,0,0},"month clamp");
    check(parts(timestamp({2026,11,16,0,0,1}),defaultTarget()) == std::array<int,5>{0,29,23,59,59},"month boundary time");
    check(!valid({2025,2,29}) && valid({2024,2,29}),"date validation");
    check(std::abs(numeralHeight/titleHeight-0.7)<0.00001,"70 percent numeral height");
    for (int year : {1970,2000,2024,2026,2100,2400,9998}) {
        for (int month=1;month<=12;++month) {
            for (int day=1;day<=daysInMonth(year,month);++day) {
                Date d{year,month,day,23,59,59};
                Date r=dateAt(timestamp(d));
                check(r.year==year && r.month==month && r.day==day && r.hour==23 && r.minute==59 && r.second==59,"date roundtrip");
            }
        }
    }
    for (auto size : {std::array<int,2>{1920,1080},{1366,768},{3840,2160},{1080,1920},{800,600},{5120,1440}}) {
        check(desktopScale(size[0],size[1])*1380<=size[0]+0.001,"fits screen width");
    }
    if (argc>1) {
        std::ifstream input(argv[1]);
        check(input.good(),"reference file readable");
        int64_t now,target; std::array<int,5> expected;
        while (input>>now>>target>>expected[0]>>expected[1]>>expected[2]>>expected[3]>>expected[4]) {
            if (parts(now,target)!=expected) std::cerr << "Mismatch: " << now << " -> " << target << '\n';
            check(parts(now,target)==expected,"macOS Calendar parity");
        }
    }
    std::cout << "PASS: " << checks << " countdown, calendar and layout checks\n";
}
