#include <string>
#include <map>
#include <vector>
#include "lexer.hpp"

class ParseError : public std::runtime_error {
public:
	const Token token;
	template<typename T>
	ParseError(const Token& tok, const T msg) :
		std::runtime_error(msg), token(tok)
	{}
};

const Token invalid_token(0, 0, TokenType::INVALID, 0);

template<bool TopLevel>
class Stmt;

class Program;

// Parser {{{

class Parser {
public:
	std::vector<Token> tokens;
	Program *output;
	size_t curr = 0;
	inline Parser(const std::vector<Token> tokens_) : tokens(tokens_) { parse(); }
	inline Parser(const std::vector<Token>&& tokens_) : tokens(std::move(tokens_)) { parse(); }
	inline bool done() const noexcept {
		return curr >= tokens.size();
	}
	inline const Token& peek() const noexcept {
		return done() ? invalid_token : tokens[curr];
	}
	inline const Token& next()  noexcept {
		const Token& n = peek();
		++curr;
		return n;
	}
	/* Returns true and advances if any of the arguments
	 * match `peek()`. */
	template<typename... Args>
	inline bool match(Args... args) noexcept {
		const Token& n = peek();
		if(((n == args) && ...)){
			++curr;
			return true;
		} else return false;
	}
	/* Returns true and advances if any of the arguments
	 * match `peek().type`.
	 */
	template<typename... Args>
	inline bool match_type(Args... args) noexcept {
		const Token& n = peek();
		if(((n.type == args) && ...)){
			++curr;
			return true;
		} else return false;
	}
	template<typename T>
	[[noreturn]] inline void error(const T msg) const {
		throw ParseError(tokens[curr-1], msg);
	}
	inline void expect_type(TokenType type){
		if(!match_type(type)){
			std::string msg = "Expected ";
			msg += tokenTypeToStr(type);
			error(msg);
		}
	}

	inline const Token& expect_type_r(TokenType type){
		const Token& n = peek();
		expect_type(type);
		return n;
	}

	void parse();
};

// }}}

// grammar.ebnf describes the idealised syntax.

// Expr, Type, Param {{{

template<uint16_t Level>
class BinExpr;

using Expr = BinExpr<0>;

class Primary {
public:
	const struct All {
		TokenType primtype;
		union Main {
			Token::Literal lt;
			Expr *expr;
			inline Main(Token::Literal lt_) : lt(lt_) {}
			inline Main(int i): lt(i) {}
		} main;
	} all;
	All::Main main() const noexcept { return all.main; }
	TokenType primtype() const noexcept { return all.primtype; }
	inline bool is_constant() const noexcept {
		return all.primtype == TokenType::STR_C
			|| all.primtype == TokenType::INT_C
			|| all.primtype == TokenType::REAL_C;
	}
	static All make_all(Parser& p);
	Primary(Parser& p) : all(make_all(p)) {}
};

static const std::vector<TokenType> unary_ops = {
	TokenType::NOT,
	TokenType::MINUS
};

class UnaryExpr {
public:
	const TokenType op;
	const Primary main;
	static TokenType make_op(Parser& p) {
		for(const auto type : unary_ops){
			if(p.match_type(type)){
				return type;
			}
		}
		return TokenType::INVALID;
	}
	inline bool is_constant() const noexcept {
		return main.is_constant();
	}
	UnaryExpr(Parser& p) : op(make_op(p)), main(Primary(p)) {}
};

static const std::vector<TokenType> binary_ops[] = {
	{ TokenType::OR },
	{ TokenType::AND },
	{ TokenType::EQ, TokenType::GT, TokenType::LT, TokenType::GT_EQ, TokenType::LT_EQ, TokenType::LT_GT },
	{ TokenType::PLUS, TokenType::MINUS },
	{ TokenType::STAR, TokenType::SLASH, TokenType::MOD, TokenType::DIV },
};

const uint16_t MAX_BINARY_LEVEL = 4;

// TODO: implement function call
// (I FORGOT!!!)

template<uint16_t Level>
class BinExpr {
	static_assert(Level <= MAX_BINARY_LEVEL);
public:
	using LowerExpr = typename std::conditional<(Level < MAX_BINARY_LEVEL), BinExpr<Level+1>, UnaryExpr>::type;

