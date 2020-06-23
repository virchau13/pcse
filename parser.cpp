#include <string>
#include <map>
#include <vector>
#include "lexer.hpp"

class ParseError : public std::runtime_error {
public:
	const Token token;
	ParseError(const Token& tok, const char * const msg) :
		std::runtime_error(msg), token(tok)
	{}
};


// CheckedIterator {{{

class DerefError : public std::exception {
public:
};

/* A iterator that throws a DerefError on invalid dereference. */
template<typename iterator = std::vector<Token>::iterator>
class CheckedIterator {
protected:
	const iterator end;
	iterator it;
public:
	CheckedIterator() = delete;
	CheckedIterator(iterator it_, iterator end_): end(end_), it(it_) {}
	using Cit = CheckedIterator;
	template<typename T>
	inline Cit& operator+=(const T x) {
		it += x;
		return *this;
	}
	template<typename T>
	inline Cit& operator-=(const T x){
		it -= x;
		return *this;
	}
	template<typename T>
	inline Cit operator+(const T x){
		return Cit(it + x, end);
	}
	template<typename T>
	inline Cit operator-(const T x){
		return Cit(it - x, end);
	}
	inline auto operator-(const Cit x){
		return it - x.it;
	}
	inline bool operator<(const Cit x){
		return it < x.it;
	}
	inline bool operator<=(const Cit x){
		return it <= x.it;
	}
	inline bool operator>(const Cit x){
		return it > x.it;
	}
	inline bool operator>=(const Cit x){
		return it >= x.it;
	}
	inline Cit& operator--(){
		--it;
		return *this;
	}
	inline Cit& operator++(){
		++it;
		return *this;
	}
	inline const Cit operator--(int){
		Cit tmp = *this;
		--it;
		return tmp;
	}
	inline Cit& operator++(int){
		Cit tmp = *this;
		++it;
		return tmp;
	}
	auto operator*(){
		if(it >= end){
			throw DerefError();
		}
		return *it;
	}
	auto operator->(){
		if(it >= end){
			throw DerefError();
		}
		return it;
	}
	bool will_be_valid() const noexcept {
		return it < end-1;
	}

	static const CheckedIterator<iterator> invalid;
};
// }}}

// Grammar classes {{{

// See grammar.ebnf for a description of each.

// needed for Primary
template<uint8_t Level>
class BinExpr;

using Expr = BinExpr<0>;

const std::vector<TokenType> primary_inside_types = {
	TokenType::REAL,
	TokenType::INT,
	TokenType::TRUE,
	TokenType::FALSE,
	TokenType::IDENTIFIER
};

class Primary {
public:
	union Inside {
	 	Token token;
		Expr *expr;
		inline Inside(Token tok) : token(tok) {}
		inline Inside(const Expr& expr_);
		inline Inside(Expr&& expr_);
		~Inside(){}
	} inside;
	static Inside make_inside(CheckedIterator<>& it);
	Primary(CheckedIterator<>& it) : inside(make_inside(it)) {}
};

class UnaryExpr {
public:
	const Token op;
	const Primary primary;
	/* Constructs UnaryExpr from position `it`, and advances it past the UnaryExpr. */
	static Token make_op(CheckedIterator<>& it) {
		if(it->type != TokenType::NOT && it->type != TokenType::MINUS){
			throw ParseError(*it, "Invalid unary operator");
		}
		const Token tmp = *it;
		++it;
		return tmp;
	}
	static Primary make_primary(CheckedIterator<>& it){
		return Primary(it);
	}
	UnaryExpr(CheckedIterator<>& it) 
		: op(make_op(it)), primary(make_primary(it)) 
	{}
};

const uint8_t MAX_BINARY_LEVEL = 4;

static const std::vector<TokenType> binary_ops[] = {
	{ TokenType::OR },
	{ TokenType::AND },
	{ TokenType::EQ, TokenType::GT, TokenType::LT, TokenType::GT_EQ, TokenType::LT_EQ, TokenType::LT_GT },
	{ TokenType::PLUS, TokenType::MINUS },
	{ TokenType::STAR, TokenType::SLASH, TokenType::MOD, TokenType::DIV },
};

template<uint8_t Level>
class BinExpr {
	static_assert(Level <= MAX_BINARY_LEVEL);
public:
	using LowerExpr = std::conditional<(Level < MAX_BINARY_LEVEL), BinExpr<Level+1>, UnaryExpr>;
	
	// Must always be read like: exprs[0], ops[0], exprs[1], ops[1], exprs[2], ...
	std::vector<Token> ops;
	std::vector<LowerExpr> exprs;
	static bool is_valid_operator(CheckedIterator<>& it){
		for(const auto x : binary_ops[Level]){
			if(it->type == x) return true;
		}
		return false;
	}
	/* Constructs BinExpr from position `it`, and advances it past the BinExpr. */
	BinExpr(CheckedIterator<>& it) {
		for(;;){
			// parse left
			exprs.push_back(LowerExpr(it));
			// parse operator
			// see if there is an operator and break if there isn't
			if(!is_valid_operator(it)) break;
			++it;
			// we expect another operator to come up, so just let the loop continue
		}
	}
	BinExpr(const BinExpr<Level>& expr): ops(expr.ops), exprs(expr.exprs) {}
	BinExpr(BinExpr<Level>&& expr) noexcept : ops(std::move(expr.ops)), exprs(std::move(expr.exprs)) {}
};

Primary::Inside::Inside(const Expr& expr_) : expr(new Expr(expr_)) {}
Primary::Inside::Inside(Expr&& expr_) : expr(new Expr(expr_)) {}

Primary::Inside Primary::make_inside(CheckedIterator<>& it){
	for(const auto type : primary_inside_types){
		if(it->type == type){
			return *it;
		}
	}
	if(it->type == TokenType::LEFT_PAREN){
		++it;
		Expr expr(it);
		if(it->type != TokenType::RIGHT_PAREN){
			throw ParseError(*it, "Expected right paren!");
		}
	}
	throw ParseError(*it, "Invalid identifier or constant");
}

template<bool TopLevel>
class Block {
};

// }}}
