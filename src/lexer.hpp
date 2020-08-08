#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <cstdlib>
#include <map>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <cstring>

#include "fraction.hpp"
#include "utils.hpp"
#include "date.hpp"
#include "globals.hpp"

// Token list {{{

// not allowed to put directly

// We will use X-Macros
// to make the enum <-> string conversion easier.
#define TOKENTYPE_LIST \
	TOK(LEFT_PAREN) \
	TOK(RIGHT_PAREN) \
	TOK(LEFT_SQ) \
	TOK(RIGHT_SQ) \
	TOK(COMMA) \
	TOK(MINUS) OP(MINUS, -)\
	TOK(PLUS) OP(PLUS, +) \
	TOK(SLASH) OP(SLASH, /) \
	TOK(STAR) OP(STAR, *) \
	TOK(COLON) \
	TOK(ASSIGN) \
	TOK(EQ) OP(EQ, =) \
	TOK(LT_GT) OP(LT_GT, <>) \
	TOK(GT) OP(GT, >) \
	TOK(GT_EQ) OP(GT_EQ, >=) \
	TOK(LT) OP(LT, <) \
	TOK(LT_EQ) OP(LT_EQ, <=) \
	TOK(AND) RESERVED(AND) OP(AND, AND)\
	TOK(OR) RESERVED(OR) OP(OR, OR)\
	TOK(NOT) RESERVED(NOT) OP(NOT, NOT)\
	TOK(IF)	RESERVED(IF)\
	TOK(THEN)	RESERVED(THEN)\
	TOK(ELSE)	RESERVED(ELSE)\
	TOK(ENDIF)	RESERVED(ENDIF)\
	TOK(DECLARE)	RESERVED(DECLARE)\
	TOK(FOR)	RESERVED(FOR)\
	TOK(TO)	RESERVED(TO)\
	TOK(STEP)	RESERVED(STEP)\
	TOK(NEXT)	RESERVED(NEXT)\
	TOK(WHILE)	RESERVED(WHILE)\
	TOK(DO) RESERVED(DO)\
	TOK(ENDWHILE)	RESERVED(ENDWHILE)\
	TOK(REPEAT)	RESERVED(REPEAT)\
	TOK(UNTIL)	RESERVED(UNTIL)\
	TOK(CONSTANT)	RESERVED(CONSTANT)\
	TOK(INPUT)	RESERVED(INPUT)\
	TOK(OUTPUT)	RESERVED(OUTPUT)\
	TOK(CASE)	RESERVED(CASE)\
	TOK(OF)	RESERVED(OF)\
	TOK(OTHERWISE)	RESERVED(OTHERWISE)\
	TOK(ENDCASE)	RESERVED(ENDCASE)\
	TOK(PROCEDURE)	RESERVED(PROCEDURE)\
	TOK(BYREF)	RESERVED(BYREF)\
	TOK(ENDPROCEDURE)	RESERVED(ENDPROCEDURE)\
	TOK(CALL)	RESERVED(CALL)\
	TOK(FUNCTION)	RESERVED(FUNCTION)\
	TOK(RETURNS)	RESERVED(RETURNS)\
	TOK(RETURN)	RESERVED(RETURN)\
	TOK(ENDFUNCTION)	RESERVED(ENDFUNCTION)\
	TOK(INTEGER) RESERVED(INTEGER) TYPE(INTEGER)\
	TOK(REAL) RESERVED(REAL) TYPE(REAL)\
	TOK(STRING) RESERVED(STRING) TYPE(STRING)\
	TOK(ARRAY) RESERVED(ARRAY) TYPE(ARRAY)\
	TOK(CHAR) RESERVED(CHAR) TYPE(CHAR)\
	TOK(BOOLEAN) RESERVED(BOOLEAN) TYPE(BOOLEAN)\
	TOK(DATE) RESERVED(DATE) TYPE(DATE)\
	TOK(TRUE) RESERVED(TRUE)\
	TOK(FALSE) RESERVED(FALSE)\
	TOK(MOD) RESERVED(MOD) OP(MOD, MOD)\
	TOK(DIV) RESERVED(DIV) OP(DIV, DIV)\
	/* special */ \
	TOK(IDENTIFIER)\
	TOK(STR_C)\
	TOK(INT_C)\
	TOK(REAL_C)\
	TOK(CHAR_C)\
	TOK(DATE_C)\
	TOK(INVALID)

// }}}

// TokenType data structures {{{

/* default */
#define RESERVED(a) /* nothing */
#define TYPE(a) /* nothing */
#define TOK(a) /* nothing */
#define OP(a, b) /* nothing */

