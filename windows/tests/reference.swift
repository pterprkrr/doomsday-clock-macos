import Foundation
var calendar = Calendar(identifier: .gregorian)
calendar.timeZone = TimeZone(secondsFromGMT: 8 * 3600)!
func date(_ y: Int, _ m: Int, _ d: Int, _ h: Int = 0, _ min: Int = 0, _ s: Int = 0) -> Date {
    calendar.date(from: DateComponents(year:y,month:m,day:d,hour:h,minute:min,second:s))!
}
func emit(_ start: Date, _ target: Date) {
    let p = calendar.dateComponents([.month,.day,.hour,.minute,.second], from:start,to:target)
    let values = target > start ? [p.month!,p.day!,p.hour!,p.minute!,p.second!] : [0,0,0,0,0]
    print(([Int(start.timeIntervalSince1970),Int(target.timeIntervalSince1970)]+values).map(String.init).joined(separator:" "))
}
for year in [2024,2025,2026,2100] {
    for month in 1...12 {
        for day in [1,15,28,29,30,31] {
            let start = date(year,month,day,23,59,59)
            for distance in [1,2,7,30,31,59,60,365,400] {
                emit(start,start.addingTimeInterval(Double(distance*86400-13)))
            }
        }
    }
}
emit(date(2026,11,5),date(2026,12,16))
emit(date(2026,12,16),date(2026,12,16))
emit(date(2027,1,1),date(2026,12,16))
