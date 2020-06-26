#ifndef DATE_HPP
#define DATE_HPP

#include <cstdint>

struct Date {
	const uint8_t day, month;
	const uint16_t year;
	inline Date(uint8_t day_, uint8_t month_, uint16_t year_)
	: day(day_), month(month_), year(year_) {}
};

#endif