enum class TokenType {
#undef TOK
#define TOK(a) a,
	TOKENTYPE_LIST
#undef TOK
#define TOK(a) /* nothing */
	LENGTH
};

const size_t TOKENTYPE_LENGTH = static_cast<int>(TokenType::LENGTH);

const std::vector<std::string_view> TokenTypeStrTable = {
#undef TOK
#define TOK(a) #a,
	TOKENTYPE_LIST
#undef TOK
#define TOK(a) /* nothing */
};

inline std::string_view tokenTypeToStr(const TokenType type) noexcept {
	return TokenTypeStrTable[static_cast<int>(type)];
}

const std::map<std::string_view, TokenType> reservedWords {
#undef RESERVED
#define RESERVED(a) { #a, TokenType:: a },
	TOKENTYPE_LIST
#undef RESERVED
#define RESERVED(a) /* nothing */
};

const std::vector<bool> is_reserved_word = [](){
	std::vector<bool> res(TOKENTYPE_LENGTH, false);
	for(const auto& x : reservedWords){
		res[static_cast<int>(x.second)] = true;
	}
	return res;
}();

inline bool isReservedWord(const TokenType type) noexcept {
	return is_reserved_word[static_cast<int>(type)];
}

const std::vector<TokenType> type_keywords = {
#undef TYPE
#define TYPE(a) TokenType:: a,
	TOKENTYPE_LIST
#undef TYPE
#define TYPE(a) /* nothing */
};

const std::map<TokenType, std::string_view> op_to_str {
#undef OP
#define OP(a, b) { TokenType:: a, #b },
	TOKENTYPE_LIST
#undef OP
#define OP(a, b) /* nothing */
};

inline std::string_view opToStr(const TokenType type){
	return op_to_str.at(type);
}

inline bool isTypeKeyword(const TokenType type) noexcept {
	return std::find(type_keywords.begin(), type_keywords.end(), type) != type_keywords.end();
}

const std::string MAX_FRAC_NUM_STR = std::to_string(std::numeric_limits<Fraction<>::num_type>::max());
const std::string MAX_INT_STR = std::to_string(std::numeric_limits<int64_t>::max());

// }}}

// Token {{{

struct Token {
	size_t line, col;
	TokenType type;
	union Literal {
		std::string_view str;
		int64_t i64;
		Fraction<> frac;
		char c;
		Date date;
		Literal(std::string_view str_): str(str_) {}
		Literal(int64_t i): i64(i) {}
		Literal(int i): i64(i) {}
		Literal(Fraction<> frac_): frac(frac_) {}
		Literal(const char *p) : str(p) {}
		Literal(const char c_) : c(c_) {}
		Literal(uint8_t day, uint8_t month, uint16_t year): date(day, month, year) {}
		Literal(Date d): date(d) {}
	} literal;
	Token(size_t line_, size_t col_, TokenType type_, Literal lit_) :
		line(line_), col(col_), type(type_), literal(lit_) {}
	bool operator==(const Token& other) const noexcept {
		return line == other.line
			&& col == other.col
			&& type == other.type
			&& (type == TokenType::REAL_C ? literal.frac == other.literal.frac :
				type == TokenType::INT_C || type == TokenType::IDENTIFIER ? literal.i64 == other.literal.i64 :
				type == TokenType::STR_C ? literal.str == other.literal.str :
				type == TokenType::CHAR_C ? literal.c == other.literal.c :
				/* has to be a reserved word or token, so is true */ true);
	}
	/* Make Catch2 print our type */
	friend std::ostream& operator<<(std::ostream& os, const Token& tok) noexcept {
		os << "{ line = " << tok.line
			<< ", col = " << tok.col
			<< ", type = " << tokenTypeToStr(tok.type) 
			<< ", literal";
		if(tok.type == TokenType::REAL_C){
			os << ".frac = " << tok.literal.frac;
		
		} else if(tok.type == TokenType::STR_C){
			// Str
			os << ".str = " << tok.literal.str;
		} else if(tok.type == TokenType::CHAR_C){
			os << ".c = " << tok.literal.c;
		} else {
			// tok.type == TokenType::INT_C || t.type == TokenType::IDENTIFIER or is reserved word
			os << ".i64 = " << tok.literal.i64;
		}
		os << "}\n";
		return os;
	}
};

// }}}

const Token invalid_token(0, 0, TokenType::INVALID, 0);

// Lexer {{{

