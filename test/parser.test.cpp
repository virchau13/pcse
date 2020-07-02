#include <catch2/catch.hpp>
#include "../src/parser.hpp"
#include <filesystem>

namespace fs = std::filesystem;

Program *test_string(const std::string_view sv){
	Lexer tmp(sv);
	Parser p(tmp.output);
	return p.output;
}

TEST_CASE("Parsing", "[parser]"){

	{
		Program &p = *test_string(
			"FOR i <- 1 TO 10 OUTPUT i NEXT"
		);
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
		Program& p = *test_string(
			"OUTPUT 2 + 3 * 4\n"
		);
		// std::cout << p << '\n';
		REQUIRE(p.stmts.size() == 1);
		REQUIRE(p.stmts[0].exprs.size() == 1);
		{
			// check if is in correct tree structure
			// (sorry for messy code)
			Expr& e0 = p.stmts[0].exprs[0];
			REQUIRE(e0.exprs.size() == 1);
			REQUIRE(e0.ops.size() == 0);
			auto& e1 = e0.exprs[0];
			REQUIRE(e1.exprs.size() == 1);
			REQUIRE(e1.ops.size() == 0);
			auto& e2 = e1.exprs[0];
			REQUIRE(e2.exprs.size() == 1);
			REQUIRE(e2.ops.size() == 0);
			auto& e3 = e2.exprs[0];
			REQUIRE(e3.exprs.size() == 2);
			REQUIRE(e3.ops.size() == 1);
			REQUIRE(e3.ops[0] == TokenType::PLUS);
			{
				auto& e4 = e3.exprs[0];
				REQUIRE(e4.exprs.size() == 1);
				REQUIRE(e4.ops.size() == 0);
				auto& un = e4.exprs[0];
				REQUIRE(un.op == TokenType::INVALID);
				auto& prim = *un.main.primary;
				REQUIRE(prim.primtype() == TokenType::INT_C);
				REQUIRE(prim.main().lt.i64 == 2);
			}
			{
				auto& e4 = e3.exprs[1];
				REQUIRE(e4.exprs.size() == 2);
				REQUIRE(e4.ops.size() == 1);
				REQUIRE(e4.ops[0] == TokenType::STAR);
				for(int64_t i = 0; i < 2; i++){
					auto& un = e4.exprs[i];
					REQUIRE(un.op == TokenType::INVALID);
					auto& prim = *un.main.primary;
					REQUIRE(prim.primtype() == TokenType::INT_C);
					REQUIRE(prim.main().lt.i64 == 3 + i);
				}
			}
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
			UNSCOPED_INFO("Program is: \n" << *p.output << '\n');
		} catch(ParseError& e){
			failed = true;
			UNSCOPED_INFO("Error is " << e.what() << "\nat: " << e.token.line << ":" << e.token.col << "\n");
		}
		REQUIRE(should_pass != failed);
	}
}

