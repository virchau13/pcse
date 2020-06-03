/* Requires Catch2. */
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

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
	);
	TokenType x;
	std::vector<Token> expected = {
		/* Comments should be ignored. */
		{ .line = 4, .col = 4, .type = IDENTIFIER, .literal = "newlines" },
		{ .line = 5, .col = 1, .type = IDENTIFIER, .literal = "newline" },
		{ .line = 6, .col = 1, .type = INT, .literal = 2 },
		{ .line = 6, .col = 3, .type = REAL, .literal = 3.0 },
		{ .line = 6, .col = 7, .type = REAL, .literal = 4.999 },
		{ .line = 6, .col = 13, .type = INT, .literal = 5 },
		/* Note that the literal for this one _has_ to be 0.
		 * This is to help differentiate between tokens
		 * that are _keywords_, and tokens that actually contain data
		 * in `literal.str`. 
		 */
		{ .line = 6, .col = 15, .type = ENDFUNCTION, .literal = 0 },
		{ .line = 7, .col = 1, .type = STR, .literal = "str" }
	};
	REQUIRE(lex.tokens == expected);
}
