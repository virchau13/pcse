#ifndef PARSER_HPP
#define PARSER_HPP
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

template<bool TopLevel>
class Stmt;

std::ostream& operator<<(std::ostream& os, const Stmt<true>& stmt) noexcept;
std::ostream& operator<<(std::ostream& os, const Stmt<false>& stmt) noexcept;

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
	// LL(1) :D
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

// Expr, Type, Param, Primary {{{

template<uint16_t Level>
class BinExpr;

using Expr = BinExpr<0>;
std::ostream& operator<<(std::ostream& os, const Expr& expr) noexcept;

class LValue {
public:
	int64_t id;
	Expr *expr = nullptr;
	LValue(Parser& p, int64_t id = 0);
};

class Primary {
public:
	const struct All {
		// Valid values: 
		// STR_C, INT_C, REAL_C, CHAR_C, IDENTIFIER [lvalue], 
		// TRUE, FALSE, CALL [function call], INVALID [(expr)]
		TokenType primtype;
		int64_t func_id;
		union Main {
			LValue lvalue;
			Token::Literal lt;
			Expr *expr;
			std::vector<Expr> *args;
			inline Main(Token::Literal lt_) : lt(lt_) {}
			inline Main(int i): lt(i) {}
		} main;
	} all;
	All::Main main() const noexcept { return all.main; }
	TokenType primtype() const noexcept { return all.primtype; }
	inline bool is_constant() const noexcept {
		return isAnyOf(all.primtype,
				TokenType::STR_C, TokenType::INT_C, TokenType::REAL_C,
				TokenType::CHAR_C, TokenType::TRUE, TokenType::FALSE);
	}
	static All make_all(Parser& p);
	Primary(Parser& p) : all(make_all(p)) {}
	// friend operator<< {{{
	/* make easier to debug */
	friend std::ostream& operator<<(std::ostream& os, const Primary& p) noexcept {
		os << '{';
		switch(p.primtype()){
#define CASE(x) case TokenType:: x
			CASE(STR_C):
				os << '"' << p.main().lt.str << '"';
				break;
			CASE(INT_C):
				os << p.main().lt.i64;
				break;
			CASE(REAL_C):
				os << p.main().lt.frac;
				break;
			CASE(CHAR_C):
				os << '\'' << p.main().lt.c << '\'';
				break;
			CASE(TRUE):
			CASE(FALSE):
				os << tokenTypeToStr(p.primtype());
				break;
			CASE(CALL):
				os << '~' << p.all.func_id << '(';
				for(size_t i = 0; i < p.main().args->size(); i++){
					os << (*p.main().args)[i];
					if(i != p.main().args->size() - 1){
						os << ", ";
					}
				}
				break;
			CASE(INVALID):
				os << '(' << *p.main().expr << ')';
				break;
			CASE(IDENTIFIER):
				os << '~' << p.main().lt.i64;
				break;
			default:
				break;
#undef CASE
		}
		os << '}';
		return os;
	} 
	// }}}
};

static const std::vector<TokenType> unary_ops = {
	TokenType::NOT,
	TokenType::MINUS
};

class UnaryExpr {
public:
	const TokenType op;
	const union Main {
		Primary *primary;
		UnaryExpr *unexpr;
		Main(Primary *p) : primary(p) {}
		Main(UnaryExpr *e): unexpr(e) {}
	} main;
	static TokenType make_op(Parser& p) {
		for(const auto type : unary_ops){
			if(p.match_type(type)){
				return type;
			}
		}
		return TokenType::INVALID;
	}
	static Main make_main(TokenType op, Parser& p){
		if(op == TokenType::INVALID){
			return new Primary(p);
		} else {
			return new UnaryExpr(p);
		}
	}
	inline bool is_constant() const noexcept {
		return (op == TokenType::INVALID ? main.primary->is_constant() : main.unexpr->is_constant());
	}
	UnaryExpr(Parser& p) : op(make_op(p)), main(make_main(op, p)) {}
	// friend operator<< {{{
	friend std::ostream& operator<<(std::ostream& os, const UnaryExpr& un) noexcept {
		os << '{';
		if(un.op == TokenType::INVALID){
			os << *un.main.primary;
		} else {
			os 	<< (un.op == TokenType::NOT ?
					"NOT" : "-")
				<< *un.main.unexpr;
		}
		os << '}';
		return os;
	}
	// }}}
};

