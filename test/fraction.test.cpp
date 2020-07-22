#include <catch2/catch.hpp>
#include <limits>

#include "../src/fraction.hpp"

TEST_CASE("Fractions", "[fraction]"){
	{
		Fraction f(1, 3);
		REQUIRE(f * 3 == 1);
		f *= 3;
		REQUIRE(f == 1);
		REQUIRE(f == Fraction(1, 1));
	}

	// Bug I ran into while implementing
	{
		Fraction l = Fraction(21, 10) /* 2.1 */, r = Fraction(2, 10) /* 0.2 */;
		REQUIRE(l + r == Fraction(23, 10)); // 2.1 + 0.2 = 2.3
		REQUIRE(l - r == Fraction(19, 10)); // 2.1 - 0.2 = 1.9
		REQUIRE(l * r == Fraction(42, 100)); // 2.1 * 0.2 = 0.42
		REQUIRE(l / r == Fraction(21, 2)); // 2.1 / 0.2 = 10.5
	}
}
