/* Requires Catch2. */
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "fraction.hpp"
#include "lexer.hpp"

TEST_CASE("Lexing", "[lex]"){
	Lexer lex(
		" \n"
		" \r\n"
		" // comments are ignored \t\r\n"
		"\t\t\tnewlines\n"
		"newline\n"
		"2 3.0 4.999 5 ENDFUNCTION\n"
		"\"str\"\n"
		"newline\n"
	);
	std::vector<Token> expected = {
		// Order: line, col, type, literal
		/* Comments should be ignored. */
								/* Identifiers are stored as increasing numbers from 1 */
		{  4, 4,  TokenType::IDENTIFIER, 1 }, /* `newlines` */
		{  5, 1,  TokenType::IDENTIFIER, 2 }, /* `newline` */
		{  6, 1,  TokenType::INT, 2 },
		{  6, 3,  TokenType::REAL, Fraction(3, 1) },
		{  6, 7,  TokenType::REAL, Fraction(4999, 1000) },
		{  6, 13, TokenType::INT, 5 },
		/* Note that the literal for this one _has_ to be 0.
		 * This is to help differentiate between tokens
		 * that are _keywords_, and tokens that actually contain data
		 * in `literal.str`. 
		 */
		{  6, 15, TokenType::ENDFUNCTION, 0 },
		{  7, 1, TokenType::STR, "str" },
		{  8, 1, TokenType::IDENTIFIER, 2 } /* Should be same identifier number as `newline` */
	};
	REQUIRE(lex.tokens == expected);
}


TEST_CASE("Fractions", "[fraction]"){
	Fraction f(1, 3);
	REQUIRE(f * 3 == 1);
	f *= 3;
	REQUIRE(f == 1);
	REQUIRE(f == Fraction(1, 1));
}
