#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <cstdlib>
#include <map>
#include <ostream>

#ifdef DEBUG
#include <iostream>
#endif

enum TokenType {
	INVALID,
	// symbols
	LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, LEFT_SQ, RIGHT_SQ,
	COMMA, MINUS, PLUS, SLASH, STAR, COLON,
	// comparers
	EQ, EQ_EQ,
	GT, GT_EQ,
	LT, LT_EQ,
	AND, OR, NOT,

	IDENTIFIER, STR, INT, REAL,

	IF, THEN, ELSE, ENDIF, DECLARE, FOR, TO, STEP, NEXT, WHILE, ENDWHILE, REPEAT, UNTIL, CONSTANT, INPUT, OUTPUT, CASE, OF, ENDCASE, PROCEDURE, ENDPROCEDURE, CALL, FUNCTION, RETURNS, RETURN, ENDFUNCTION,
	
	// types
	T_INTEGER, T_STRING, T_ARRAY, T_CHAR, T_BOOLEAN, T_DATE
};

// Maps to make life easier {{{
const std::map<std::string_view, TokenType> reservedWords {
	{"IF", IF},
	{"THEN", THEN},
	{"ELSE", ELSE},
	{"ENDIF", ENDIF},
	{"DECLARE", DECLARE},
	{"FOR", FOR},
	{"TO", TO},
	{"STEP", STEP},
	{"NEXT", NEXT},
	{"WHILE", WHILE},
	{"ENDWHILE", ENDWHILE},
	{"REPEAT", REPEAT},
	{"UNTIL", UNTIL},
	{"CONSTANT", CONSTANT},
	{"INPUT", INPUT},
	{"OUTPUT", OUTPUT},
	{"CASE", CASE},
	{"OF", OF},
	{"ENDCASE", ENDCASE},
	{"PROCEDURE", PROCEDURE},
	{"ENDPROCEDURE", ENDPROCEDURE},
	{"CALL", CALL},
	{"FUNCTION", FUNCTION},
	{"RETURNS", RETURNS},
	{"RETURN", RETURN},
	{"ENDFUNCTION", ENDFUNCTION}
};
const std::map<std::string_view, TokenType> types {
	{"INTEGER", T_INTEGER},
	{"STRING", T_STRING},
	{"ARRAY", T_ARRAY},
	{"CHAR", T_CHAR},
	{"BOOLEAN", T_BOOLEAN},
	{"DATE", T_DATE}
};
const std::map<TokenType, std::string_view> tokenNames {
	{INVALID, "INVALID"},
	{LEFT_PAREN, "LEFT_PAREN"},
	{RIGHT_PAREN, "RIGHT_PAREN"},
	{LEFT_BRACE, "LEFT_BRACE"},
	{RIGHT_BRACE, "RIGHT_BRACE"},
	{LEFT_SQ, "LEFT_SQ"},
	{RIGHT_SQ, "RIGHT_SQ"},
	{COMMA, "COMMA"},
	{MINUS, "MINUS"},
	{PLUS, "PLUS"},
	{SLASH, "SLASH"},
	{STAR, "STAR"},
	{COLON, "COLON"},
	{EQ, "EQ"},
	{EQ_EQ, "EQ_EQ"},
	{GT, "GT"},
	{GT_EQ, "GT_EQ"},
	{LT, "LT"},
	{LT_EQ, "LT_EQ"},
	{AND, "AND"},
	{OR, "OR"},
	{NOT, "NOT"},
	{IDENTIFIER, "IDENTIFIER"},
	{STR, "STR"},
	{INT, "INT"},
	{REAL, "REAL"},
	{IF, "IF"},
	{THEN, "THEN"},
	{ELSE, "ELSE"},
	{ENDIF, "ENDIF"},
	{DECLARE, "DECLARE"},
	{FOR, "FOR"},
	{TO, "TO"},
	{STEP, "STEP"},
	{NEXT, "NEXT"},
	{WHILE, "WHILE"},
	{ENDWHILE, "ENDWHILE"},
	{REPEAT, "REPEAT"},
	{UNTIL, "UNTIL"},
	{CONSTANT, "CONSTANT"},
	{INPUT, "INPUT"},
	{OUTPUT, "OUTPUT"},
	{CASE, "CASE"},
	{OF, "OF"},
	{ENDCASE, "ENDCASE"},
	{PROCEDURE, "PROCEDURE"},
	{ENDPROCEDURE, "ENDPROCEDURE"},
	{CALL, "CALL"},
	{FUNCTION, "FUNCTION"},
	{RETURNS, "RETURNS"},
	{RETURN, "RETURN"},
	{ENDFUNCTION, "ENDFUNCTION"},
	{T_INTEGER, "T_INTEGER"},
	{T_STRING, "T_STRING"},
	{T_ARRAY, "T_ARRAY"},
	{T_CHAR, "T_CHAR"},
	{T_BOOLEAN, "T_BOOLEAN"},
	{T_DATE, "T_DATE"}
};
// }}}

