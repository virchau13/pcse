#include <iostream>
#include <vector>
#include "interpreter.hpp"

int main(int argc, char *argv[]){
	const char *filename = nullptr;
	bool print_tokens = false;
	bool print_tree = false;
	for(int i = 1; i < argc; i++){
		std::string_view arg(argv[i]);
		if(!arg.size()) goto fail;
		if(arg[0] == '-'){
			if(arg == "--help" || arg == "-h"){
				fprintf(stderr, 
					"Usage: %s [OPTIONS...] FILE\n"
					"Interpret FILE as pseudocode.\n"
					"Options:\n"
					"--print-tokens: Print the token list of the file.\n"
					"--print-tree: Print the syntax tree of the file.\n"
					"-h, --help: Print help.\n",
					argv[0]);
				exit(EXIT_SUCCESS);
			} else if(arg == "--print-tokens"){
				print_tokens = true;
			} else if(arg == "--print-tree"){
				print_tree = true;
			} else {
				fprintf(stderr, "Unknown option %s\n", argv[i]);
				goto fail;
			}
		} else {
			filename = argv[i];
			if(i != argc - 1){
				fprintf(stderr, "FILE must be the last option\n");
				goto fail;
			}
		}
		continue;
fail:
		fprintf(stderr, "Usage: %s [OPTIONS...] FILE\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if(filename == nullptr){
		fprintf(stderr, "No file specified!\n");
		exit(EXIT_FAILURE);
	}
	// read all from file
	std::ifstream in(filename, std::ios::in);
	if(!in){
		std::cerr << "File does not exist!\n";
		exit(EXIT_FAILURE);
	}

#define CATCH(name) \
	catch(name &e) { \
		std::cerr << #name << ": " << e.what() << '\n'; \
		return EXIT_FAILURE; \
	}
	
	try {
		Lexer lexer(in);
		if(print_tokens){
			for(const auto& token : lexer.output){
				std::cerr << token << '\n';
			}
		}
		Parser parser(lexer.output);
		if(print_tree){
			std::cerr << *parser.output << '\n';
		}
		Env env(lexer.identifier_count, lexer.id_num);
		parser.run(env);
	} catch(std::istream::failure& e){ 
		std::cerr << "File error: Failure to read file\n";
		std::cerr << "istream::failure::what(): " << e.what() << '\n';
		return EXIT_FAILURE;
	} CATCH(LexError) CATCH(ParseError) CATCH(TypeError) CATCH(RuntimeError);

	return EXIT_SUCCESS;
}
