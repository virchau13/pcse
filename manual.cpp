#include <iostream>
#include <algorithm>
#define DEBUG 1
#include "lexer.hpp"

int main(){
	std::cout << "> ";
	while(1){
		std::string source, line;
		while(1){
			std::getline(std::cin, line);
			if(line == "EOF") break;
			else {
				source += line;
				source += '\n';
			}
		}
		Lexer lex(source);
		for(const auto& token : lex.tokens){
			std::cout << token << '\n';
		}
		source.clear();
	}
}
