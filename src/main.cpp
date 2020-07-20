#include <iostream>
#include <vector>
#include "interpreter.hpp"

int main(int argc, char *argv[]){
	if(argc != 2){
		fprintf(stderr, "usage: %s filename\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	// read all from file
	std::ifstream in(argv[1], std::ios::in);
	if(!in){
		std::cerr << "File does not exist!\n";
		exit(EXIT_FAILURE);
	}
	std::string cont;
	in.seekg(0, std::ios::end);
	cont.resize(in.tellg());
	in.seekg(0, std::ios::beg);
	in.read(&cont[0], cont.size());
	in.close();
	
	Lexer lexer(cont);
	Parser parser(lexer.output);
	Env env(lexer.identifier_count);
	parser.run(env);
}
