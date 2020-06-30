#include <catch2/catch.hpp>
#include <limits>

#include "../src/fraction.hpp"

TEST_CASE("Fractions", "[fraction]"){
	Fraction f(1, 3);
	REQUIRE(f * 3 == 1);
	f *= 3;
	REQUIRE(f == 1);
	REQUIRE(f == Fraction(1, 1));
}
