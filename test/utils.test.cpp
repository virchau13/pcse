#include <catch2/catch.hpp>
#include <limits>

#include "../src/utils.hpp"

TEST_CASE("isDigit()", "[utils]"){
	// Bruteforce all testcases.
	std::string digits = "0123456789";
	for(char c = std::numeric_limits<char>::min(); c < std::numeric_limits<char>::max(); c++){
		bool isnum = (digits.find(c) != std::string::npos);
		INFO("char is " << c);
		REQUIRE(isDigit(c) == isnum);
	}
}

std::string alphas = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

TEST_CASE("isAlpha()", "[utils]"){
	// Bruteforce all
	for(char c = std::numeric_limits<char>::min(); c < std::numeric_limits<char>::max(); c++){
		bool isal = (alphas.find(c) != std::string::npos);
		INFO("char is " << c);
		REQUIRE(isAlpha(c) == isal);
	}
}
