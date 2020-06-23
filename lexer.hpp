#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <cstdlib>
#include <map>
#include <ostream>
#include <algorithm>
#include <cstring>

#ifdef DEBUG
#include <iostream>
#endif

#include "fraction.hpp"

// Tokens and string-to-token maps {{{

// We will use X-Macros
// to make the enum <-> string conversion easier.
#define TOKENTYPE_LIST \
	TOK(INVALID, "INVALID"),\
	TOK(LEFT_PAREN, "LEFT_PAREN"),\
	TOK(RIGHT_PAREN, "RIGHT_PAREN"),\
	TOK(LEFT_BRACE, "LEFT_BRACE"),\
	TOK(RIGHT_BRACE, "RIGHT_BRACE"),\
	TOK(LEFT_SQ, "LEFT_SQ"),\
	TOK(RIGHT_SQ, "RIGHT_SQ"),\
	TOK(COMMA, "COMMA"),\
	TOK(MINUS, "MINUS"),\
	TOK(PLUS, "PLUS"),\
	TOK(SLASH, "SLASH"),\
	TOK(STAR, "STAR"),\
	TOK(COLON, "COLON"),\
	TOK(ASSIGN, "ASSIGN"),\
	TOK(EQ, "EQ"),\
	TOK(LT_GT, "LT_GT"),\
	TOK(GT, "GT"),\
	TOK(GT_EQ, "GT_EQ"),\
	TOK(LT, "LT"),\
	TOK(LT_EQ, "LT_EQ"),\
	TOK(AND, "AND"),\
	TOK(OR, "OR"),\
	TOK(NOT, "NOT"),\
	TOK(IDENTIFIER, "IDENTIFIER"),\
	TOK(STR, "STR"),\
	TOK(INT, "INT"),\
	TOK(REAL, "REAL"),\
	TOK(IF, "IF"),\
	TOK(THEN, "THEN"),\
	TOK(ELSE, "ELSE"),\
	TOK(ENDIF, "ENDIF"),\
	TOK(DECLARE, "DECLARE"),\
	TOK(FOR, "FOR"),\
	TOK(TO, "TO"),\
	TOK(STEP, "STEP"),\
	TOK(NEXT, "NEXT"),\
	TOK(WHILE, "WHILE"),\
	TOK(ENDWHILE, "ENDWHILE"),\
	TOK(REPEAT, "REPEAT"),\
	TOK(UNTIL, "UNTIL"),\
	TOK(CONSTANT, "CONSTANT"),\
	TOK(INPUT, "INPUT"),\
	TOK(OUTPUT, "OUTPUT"),\
	TOK(CASE, "CASE"),\
	TOK(OF, "OF"),\
	TOK(OTHERWISE, "OTHERWISE"),\
	TOK(ENDCASE, "ENDCASE"),\
	TOK(PROCEDURE, "PROCEDURE"),\
	TOK(BYREF, "BYREF"),\
	TOK(ENDPROCEDURE, "ENDPROCEDURE"),\
	TOK(CALL, "CALL"),\
	TOK(FUNCTION, "FUNCTION"),\
	TOK(RETURNS, "RETURNS"),\
	TOK(RETURN, "RETURN"),\
	TOK(ENDFUNCTION, "ENDFUNCTION"),\
	TOK(T_INTEGER, "T_INTEGER"),\
	TOK(T_REAL, "T_REAL"),\
	TOK(T_STRING, "T_STRING"),\
	TOK(T_ARRAY, "T_ARRAY"),\
	TOK(T_CHAR, "T_CHAR"),\
	TOK(T_BOOLEAN, "T_BOOLEAN"),\
	TOK(T_DATE, "T_DATE"),\
	TOK(TRUE, "TRUE"),\
	TOK(FALSE, "FALSE"),\
	TOK(MOD, "MOD"),\
	TOK(DIV, "DIV")


enum class TokenType {
#define TOK(a,b) a
	TOKENTYPE_LIST
#undef TOK
};

