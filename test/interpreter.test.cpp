#include <catch2/catch.hpp>
#include <filesystem>
#define TESTS
#include "../src/interpreter.hpp"

namespace fs = std::filesystem;

bool endsWith(const std::string& name, const std::string_view ext){
	return name.size() >= ext.size() && name.compare(name.size()-ext.size(), ext.size(), ext) == 0;
}

std::string readFile(const std::string& file){
	std::ifstream in(file, std::ios::in);
	std::string cont;
	in.seekg(0, std::ios::end);
	cont.resize(in.tellg());
	in.seekg(0, std::ios::beg);
	in.read(&cont[0], cont.size());
	in.close();
	return cont;
}


TEST_CASE("INTERPRETING", "[interpreter]"){
	for(const auto& file : fs::directory_iterator("test/valid-files")){
		const std::string name = file.path().filename().string();
		INFO("File is " << name);
		if(!endsWith(name, ".in.pcse")) continue; /* we don't want to look at this file */
		
		Lexer lex(readFile(name));
		Parser parser(lex.output);
		Env env(lex.identifier_count);
		parser.run(env);
		const std::string correct = readFile(name.substr(0, name.size() - strlen(".in.pcse")) + ".out");
		REQUIRE(env.out.str() == correct);
	}
}
