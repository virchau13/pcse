#ifndef DATE_HPP
#define DATE_HPP

#include <cstdint>
#include <stdexcept>

struct DateError : public std::runtime_error {
	template<typename... Args>
	DateError(Args... args): std::runtime_error(args...) {}
};

struct Date {
	uint8_t day, month;
	uint16_t year;
	inline Date(uint8_t day_, uint8_t month_, uint16_t year_)
	: day(day_), month(month_), year(year_) {
		if(month > 12) throw DateError("Month value too high");
		size_t max_day;
		switch(month){
			case 1:
			case 3:
			case 5:
			case 7:
			case 8:
			case 10:
			case 12:
				max_day = 31;
				break;
			case 2:
				max_day = (year % 4 == 0 && !(year % 100 == 0 && year % 400 == 0) /* is leap year */ ?
					29 : 28);
				break;
			default:
				max_day = 30;
				break;
		}
		if(day > max_day){
			throw DateError("Day value too high");
		}
	}
	inline bool operator==(const Date other){
		return day == other.day &&
			month == other.month &&
			year == other.year;
	}
	inline bool operator!=(const Date other){ return !operator==(other); }

#define ltgt(op) \
	inline bool operator op (const Date other) { \
		if(year op other.year) return true; \
		if(year == other.year){ \
			if(month op other.month) return true; \
			if(month == other.month){ \
				if(day op other.day) return true; \
				return false; \
			} \
			return false; \
		} \
		return false; \
	}

	ltgt(<);
	ltgt(>);
	ltgt(<=);
	ltgt(>=);

#undef ltgt
};

#endif
