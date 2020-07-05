#include <catch2/catch.hpp>
#include "../src/lexer.hpp"
#include <filesystem>

namespace fs = std::filesystem;

TEST_CASE("Lexing", "[lex]"){
	{
		Lexer lex(
				" \n"
				" \r\n"
				" * // comments are ignored \t\r\n"
				"\t\t\tnewlines\n"
				"newline\n"
				"2 3.0 4.999 5 ENDFUNCTION\n"
				"\"str\"\n"
				"newline\n"
				"<- < -\n"
				"'#'"
				);
		std::vector<Token> expected = {
			// Order: line, col, type, literal
			{ 3, 2, TokenType::STAR, 0 },
			// Comments should be ignored.
			// Identifiers are stored as increasing numbers from 1 
			{  4, 4,  TokenType::IDENTIFIER, 1 }, // `newlines`
			{  5, 1,  TokenType::IDENTIFIER, 2 }, // `newline`
			{  6, 1,  TokenType::INT_C, 2 },
			{  6, 3,  TokenType::REAL_C, Fraction(3, 1) },
			{  6, 7,  TokenType::REAL_C, Fraction(4999, 1000) },
			{  6, 13, TokenType::INT_C, 5 },
			/* Note that the literal for this one _has_ to be 0.
			 * This is to help differentiate between tokens
			 * that are _keywords_, and tokens that actually contain data
			 * in `literal.str`. 
			 */
			{  6, 15, TokenType::ENDFUNCTION, 0 },
			{  7, 1, TokenType::STR_C, "str" },
			{  8, 1, TokenType::IDENTIFIER, 2 }, // Should be same identifier number as `newline`
			{  9, 1, TokenType::ASSIGN, 0 },
			{  9, 4, TokenType::LT, 0 },
			{  9, 6, TokenType::MINUS, 0 },
			{ 10, 1, TokenType::CHAR_C, '#' }
		};
		REQUIRE(lex.output == expected);
	}
	{
		const std::string_view words = "AND ARRAY BOOLEAN BYREF CALL CASE CHAR CONSTANT DATE DECLARE DIV ELSE ENDCASE ENDFUNCTION ENDIF ENDPROCEDURE ENDWHILE FALSE FOR FUNCTION IF INPUT INTEGER MOD NEXT NOT OF OR OTHERWISE OUTPUT PROCEDURE REAL REPEAT RETURN RETURNS STEP STRING THEN TO TRUE UNTIL WHILE";
		std::vector<size_t> cols;
		for(size_t i = 0; i < words.length(); i++){
			if(words[i] != ' ' && (i == 0 || words[i-1] == ' ')){
				cols.push_back(i+1);
			}
		}
		Lexer lex(words);
		std::stringstream sstream;
		for(const auto& tok : lex.output) sstream << tok;
		REQUIRE(lex.output.size() == cols.size());
		INFO("lex.output is a vector of size " << lex.output.size() << " with types:\n" << sstream.str());
		for(size_t i = 0; i < cols.size(); i++){
			REQUIRE(lex.output[i].line == 1);
			REQUIRE(lex.output[i].col == cols[i]);
			INFO("Current type: " << tokenTypeToStr(lex.output[i].type));
			REQUIRE(isReservedWord(lex.output[i].type));
			REQUIRE(lex.output[i].literal.i64 == 0);
		}
	}
	{
		/* When I first wrote this test.
		 * I wrote "21/20/2019".
		 * Seriously.
		 * That's 15 minutes of my life I am never getting back.
		 */
		Lexer lex(" 21/11/2019");
		REQUIRE(lex.output.size() == 1);
		const Token expected = { 1, 2, TokenType::DATE_C, Date(21, 11, 2019) };
		REQUIRE(lex.output[0] == expected);
	}
	for(const auto& file : fs::directory_iterator("test/lex-files")){
		const std::string name = file.path().filename().string();
		INFO("File is " << name);
		// check if the test is meant to pass/fail or should be ignored
		const std::string verdict = name.substr(0, 4);
		if(verdict != "fail" && verdict != "pass") /* ignore this file */ continue;
		const bool should_pass = (verdict == "pass");
		// read file
		std::ifstream in(file.path().c_str(), std::ios::in);
		std::string cont;
		in.seekg(0, std::ios::end);
		cont.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&cont[0], cont.size());
		in.close();

		// test it
		bool failed = false;
		try {
			Lexer tmp(cont);
		} catch(LexError& e){
			failed = true;
			UNSCOPED_INFO("Error is " << e.what());
		}
		REQUIRE(should_pass != failed);
	}
}