static const std::vector<TokenType> binary_ops[] = {
	{ TokenType::OR },
	{ TokenType::AND },
	{ TokenType::EQ, TokenType::GT, TokenType::LT, TokenType::GT_EQ, TokenType::LT_EQ, TokenType::LT_GT },
	{ TokenType::PLUS, TokenType::MINUS },
	{ TokenType::STAR, TokenType::SLASH, TokenType::MOD, TokenType::DIV },
};

const uint16_t MAX_BINARY_LEVEL = 4;

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
	// friend operator<< {{{
	friend std::ostream& operator<<(std::ostream& os, const BinExpr<Level>& b) noexcept {
		os << '{';
		for(size_t i = 0; i < b.ops.size(); i++){
			os << b.exprs[i] << ' ' << opToStr(b.ops[i]) << ' ';
		}
		os << b.exprs.back();
		os << '}';
		return os;
	}
	// }}}
};

LValue::LValue(Parser& p, int64_t id){
	if(id == 0){
		// No pre-consumed identifier
		id = p.expect_type_r(TokenType::IDENTIFIER).literal.i64;
	}
	if(p.match_type(TokenType::LEFT_SQ)){
		// identifier LEFT_SQ expr RIGHT_SQ
		// (array access)
		// we distinguish between this and array access by letting expr == nullptr by default.
		expr = new Expr(p);
		p.expect_type(TokenType::RIGHT_SQ);
	}
}

Primary::All Primary::make_all(Parser& p){
	All res = {TokenType::INVALID, 0, 0};
	const Token& n = p.next();
	if(isAnyOf(n.type, 
				TokenType::REAL_C, TokenType::INT_C, TokenType::STR_C,
				TokenType::TRUE, TokenType::FALSE, TokenType::CHAR_C,
				TokenType::DATE_C)){
		/* literal */
		res.primtype = n.type;
		res.main = n.literal;
		return res;
	} else if(n.type == TokenType::IDENTIFIER){
		if(p.match_type(TokenType::LEFT_PAREN)){
			/* function call */
			res.primtype = TokenType::CALL; // lmao
			res.func_id = n.literal.i64; /* func_id is used to store the function's identifier */
			res.main.args = new std::vector<Expr>();
			if(p.match_type(TokenType::RIGHT_PAREN)){
				return res;
			}
			for(;;){
				res.main.args->emplace_back(p);
				if(p.match_type(TokenType::RIGHT_PAREN)){
					return res;
				}
				p.expect_type(TokenType::COMMA); /* another expr is coming */
			}
		} else {
			/* normal lvalue */
			res.primtype = TokenType::IDENTIFIER;
			// tell LValue we've already consumed an identifier
			// (it checks if id == 0, and since id > 0 for any valid id, it's fine)
			res.main.lvalue = LValue(p, n.literal.i64);
			return res;
		}
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
		union Name {
			Type *rec;
			TokenType tok;
			Name(TokenType t): tok(t) {} 
			Name(Type *t): rec(t) {}
			Name() {}
		} name;
	} all;
	inline bool is_array() const noexcept { return all.is_array; }
	inline const Expr *start() const noexcept { return all.start; }
	inline const Expr *end() const noexcept { return all.end; }
	inline All::Name name() const noexcept { return all.name; }
	static All make_all(Parser& p){
		All res;
		if(p.match_type(TokenType::ARRAY)){
			res.is_array = true;
			p.expect_type(TokenType::LEFT_SQ);
			res.start = new Expr(p);
			p.expect_type(TokenType::COLON);
			res.end = new Expr(p);
			p.expect_type(TokenType::RIGHT_SQ);
			p.expect_type(TokenType::OF);
			res.name.rec = new Type(p);
			return res;
		} else {
			res.is_array = false;
			for(const auto type : type_keywords){
				if(p.match_type(type)){
					res.name = type;
					return res;
				}
			}
			p.error("Invalid type name");
		}
	}
	Type(Parser& p): all(make_all(p)) {}
	// friend operator<< {{{
	friend std::ostream& operator<<(std::ostream& os, const Type& type){
		os << '{';
		if(type.all.is_array){
			os << "ARRAY[";
			os << (*type.all.start) << ':' << (*type.all.end);
			os << "] OF ";
			os << (*type.all.name.rec);
		} else {
			os << tokenTypeToStr(type.all.name.tok);
		}
		os << '}';
		return os;
	}
	// }}}
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
	// friend operator<< {{{
	friend std::ostream& operator<<(std::ostream& os, const Param& p) noexcept {
		os << '{';
		if(p.byref) os << "BYREF ";
		os << '{' << p.ident << "}: ";
		os << p.type;
		os << '}';
		return os;
	}
	// }}}
};