const std::vector<std::string> TokenTypeStrTable = {
#define TOK(a,b) b
	TOKENTYPE_LIST
#undef TOK
};
inline const std::string_view tokenTypeToStr(const TokenType type) noexcept {
	return TokenTypeStrTable[static_cast<int>(type)];
}
const std::map<std::string_view, TokenType> reservedWords {
	{"IF", TokenType::IF},
	{"THEN", TokenType::THEN},
	{"ELSE", TokenType::ELSE},
	{"ENDIF", TokenType::ENDIF},
	{"DECLARE", TokenType::DECLARE},
	{"FOR", TokenType::FOR},
	{"TO", TokenType::TO},
	{"STEP", TokenType::STEP},
	{"NEXT", TokenType::NEXT},
	{"WHILE", TokenType::WHILE},
	{"ENDWHILE", TokenType::ENDWHILE},
	{"REPEAT", TokenType::REPEAT},
	{"UNTIL", TokenType::UNTIL},
	{"CONSTANT", TokenType::CONSTANT},
	{"INPUT", TokenType::INPUT},
	{"OUTPUT", TokenType::OUTPUT},
	{"CASE", TokenType::CASE},
	{"OF", TokenType::OF},
	{"OTHERWISE", TokenType::OTHERWISE},
	{"ENDCASE", TokenType::ENDCASE},
	{"PROCEDURE", TokenType::PROCEDURE},
	{"BYREF", TokenType::BYREF},
	{"ENDPROCEDURE", TokenType::ENDPROCEDURE},
	{"CALL", TokenType::CALL},
	{"FUNCTION", TokenType::FUNCTION},
	{"RETURNS", TokenType::RETURNS},
	{"RETURN", TokenType::RETURN},
	{"ENDFUNCTION", TokenType::ENDFUNCTION},
	{"TRUE", TokenType::TRUE},
	{"FALSE", TokenType::FALSE},
	{"MOD", TokenType::MOD},
	{"DIV", TokenType::DIV}
};
const std::map<std::string_view, TokenType> types {
	{"INTEGER", TokenType::T_INTEGER},
	{"REAL", TokenType::T_REAL},
	{"STRING", TokenType::T_STRING},
	{"ARRAY", TokenType::T_ARRAY},
	{"CHAR", TokenType::T_CHAR},
	{"BOOLEAN", TokenType::T_BOOLEAN},
	{"DATE", TokenType::T_DATE},
};
// }}}

// Token {{{

union Literal {
	Fraction<> frac;
	int64_t i64;
	/* Note that identifiers are stored as `i64`s, not `str`s */
	char *str;
	inline Literal(int64_t i){
		i64 = i;
	}
	inline Literal(int i){
		i64 = i;
	}
	inline Literal(Fraction<> res){
		frac = res;
	}
	inline Literal(const std::string_view sv){
		str = new char[sv.length()+1];
		memcpy((void *)str, sv.data(), sv.length());
		str[sv.length()] = '\0';
	}
	inline Literal(const char *s){
		str = strdup(s);
	}
};

const std::string MAX_INT_STR = std::to_string(std::numeric_limits<int64_t>::max());
const std::string MAX_FRAC_NUM_STR = std::to_string(std::numeric_limits<decltype(Literal::frac)::num_type>::max());

struct Token {

	size_t line;
	size_t col;
	TokenType type;
	Literal literal;

	~Token(){
		if(type == TokenType::STR) free(literal.str);
	}

	Token(const size_t line_, const size_t col_, TokenType type_, Literal literal_)
	: line(line_), col(col_), type(type_), literal(literal_) {
	}

	Token(const Token& other) noexcept 
		: literal(other.type == TokenType::STR ? other.literal.str : other.literal) 
	{
		line = other.line;
		col = other.col;
		type = other.type;
	}

	
	Token(Token&& other) noexcept : literal(other.literal) {
		type = other.type;
		other.type = TokenType::INVALID;
		col = other.col;
		line = other.line;
		other.literal = 0;
	}

	Token& operator=(const Token& other) noexcept {
		line = other.line;
		col = other.col;
		if(type == TokenType::STR) free(literal.str);
		type = other.type;
		if(other.type == TokenType::STR){
			literal = other.literal.str;
		} else {
			literal = other.literal;
		}
		return *this;
	}

