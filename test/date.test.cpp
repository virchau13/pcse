#include <catch2/catch.hpp>
#include "../src/date.hpp"

TEST_CASE("Date", "[date]"){
	const std::array<std::vector<std::array<int, 3>>,2> should {{
		/* fail */ 
		{
			{ 0, 0, 0 },
			{ 29, 2, 2019 }
		},
		/* pass */
		{
			{ 31, 12, 2020 },
		}
	}};
	for(int should_pass = false; should_pass < 2; should_pass++){
		for(const auto& x : should[should_pass]){
			bool passed = true;
			try {
				Date d(x[0], x[1], x[2]);
			} catch(DateError& e){
				passed = false;
			}
			REQUIRE(should_pass == passed);
		}
	}
	Date d(1, 1, 1), f(2, 9, 2020);
	REQUIRE(d == d);
	REQUIRE(f == f);
	REQUIRE(d != f);
	REQUIRE(d < f);
	REQUIRE(d <= f);
	REQUIRE(f >= d);
	REQUIRE(f > d);
}
