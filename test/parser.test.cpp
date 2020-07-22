#include <catch2/catch.hpp>
#define TESTS
#include "../src/parser.hpp"
#include <filesystem>

namespace fs = std::filesystem;

struct Test {
	const Lexer lex;
	const Parser p;
	Test(const std::string_view sv): lex(sv), p(lex.output) {}
};

TEST_CASE("Parsing", "[parser]"){

	{
		Test test("FOR i <- 1 TO 10 OUTPUT i NEXT");
		Program &p = *test.p.output;
		REQUIRE(p.stmts.size() == 1);
		REQUIRE(p.stmts[0].form == StmtForm::FOR);
		REQUIRE(p.stmts[0].blocks.size() == 1);
		Block &b = p.stmts[0].blocks[0];
		REQUIRE(b.stmts.size() == 1);
		REQUIRE(b.stmts[0].form == StmtForm::OUTPUT);
		REQUIRE(b.stmts[0].exprs.size() == 1);
		REQUIRE(p.stmts[0].exprs.size() == 2);
	}

	{ 
		Test test("OUTPUT 2 + 3 * 4");
		Program& p = *test.p.output;
		// std::cout << p << '\n';
		REQUIRE(p.stmts.size() == 1);
		REQUIRE(p.stmts[0].exprs.size() == 1);
		{
			// check if is in correct tree structure
			std::stringstream sstream;
			sstream << p;
		}
	}

	for(const auto& file : fs::directory_iterator("test/parser-files")){
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
			std::stringstream sstream;
			for(const auto& t : tmp.output){
				sstream << t;
			}
			UNSCOPED_INFO("Tokens are " << sstream.str() << '\n');
			Parser p(tmp.output);
			/* Despite this seeming useless, it actually serves two purposes.
			 * (1) If the parser does not throw an exception but was supposed to, 
			 *     this shows what it decoded into.
			 * (2) It checks if the syntax tree is valid: if any part is logically inconsistent,
			 *     it'll segfault due to it dereferencing a null pointer most of the time.
			 *     :)
			 */
			UNSCOPED_INFO("Program is: \n" << *p.output << '\n');
		} catch(ParseError& e){
			failed = true;
			UNSCOPED_INFO("Error is " << e.what() << "\nat: " << e.token.line << ":" << e.token.col << "\n");
		}
		REQUIRE(should_pass != failed);
	}
}