	bool operator==(const Token& t) const noexcept {
		return t.line == line
			&& t.col == col
			&& t.type == type
			&& (t.type == TokenType::REAL ? t.literal.frac == t.literal.frac :
				(t.type == TokenType::INT || t.type == TokenType::IDENTIFIER) ? t.literal.i64 == literal.i64 :
				t.type == TokenType::STR ? strcmp(t.literal.str, literal.str) == 0 :
				t.literal.i64 == 0 ? t.type == type : false);
	}
	// Make Catch2 print our type
	friend std::ostream& operator<<(std::ostream& os, const Token& t){
		const std::string_view tokenName = tokenTypeToStr(t.type);
		os << "{ line = " << t.line
			<< ", col = " << t.col
			<< ", type = " << tokenName
			<< ", literal";
		if(t.type == TokenType::REAL){
			os << ".frac = " << t.literal.frac;
		} else if(t.type == TokenType::INT || t.type == TokenType::IDENTIFIER){
			os << ".i64 = " << t.literal.i64;
		} else if(t.type == TokenType::STR){
			// Str
			os << ".str = " << t.literal.str;

		} else if(t.literal.i64 == 0){
			/* Keyword */
			os << ".i64 = 0";
		} else {
			throw std::runtime_error("Malformed Token - this should not have happened!\n");
		}
		os << " }";
		return os;
	}
};

// }}}

// Lexer {{{

class LexError : public std::runtime_error { 
public:
	const std::string_view source;
	const size_t line;
	const size_t col;
	LexError(const std::string_view source_, size_t line_, size_t col_, const char * const msg): 
		std::runtime_error(msg), source(source_), line(line_), col(col_)
	{}
};

class Lexer {
public:
	std::string_view source;
	std::vector<Token> tokens;
	// Newline pos
	std::vector<size_t> nlpos = {0};
	int64_t identifier_count = 1;
	std::map<std::string_view, int64_t> identifiers;
private:
	size_t curr = 0, line = 1;
	inline size_t getCol() const noexcept {
		return curr - nlpos.back() + 1;
	}
	size_t getCol(const size_t pos) const noexcept {
		// The most common case we're expecting - no need to do an intensive search.
		if(pos > nlpos.back()) return pos - nlpos.back() + 1;
		else {
			// We must find the last '\n' that has position <= pos.
			// So:
			auto lastnl = std::upper_bound(nlpos.begin(), nlpos.end(), pos);
			// It is impossible for lastnl == nlpos.begin(),
			// because pos is unsigned, so it must be >= 0,
			// and for `0` upper_bound will return nlpos.begin() + 1.
			// So we don't test for that.
			lastnl--;
			return pos - *lastnl + 1;
		}
	}
	/* Assumes that you have advanced *past* '\n'.
	 * i.e. source[curr-1] == '\n'
	 */
	inline void newline() noexcept {
		line++;
		nlpos.push_back(curr);
	}