class LexError : public std::runtime_error {
public:
	size_t pos, line, col;
	template<typename T>
	LexError(size_t pos_, size_t line_, size_t col_, const T msg): std::runtime_error(msg), pos(pos_), line(line_), col(col_) {}
};

class Lexer {
public:
	std::istream& in;
	std::vector<Token> output;
	std::vector<size_t> line_loc;
	int64_t identifier_count = 0;
	std::map<std::string_view, int64_t> id_num;
protected:
	size_t line = 1;
	size_t curr = 0;

	/* <=DATE CAPTURING=>
	 * To capture dates, the Lexer
	 * turns anything of the form INT_C SLASH INT_C SLASH INT_C
	 * into a Date.
	 * There is no other way to avoid the ambiguity 
	 * without being excessively complicated,
	 * and really, why would anyone write 1 / 2 / 3 ? just write (1/2)/3
	 *
	 * This date_stage variable captures the "progress".
	 * As the tokens are generated, this happens to date_stage:
	 * date_stage | 0     1     2     3     4     5
	 * tokens     | (any) INT_C SLASH INT_C SLASH INT_C
	 * If anything gets in the way, date_stage will be reset to 0.
	 * If it's five, all the INT_C's and SLASH's will get removed, 
	 * and a DATE_C inserted in their place.
	 */
	uint_least8_t date_stage = 0;

	// done()/peek()/next() like functions {{{
	inline size_t getCol() const  {
		return (line_loc.empty() ? curr : curr - line_loc.back() - 1);
	}
	size_t getCol(size_t pos) const  {
		if(line_loc.empty()) return pos + 1;
		// most common case
		else if(pos >= line_loc.back()) return pos - line_loc.back();
		else {
			// Otherwise, we have to find the last '\n' that has position <= pos.
			auto lastnl = std::upper_bound(line_loc.begin(), line_loc.end(), pos);
			if(lastnl == line_loc.begin()){
				// was before any newline
				return pos + 1;
			} else {
				--lastnl;
				return pos - *lastnl;
			}
		}
	}
	inline void emit(TokenType type)  {
		output.emplace_back(line, getCol(), type, 0);
	}
	inline void emit(TokenType type, Token::Literal lt)  {
		output.emplace_back(line, getCol(), type, lt);
	}
	inline void emit(TokenType type, Token::Literal lt, size_t startpos)  {
		output.emplace_back(line, getCol(startpos), type, lt);
	}
	inline bool done() const  {
		return in.eof();
	}
	inline char peek() const  {
		return in.eof() ? '\0' : in.peek();
	}
	inline char next()  {
		char c;
		in.get(c);
		if(in.eof()){
			c = '\0';
		} else {
			++curr;
		}
		return c;
	}
	inline bool match(const char c)  {
		if(c == peek()){
			next();
			return true;
		} else return false;
	}
	template<typename T>
		inline void error(const T msg) const{
			throw LexError(curr-1, line, getCol(), msg);
		}
	inline void expect(char c){
		if(c != next()){
			std::string msg = "Expected ";
			msg += c;
			error(msg);
		}
	}
	// }}}

	// newline, number, string, identifier {{{
	inline void newline()  {
		line_loc.push_back(curr - 1);
		line++;
	}
	inline void number(char c){
		// Integer or Real
		const size_t start = curr - 1;
		std::string numstr;
		numstr += c;
		numstr.reserve(std::max(MAX_FRAC_NUM_STR.length(), MAX_INT_STR.length()));
		while(isDigit(peek())) numstr.push_back(next());
		if(peek() == '.'){
			// Real
			numstr.push_back(next());
			if(!isDigit(peek())){
				error("Expected digit after decimal point");
			}
			numstr.push_back(next());
			while(isDigit(peek())) numstr.push_back(next());
			if(numstr.size() >= MAX_FRAC_NUM_STR.length()){
				error("Real constant too large");
			}
			if(isAlpha(peek())){
				// We won't allow 12.2e2.
				// Makes for confusion.
				next();
				error("Unexpected character after number");
			}
			// parse fraction
			emit(TokenType::REAL_C, Fraction<>::fromValidStr(numstr), start);
		} else {
			// Integer
			if(isAlpha(peek())){
				// 12e2 is not allowed.
				next();
				error("Unexpected character after number");
			}
			// check if is too big
			// (stupid check so that math using it doesn't overflow easily)
			if(numstr.size() >= MAX_INT_STR.length()){
				error("Integer constant too large");
			}
			// parse integer
			int64_t res = 0;
			{
				int64_t mul = 1;
				for(ssize_t i = numstr.size()-1; i >= (ssize_t)0; i--){
					res += (int64_t)(numstr[i] - '0') * mul;
					mul *= 10;
				}
			}
			emit(TokenType::INT_C, res, start);
		}
	}
	inline void string(){
		const size_t start = curr - 1;
		std::string res;
		while(!done() && peek() != '"'){
			res += next();
			if(res.back() == '\n') newline();
		}
		// will throw if the string is incomplete
		expect('"');
		emit(TokenType::STR_C, global::toStrView(res), start);
	}
	inline void identifier(char c){
		const size_t start = curr-1;
		std::string id;
		id += c;
		while(isAlpha(peek()) || peek() == '_') id.push_back(next());
		if(reservedWords.find(id) != reservedWords.end()){
			emit(reservedWords.at(id), 0, start);
		} else {
			int64_t idn = identifier_count + 1;
			auto it = id_num.find(id);
			if(it != id_num.end()){
				idn = it->second;
			} else {
				id_num.insert(it, { global::toStrView(id), idn });
				identifier_count++;
			}
			emit(TokenType::IDENTIFIER, idn, start);
		}
	}
	// }}}