	// Must always be read like: exprs[0], ops[0], exprs[1], ops[1], exprs[2], ...
	std::vector<TokenType> ops;
	std::vector<LowerExpr> exprs;
	BinExpr(Parser& p){
		for(;;){
			// parse left
			exprs.emplace_back(p);
			// parse operator
			TokenType op = TokenType::INVALID;
			for(const auto type : binary_ops[Level]){
				if(p.match_type(type)){
					op = type;
					break;
				}
			}
			if(op == TokenType::INVALID){
				/* Return now, we're done. */
				return;
			} else {
				ops.push_back(op);
				/* Expect the loop to continue to parse the next
				 * LowerExpr, so don't do anything.
				 */
			}
		}
	}
};

Primary::All Primary::make_all(Parser& p){
	All res = {TokenType::INVALID, 0};
	const Token& n = p.next();
	if(isAnyOf(n.type, 
				TokenType::REAL_C, TokenType::INT_C, TokenType::STR_C,
				TokenType::TRUE, TokenType::FALSE, TokenType::IDENTIFIER)){
		res.primtype = n.type;
		res.main = n.literal;
		return res;
	} else if(n.type == TokenType::LEFT_PAREN){
		res.main.expr = new Expr(p);
		p.expect_type(TokenType::RIGHT_PAREN);
		return res;
	} else {
		p.error("Invalid primary");
	}
}

class Type {
public:
	const struct All {
		bool is_array;
		Expr *start, *end;
		TokenType name;
	} all;
	inline bool is_array(){ return all.is_array; }
	inline const Expr *start(){ return all.start; }
	inline const Expr *end(){ return all.end; }
	inline TokenType name(){ return all.name; }
	static All make_all(Parser& p){
		All res;
		if(p.match_type(TokenType::ARRAY)){
			res.is_array = true;
			p.expect_type(TokenType::LEFT_SQ);
			res.start = new Expr(p);
			p.expect_type(TokenType::COLON);
			res.end = new Expr(p);
			p.expect_type(TokenType::RIGHT_SQ);
		}
		for(const auto type : type_keywords){
			if(p.match_type(type)){
				res.name = type;
				return res;
			}
		}
		p.error("Invalid type name");
	}
	Type(Parser& p): all(make_all(p)) {}
};

class Param {
public:
	bool byref;
	int64_t ident;
	Type type;
	static bool make_byref(Parser& p){
		return p.match_type(TokenType::BYREF);
	}
	static int64_t make_ident(Parser& p){
		if(p.peek().type != TokenType::IDENTIFIER){
			/* will show error */
			p.expect_type(TokenType::IDENTIFIER);
		}
		return p.next().literal.i64;
	}
	Param(Parser& p) : byref(make_byref(p)), ident(make_ident(p)), type(p) {}
};

// }}}

// Program, Block, Stmt {{{

enum class StmtForm {
	DECLARE,
	CONSTANT,
	PROCEDURE,
	FUNCTION,
	ASSIGN,
	INPUT,
	OUTPUT,
	IF,
	CASE,
	FOR,
	REPEAT,
	WHILE,
	CALL
};

const std::vector<TokenType> valid_stmt_starts = {
	TokenType::DECLARE,
	TokenType::CONSTANT,
	TokenType::PROCEDURE,
	TokenType::FUNCTION,
	TokenType::IDENTIFIER,
	TokenType::INPUT,
	TokenType::OUTPUT,
	TokenType::IF,
	TokenType::CASE,
	TokenType::FOR,
	TokenType::REPEAT,
	TokenType::WHILE,
	TokenType::CALL,
};

bool isValidStmtStart(const TokenType type) noexcept {
	for(const auto t : valid_stmt_starts){
		if(t == type) return true;
	} 
	return false;
}

class Block {
public:
	std::vector<Stmt<false>> stmts;
	Block(Parser& p){
		while(isValidStmtStart(p.peek().type)){
			stmts.emplace_back(p);
		}
	}
};

class Program {
public:
	std::vector<Stmt<true>> stmts;
	Program(Parser& p){
		while(!p.done()){
			stmts.emplace_back(p);
		}
	}
};