	inline char advance() noexcept {
		char x = source[curr];
		curr++;
		return x;
	}
	/* Checks if the next character
	 * is `c`, and if so,
	 * it advances.
	 */
	inline bool match(const char c) noexcept {
		if(source[curr] == c){
			curr++;
			return true;
		}
		return false;
	}
	// Add token that starts at position `start` in `source`.
	inline void addTokenStartingAt(size_t start, TokenType type, Literal lt = 0) noexcept {
		tokens.push_back({ line, getCol(start), type, lt });
	}
	inline void addToken(TokenType type, Literal lt = 0) noexcept {
		tokens.push_back({ line, getCol(curr), type, lt });
	}
	inline char peek(size_t ahead = 1) const noexcept {
		if(curr + ahead - 1 >= source.length()) return '\0';
		else return source[curr + ahead - 1];
	}
	// if we're after the last character
	inline bool noMore() const noexcept {
		return curr >= source.length();
	}
	inline bool isAlNum_(const char c) const noexcept {
		return isalnum(c) || c == '_';
	}
	void lex(){
		while(!noMore()){
			char c = advance();
			switch(c){
				case '(': addToken(TokenType::LEFT_PAREN); break;
				case ')': addToken(TokenType::RIGHT_PAREN); break;
				case '{': addToken(TokenType::LEFT_BRACE); break;
				case '}': addToken(TokenType::RIGHT_BRACE); break;
				case ',': addToken(TokenType::COMMA); break;
				case '-': addToken(TokenType::MINUS); break;
				case '+': addToken(TokenType::PLUS); break;           
				case '*': addToken(TokenType::STAR); break;
				case '=': addToken(TokenType::EQ); break;
				case '>':
					if(match('=')){
						addToken(TokenType::GT_EQ);
					} else {
						addToken(TokenType::GT);
					}
					break;
				case '<':
					if(match('=')){
						addToken(TokenType::LT_EQ);
					} else if(match('>')){
						addToken(TokenType::LT_GT);
					} else if(match('-')){
						addToken(TokenType::ASSIGN);
					} else {
						addToken(TokenType::LT);
					}
					break;
				case '/':
					/* comment or division */
					if(match('/')){
						// comment until end of line
						while(peek() != '\n' && !noMore()) advance();
					} else {
						// division
						addToken(TokenType::SLASH);
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
					nlpos.push_back(curr);
					break;
				case '"': {
					// string
					const size_t start = curr - 1;
					while(peek() != '"' && !noMore()){
						if(match('\n')) newline();
						if(peek() == '\0'){
							// invalid null character
							throw LexError(source, line, getCol(), "Invalid null character");
						}
						advance();
					}
					if(!match('"')){
						// This is only possible if
						// noMore() is triggered
						throw LexError(source, line, getCol(),
								"Unexpected end-of-file: string not finished");
					}
					addTokenStartingAt(start, TokenType::STR, source.substr(start+1, curr - start - 2));
					break;
				}
				default:
					if(isdigit(c)) {
						// Integer or Real
						const size_t start = curr - 1;
						while(isdigit(peek())) advance();
						if(peek() == '.' && isdigit(peek(2))){
							// real
							advance();
							while(isdigit(peek())) advance();
							// check if it's too large to fit inside Fraction
#define TOO_LARGE() throw LexError(source, line, getCol(start), "Real constant too large");
							if(curr - start >= MAX_FRAC_NUM_STR.length()){
								TOO_LARGE()
							}
#undef TOO_LARGE
							if(isalpha(peek())){
								// We won't allow stuff like
								// "x == 12.2OR TRUE"
								// or stuff like
								// 12.2e2.
								// It makes us require safeties around atof().
								throw LexError(source, line, getCol(), 
										"Unexpected character after number");
							} else {
								// (Hopefully) the above safety worked.

								// Parse fraction
								int32_t top, bot;
								{
									int32_t mul = 1;
									top = 0;
									for(size_t i = curr-1; i >= start; i--){
										if(source[i] == '.'){
											bot = mul * 10;
										} else {
											top += (int32_t)(source[i] - '0') * mul;
										}
									}
								}
								addTokenStartingAt(start, TokenType::REAL, Fraction<>(top, bot));
							}
						} else {
							// integer
							// check if it's too large 
#define TOO_LARGE() throw LexError(source, line, getCol(start), "Number constant too large");
							if(curr - start > MAX_INT_STR.length()){
								TOO_LARGE()
							} else if(curr - start == MAX_INT_STR.length()){
								// the annoying case
								size_t i;
								for(i = 0; i < curr - start; i++){
									if(MAX_INT_STR[i] != source[start + i]){
										if(MAX_INT_STR[i] < source[start+i]){
											TOO_LARGE()
										} else break;
									}
								}
								if(i == curr - start) TOO_LARGE()
							}
#undef TOO_LARGE
							// no alpha characters after an integer, either
							if(isalpha(peek())){
								throw LexError(source, line, getCol(),
										"Unexpected character after number");
							} else {
								addTokenStartingAt(start, TokenType::INT, atoi(source.data() + start));
							}
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
							addTokenStartingAt(start, reservedWords.at(name));
						} else if(types.find(name) != types.end()){
							// type
							addTokenStartingAt(start, types.at(name));
						} else {
							// it's a variable name

							// (so we store it as an i64)
							if(identifiers.count(name)){
								// Already exists with a name, so add it
								addTokenStartingAt(start, TokenType::IDENTIFIER, identifiers[name]);
							} else {
								// First time we're seeing this identifier
								addTokenStartingAt(start, TokenType::IDENTIFIER, identifier_count);
								identifiers[name] = identifier_count;
								identifier_count++;
							}
						}
					} else {
						// invalid character
						throw LexError(source, line, getCol(),
								"Invalid character");
					}
					break;
			}
		}
	}

public:
	Lexer(const std::string_view sv) : source(sv) {
		lex();
	}
};

// }}}

#endif /* LEXER_HPP */
