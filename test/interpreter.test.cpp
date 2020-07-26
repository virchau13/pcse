#include <catch2/catch.hpp>
#include <filesystem>
#define TESTS
#include "../src/interpreter.hpp"

namespace fs = std::filesystem;

bool endsWith(const std::string& name, const std::string_view ext){
	return name.size() >= ext.size() && name.compare(name.size()-ext.size(), ext.size(), ext) == 0;
}

std::string readFile(const std::string& filepath){
	INFO("Filepath is " << filepath);
	std::ifstream in(filepath.c_str(), std::ios::in);
	if(!in){
		throw std::runtime_error("x");
	}
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
		
		std::ifstream in(file.path().c_str(), std::ios::in);
		/* Lexer::Lexer uses a std::string_view, so we have to destroy it _before_ contents */
		{

			Lexer lex(in);
			Parser parser(lex.output);
			Env env(lex.identifier_count, lex.id_num);
			std::string inpname = file.path().c_str();
			// ".in.pcse" => ".in"
			{
				std::string out = ".in";
				int len = strlen(".in.pcse") - out.size();
				while(len--){
					inpname.pop_back();
				}
				for(size_t i = 0; i < out.size(); i++){
					inpname[i + inpname.size() - out.size()] = out[i];
				}
			}
			try {
				const std::string inp = readFile(inpname);
				env.in = std::istringstream(inp);
			} catch(std::runtime_error& e){
				// no input
			}
			parser.run(env);

			std::string outname = file.path().c_str();
			
			// ".in.pcse" => ".out"
			{
				std::string out = ".out";
				int len = strlen(".in.pcse") - out.size();
				while(len--){
					outname.pop_back();
				}
				for(size_t i = 0; i < out.size(); i++){
					outname[i + outname.size() - out.size()] = out[i];
				}
			}
			const std::string correct = readFile(outname);
			REQUIRE(env.out.str() == correct);
		}
	}
	for(const auto& file : fs::directory_iterator("test/invalid-files")){
		const std::string name = file.path().filename().string();
		INFO("File is " << name);
		if(!endsWith(name, ".in.pcse")) continue; /* we don't want to look at this file */

		std::ifstream in(file.path().c_str(), std::ios::in);
		/* Lexer::Lexer uses a std::string_view, so we have to destroy it _before_ contents */
		{
			std::string outname = file.path().c_str();
			// ".in.pcse" => ".out"
			{
				std::string out = ".err";
				int len = strlen(".in.pcse") - out.size();
				while(len--){
					outname.pop_back();
				}
				for(size_t i = 0; i < out.size(); i++){
					outname[i + outname.size() - out.size()] = out[i];
				}
			}
			const std::string correct = readFile(outname);
			std::string errmsg = "";
#define CATCH(err) catch(err& e){ errmsg += #err; errmsg += ": "; errmsg += e.what(); errmsg += '\n'; }
			try {
				Lexer lex(in);
				Parser parser(lex.output);
				Env env(lex.identifier_count, lex.id_num);
				parser.run(env);
			} CATCH(LexError) CATCH(ParseError) CATCH(TypeError) CATCH(RuntimeError);
			REQUIRE(errmsg == correct);
		}
	}
}