template<bool TopLevel>
class Stmt {
public:
	StmtForm form;
	std::vector<int64_t> ids;
	std::vector<Expr> exprs;
	std::vector<Type> types;
	std::vector<Param> params;
	std::vector<Block> blocks;
	void paramlist(Parser& p){
		for(;;){
			params.emplace_back(p);
			if(!p.match_type(TokenType::COMMA)){
				break;
			}
			/* start parsing next param on loop repeat */
		}
	}
	void exprlist(Parser& p){
		for(;;){
			exprs.emplace_back(p);
			if(!p.match_type(TokenType::COMMA)){
				break;
			}
		}
	}
#define CASE(x) case TokenType:: x: form = StmtForm:: x;
#define CONSUME_ID() p.expect_type_r(TokenType::IDENTIFIER).literal.i64
	void stmt(Parser& p, const Token& t){
		switch(t.type){
			case TokenType::IDENTIFIER: form = StmtForm::ASSIGN;
				ids.push_back(t.literal.i64);
				p.expect_type(TokenType::ASSIGN);
				exprs.emplace_back(p);
				break;
			CASE(INPUT)
				ids.push_back(CONSUME_ID());
				break;
			CASE(OUTPUT)
				exprlist(p);
				break;
			CASE(IF)
				exprs.emplace_back(p);
				p.expect_type(TokenType::THEN);
				blocks.emplace_back(p);
				if(p.match_type(TokenType::ELSE)){
					blocks.emplace_back(p);
				}
				p.expect_type(TokenType::ENDIF);
				break;
			CASE(CASE) // lmao
				p.expect_type(TokenType::OF);
				ids.push_back(CONSUME_ID());
				for(;;){
					exprs.emplace_back(p);
					p.expect_type(TokenType::COLON);
					blocks.emplace_back(p);
					if(p.match_type(TokenType::OTHERWISE)){
						blocks.emplace_back(p);
						p.expect_type(TokenType::ENDCASE);
						break;
					}
					if(p.match_type(TokenType::ENDCASE)){
						break;
					}
				}
				break;
			CASE(FOR)
				ids.push_back(CONSUME_ID());
				p.expect_type(TokenType::ASSIGN);
				exprs.emplace_back(p);
				p.expect_type(TokenType::TO);
				exprs.emplace_back(p);
				if(p.match_type(TokenType::STEP)){
					exprs.emplace_back(p);
				}
				blocks.emplace_back(p);
				p.expect_type(TokenType::NEXT);
				break;
			CASE(REPEAT)
				blocks.emplace_back(p);
				p.expect_type(TokenType::UNTIL);
				exprs.emplace_back(p);
				break;
			CASE(WHILE)
				exprs.emplace_back(p);
				p.expect_type(TokenType::DO);
				blocks.emplace_back(p);
				p.expect_type(TokenType::ENDWHILE);
				break;
			CASE(CALL)
				ids.push_back(CONSUME_ID());
				if(p.match_type(TokenType::LEFT_PAREN)){
					paramlist(p);
				}
				p.expect_type(TokenType::RIGHT_PAREN);
				break;
			default:
				p.error("Invalid start of statement");
		}
	}
	void stmt(Parser& p){
		const Token& t = p.next();
		stmt(p, t);
	}
	void topstmt(Parser& p){
		const Token& t = p.next();
		switch(t.type){
			CASE(DECLARE)
				ids.push_back(CONSUME_ID());
				p.expect_type(TokenType::COLON);
				types.emplace_back(p);
				break;
			CASE(CONSTANT)
				// Only push back id if there actually is one, otherwise error
				ids.push_back(CONSUME_ID());
				p.expect_type(TokenType::EQ);
				exprs.emplace_back(p);
				break;
			CASE(PROCEDURE)
				ids.push_back(CONSUME_ID());
				if(p.match_type(TokenType::LEFT_PAREN)){
					paramlist(p);	
					p.expect_type(TokenType::RIGHT_PAREN);
				}
				blocks.emplace_back(p);
				p.expect_type(TokenType::ENDPROCEDURE);
				break;
			CASE(FUNCTION)
				ids.push_back(CONSUME_ID());
				if(p.match_type(TokenType::LEFT_PAREN)){
					paramlist(p);
					p.expect_type(TokenType::RIGHT_PAREN);
				}
				p.expect_type(TokenType::RETURNS);
				types.emplace_back(p);
				blocks.emplace_back(p);
				p.expect_type(TokenType::ENDFUNCTION);
				break;
			default:
				stmt(p, t);
		}
	}
#undef CASE
#undef CONSUME_ID
	Stmt(Parser& p){
		if constexpr (TopLevel){
			topstmt(p);
		} else {
			stmt(p);
		}
	}
};

// }}}

// Parser::parse {{{

void Parser::parse(){
	output = new Program(*this);
}

// }}}