union Literal {
	double d64;
	int64_t i64;
	std::string_view str;
	inline Literal(int64_t i){
		i64 = i;
	}
	inline Literal(int i){
		i64 = i;
	}
	inline Literal(double d){
		d64 = d;
	}
	inline Literal(const std::string_view sv){
		str = sv;
	}
	inline Literal(const char *s){
		str = s;
	}
};

struct Token {
	size_t line;
	TokenType type;
	Literal literal;
	const bool operator==(const Token& t) const noexcept {
		return t.line == line
			&& t.type == type
			&& (t.type == REAL ? t.literal.d64 == literal.d64 :
				t.type == INT ? t.literal.i64 == literal.i64 :
				t.literal.i64 == 0 ? true :
				t.literal.str == literal.str);
	}
	// Make Catch2 print our type
	friend std::ostream& operator<<(std::ostream& os, const Token& t){
		const std::string_view tokenName = tokenNames.at(t.type);
		os << "{ line = " << t.line
			<< ", type = " << tokenName
			<< ", literal";
		if(t.type == REAL){
			os << ".d64 = " << t.literal.d64;
		} else if(t.type == INT){
			os << ".i64 = " << t.literal.i64;
		} else if(t.literal.i64 == 0){
			/* Keyword */
			os << ".i64 = 0";
		} else {
			// Str / identifier
			os << ".str = " << t.literal.str;
		}
		os << " }";
		return os;
	}
};
class Lexer {
public:
	std::string_view source;
	std::vector<Token> tokens;
private:
	size_t curr = 0, line = 1;
	inline const char advance() noexcept {
		char x = source[curr];
		curr++;
		return x;
	}
	/* Checks if the next character
	 * is `c`, and if so,
	 * it advances.
	 */
	inline const bool match(const char c) noexcept {
		if(source[curr] == c){
			curr++;
			return true;
		}
		return false;
	}
	inline void addToken(TokenType type, Literal lt) noexcept {
		tokens.push_back({ line, type, lt });
	}
	/* Assumes a 1-character token. */
	inline void addToken(TokenType type) noexcept {
		tokens.push_back({ line, type, 0 });
	}
	inline const char peek(size_t ahead = 1) const noexcept {
		if(curr + ahead - 1 >= source.length()) return '\0';
		else return source[curr + ahead - 1];
	}
	// after last character
	inline bool noMore() const noexcept {
		return curr >= source.length();
	}
	inline bool isAlNum_(const char c) const noexcept {
		return isalnum(c) || c == '_';
	}
	void lex(){
		bool stop = false;
		while(!noMore() && !stop){
			char c = advance();
			switch(c){
				case '(': addToken(LEFT_PAREN); break;
				case ')': addToken(RIGHT_PAREN); break;
				case '{': addToken(LEFT_BRACE); break;
				case '}': addToken(RIGHT_BRACE); break;
				case ',': addToken(COMMA); break;
				case '-': addToken(MINUS); break;
				case '+': addToken(PLUS); break;           
				case '*': addToken(STAR); break;
				case '=':
					if(match('=')){
						addToken(EQ_EQ);
					} else {
						addToken(EQ);
					}
					break;
				case '>':
					if(match('=')){
						addToken(GT_EQ);
					} else {
						addToken(GT);
					}
					break;
				case '<':
					if(match('=')){
						addToken(LT_EQ);
					} else {
						addToken(LT);
					}
					break;
				case '/':
					/* comment or division */
					if(match('/')){
						// comment until end of line
						while(peek() != '\n' && !noMore()) advance();
					} else {
						// division
						addToken(SLASH);
					}
					break;
				case ' ':
				case '\r':
				case '\t':
					// ignore whitespace
					break;
				case '\n':
					// line
					line++;
					break;
				case '"': {
					// string
					const size_t start = curr - 1;
					while(peek() != '"' && !noMore()){
						if(peek() == '\n') line++; // multiline strings
						advance();
					}
					if(!match('"')){
						addToken(INVALID);
						stop = true;
						break;
					}
					addToken(STR, source.substr(start, curr - start));
					break;
				}
				default:
					if(isdigit(c)) {
						// Integer or Real
						const char *start = source.data() + curr - 1;
						while(isdigit(peek())) advance();
						if(peek() == '.' && isdigit(peek(2))){
							// real
							advance();
							while(isdigit(peek())) advance();
							addToken(REAL, atof(start));
						} else {
							// integer
							addToken(INT, atoi(start));
						}
					} else if(isAlNum_(c)){
						// identifier or keyword or type
						const size_t start = curr - 1;
						while(isAlNum_(peek())){
							advance();
						}
						// Maximal monch rule
						std::string_view name = source.substr(start, curr - start);
						if(reservedWords.find(name) != reservedWords.end()){
							// reserved word 
							addToken(reservedWords.at(name));
						} else if(types.find(name) != types.end()){
							// type
							addToken(types.at(name));
						} else {
							// it's a variable name
							addToken(IDENTIFIER, name);
						}
					} else {
						// invalid character
						addToken(INVALID);
						stop = true;
					}
					break;
			}
		}
	}

public:
	Lexer(const std::string_view sv){
		source = sv;
		lex();
	}
};

#endif /* LEXER_HPP */