// }}}

// Program, Block, Stmt {{{

#define STMTFORM_LIST \
	FORM(DECLARE)\
	FORM(CONSTANT)\
	FORM(PROCEDURE)\
	FORM(FUNCTION)\
	FORM(ASSIGN)\
	FORM(INPUT)\
	FORM(OUTPUT)\
	FORM(IF)\
	FORM(CASE)\
	FORM(FOR)\
	FORM(REPEAT)\
	FORM(WHILE)\
	FORM(CALL)

enum class StmtForm {
#define FORM(x) x,
	STMTFORM_LIST
#undef FORM
};

const std::vector<std::string_view> stmtform_to_str = {
#define FORM(x) #x,
	STMTFORM_LIST
#undef FORM
};

std::string_view stmtformToStr(const StmtForm form){
	return stmtform_to_str[static_cast<int>(form)];
}

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
	// friend operator<< {{{
	friend std::ostream& operator<<(std::ostream& os, const Block& b) noexcept {
		os << "{\n";
		for(const auto& x : b.stmts){
			os << x << '\n';
		}
		os << '}';
		return os;
	}
	// }}}
};

class Program {
public:
	std::vector<Stmt<true>> stmts;
	Program(Parser& p){
		while(!p.done()){
			stmts.emplace_back(p);
		}
	}
	// friend operator<< {{{
	friend std::ostream& operator<<(std::ostream& os, const Program& p) noexcept {
		os << "{\n";
		for(const auto& x : p.stmts){
			os << x << '\n';
		}
		os << '}';
		return os;
	}
	// }}}
};

template<bool TopLevel>
class Stmt {
public:
	StmtForm form;
	std::vector<int64_t> ids;
	std::vector<LValue> lvalues;
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
				/* We already considered the identifier, 
				 * tell LValue that.
				 */
				lvalues.emplace_back(p, t.literal.i64);
				p.expect_type(TokenType::ASSIGN);
				exprs.emplace_back(p);
				break;
			CASE(INPUT)
				lvalues.emplace_back(p);
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
				lvalues.emplace_back(p);
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
	// friend operator<< {{{
	friend std::ostream& operator<<(std::ostream& os, const Stmt<TopLevel>& stmt) noexcept {
		os << '{';
		os << stmtformToStr(stmt.form);
		os << " ids: [";
		for(const auto x : stmt.ids){
			os << x << ',';
		}
		os << "] exprs: [";
		for(const auto& x : stmt.exprs){
			os << x << ", ";
		}
		os << "] blocks: [";
		for(const auto& x : stmt.blocks){
			os << x << ", ";
		}
		os << "] params: [";
		for(const auto& x : stmt.params){
			os << x << ", ";
		}
		os << "] types: [";
		for(const auto& x : stmt.types){
			os << x << ", ";
		}
		os << "] }";
		return os;
	}
	// }}}
};

// }}}

// Parser::parse {{{

void Parser::parse(){
	output = new Program(*this);
}

// }}}

#endif /* PARSER_HPP */