	// lex {{{	
	void lex(){
		char c;
		while((c = next()) && !done()){
			switch(c){
				case '(': emit(TokenType::LEFT_PAREN); break;
				case ')': emit(TokenType::RIGHT_PAREN); break;
				case '[': emit(TokenType::LEFT_SQ); break;
				case ']': emit(TokenType::RIGHT_SQ); break;
				case ',': emit(TokenType::COMMA); break;
				case '-': emit(TokenType::MINUS); break;
				case '+': emit(TokenType::PLUS); break;
				case '\'':
					{
						char c = next();
						emit(TokenType::CHAR_C, c, curr - 2);
						expect('\'');
					}
					break;
				case '/': 
					if(match('/')){
						// comment
						while(peek() != '\n') next();
						next();
						newline();
					} else {
						emit(TokenType::SLASH);
					}
					break;
				case '*': emit(TokenType::STAR); break;
				case ':': emit(TokenType::COLON); break;
				case '=': emit(TokenType::EQ); break;
				case '<':
					if(match('-')) emit(TokenType::ASSIGN, 0, curr - 2);
					else if(match('=')) emit(TokenType::LT_EQ, 0, curr - 2);
					else if(match('>')) emit(TokenType::LT_GT, 0, curr - 2);
					else emit(TokenType::LT);
					break;
				case '>':
					if(match('=')) emit(TokenType::GT_EQ, 0, curr - 2);
					else emit(TokenType::GT);
					break;
				case ' ':
				case '\r':
				case '\t':
					// ignore whitespace
					break;
				case '\n':
					newline();
					break;
				case '"':
					string();
					break;
				default:
					if(isDigit(c))
						number(c);
					else if(isAlpha(c))
						identifier(c);
					else {
						std::string msg = "Stray ";
						msg += c;
						msg += " in program";
						error(msg);
					}
					break;

			}
			if(output.size()){
				/* See <=DATE CAPTURING=>. */
				if(date_stage % 2 == 0 && output.back().type == TokenType::INT_C){
					date_stage++;
					if(date_stage == 5){
						// A Date token should replace the rest.
						auto it = output.end() - 5;
						const size_t line = it->line,
							  col = it->col;
						const size_t day = it->literal.i64;
						++it; ++it;
						const size_t month = it->literal.i64;
						++it; ++it;
						const size_t year = it->literal.i64;
						output.resize(output.size() - 5, invalid_token);
#define MAX_VAL(x) std::numeric_limits<decltype(Date:: x)>::max()
						if(day > MAX_VAL(day) || month > MAX_VAL(month) || year > MAX_VAL(year)){
							error("Too large Date constant. Note: if you mean to specify division, use parentheses");
						}
#undef MAX_VAL
						try {
							output.emplace_back(line, col, TokenType::DATE_C, Date(day, month, year));
						} catch(DateError& e){
							error(e.what());
						}
						date_stage = 0;
					}
				} else if(date_stage % 2 == 1 && output.back().type == TokenType::SLASH){
					date_stage++;
				} else {
					date_stage = 0;
				}
			}
		}
	}
	// }}}
public:
	inline Lexer(std::istream& src): in(src) {
		in.exceptions(std::istream::badbit);
		lex();
		// add an EOF token
		emit(TokenType::INVALID, 0, curr);
	}
};

// }}}

#endif /* LEXER_HPP */
