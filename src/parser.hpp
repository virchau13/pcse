#ifndef PARSER_HPP
#define PARSER_HPP
#include <string>
#include <map>
#include <vector>
#include "lexer.hpp"
#include "environment.hpp"

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
	inline ~Parser();
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
	void run(Env& env);
};

// }}}

// grammar.ebnf describes the idealised syntax.

// Expr, Type, Param, Primary, LValue {{{

template<uint16_t Level>
class BinExpr;

using Expr = BinExpr<0>;
std::ostream& operator<<(std::ostream& os, const Expr& expr) noexcept;

class LValue {
public:
	int64_t id;
	std::vector<Expr> *indexes = nullptr;
	LValue(Parser& p, int64_t id = 0);
	/* copy */ LValue(LValue& l) = delete;
	/* move */ LValue(LValue&& l) noexcept : id(l.id), indexes(l.indexes) {
		l.indexes = nullptr;
	}
	LValue& operator=(LValue&& l) noexcept {
		id = l.id;
		indexes = l.indexes;
		l.indexes = nullptr;
		return *this;
	}
	Env::Value& ref(Env& env) const;
	Env::Value eval(Env& env) const;
	inline EType type(const Env& env) const {
		/* Since arrays, no matter how many dimensions they have,
		 * can only have one type of element,
		 * we don't have to process everything:
		 * we can just return the element type.
		 */
		return env.getType(id).primtype;
	}
	~LValue() {
		/* Deleting a nullptr is safe. */
		delete indexes;
	}
	// Friend operator<< {{{
	friend std::ostream& operator<<(std::ostream& os, const LValue& lv){
		os << '~' << lv.id;
		if(lv.indexes != nullptr){
			for(Expr& expr : *lv.indexes){
				os << '[' << expr << ']';
			}
		}
		return os;
	}
	// }}}
};

class Primary {
public:
	struct All {
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
			Main() {}
			~Main() {}
		} main;
	} all;
	const All::Main& main() const noexcept { return all.main; }
	TokenType primtype() const noexcept { return all.primtype; }
	Primary(Parser& p);
	/* copy */ Primary(Primary& pri) = delete;
	/* move */ Primary(Primary&& pri) noexcept {
		std::memcpy(&all, &pri.all, sizeof(All));
		pri.all.main.expr = nullptr;
	}
	~Primary();
	Env::Value eval(Env& env) const;
	EType type(Env& env) const;
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
				os << ')';
				break;
			CASE(INVALID):
				os << '(' << *p.main().expr << ')';
				break;
			CASE(IDENTIFIER):
				os << p.main().lvalue;
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
	union Main {
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
	UnaryExpr(Parser& p) : op(make_op(p)), main(make_main(op, p)) {}
	/* copy */ UnaryExpr(UnaryExpr& un) = delete;
	/* move */ UnaryExpr(UnaryExpr&& un) noexcept : op(un.op), main(un.main) {
		un.main.primary = nullptr;
	}
	~UnaryExpr() {
		if(op == TokenType::INVALID){
			delete main.primary;
		} else {
			delete main.unexpr;
		}
	}
	Env::Value eval(Env& env) const;
	inline EType type(Env& env) const {
		return (op == TokenType::INVALID ? main.primary->type(env) : main.unexpr->type(env));
	}
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

const std::vector<TokenType> binary_ops[] = {
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

	LowerExpr left;
	struct Opt {
		TokenType op;
		BinExpr<Level> *right;
	} opt;

	static Opt make_opt(Parser& p){
		for(const auto op_type : binary_ops[Level]){
			if(p.match_type(op_type)){
				/* valid operator */
				return { op_type, new BinExpr<Level>(p) };
			}
		}
		// no operator
		return { TokenType::INVALID, nullptr };
	}
	
	BinExpr(Parser& p) : left(p), opt(make_opt(p)) {}
	/* copy */ BinExpr(BinExpr& be) = delete;
	/* move */ BinExpr(BinExpr&& be) noexcept : left(std::move(be.left)), opt(be.opt) {
		be.opt = { TokenType::INVALID, nullptr };
	}
	~BinExpr() {
		if(opt.op != TokenType::INVALID) delete opt.right;
	}
	Env::Value eval(Env& env) const;
	EType type(Env& env) const;
	// friend operator<< {{{
	friend std::ostream& operator<<(std::ostream& os, const BinExpr<Level>& b) noexcept {
		os << '{';
		os << b.left; 
		if(b.opt.op != TokenType::INVALID){
			os << ' ' << opToStr(b.opt.op) << ' ' << *b.opt.right;
		}
		os << '}';
		return os;
	}
	// }}}
};

LValue::LValue(Parser& p, int64_t id_) : id(id_) {
	if(id == 0){
		// No pre-consumed identifier, so we consume one
		id = p.expect_type_r(TokenType::IDENTIFIER).literal.i64;
	}
	if(p.match_type(TokenType::LEFT_SQ)){
		// identifier { LEFT_SQ expr RIGHT_SQ }
		// (array access)
		// we distinguish between this and array access by letting indexes == nullptr by default
		indexes = new std::vector<Expr>();
		for(;;){
			indexes->emplace_back(p);
			p.expect_type(TokenType::RIGHT_SQ);
			if(!p.match_type(TokenType::LEFT_SQ)) break;
			/* consume next index */
		}
	}
}

const std::vector<TokenType> const_types = {
	TokenType::REAL_C, TokenType::INT_C, TokenType::STR_C,
    TokenType::TRUE, TokenType::FALSE, TokenType::CHAR_C,
    TokenType::DATE_C
};

Primary::Primary(Parser& p) {
	const Token& n = p.next();
	if(isAnyOf(n.type, const_types)){
		/* literal */
		all.primtype = n.type;
		all.main.lt = n.literal;
		return;
	} else if(n.type == TokenType::IDENTIFIER){
		if(p.match_type(TokenType::LEFT_PAREN)){
			/* function call */
			all.primtype = TokenType::CALL; // lmao
			all.func_id = n.literal.i64; /* func_id is used to store the function's identifier */
			all.main.args = new std::vector<Expr>();
			if(p.match_type(TokenType::RIGHT_PAREN)){
				return;
			}
			for(;;){
				all.main.args->emplace_back(p);
				if(p.match_type(TokenType::RIGHT_PAREN)){
					return;
				}
				p.expect_type(TokenType::COMMA); /* another expr is coming */
			}
		} else {
			/* normal lvalue */
			all.primtype = TokenType::IDENTIFIER;
			// tell LValue we've already consumed an identifier
			// (it checks if id == 0, and since id > 0 for any valid id, it's fine)
			LValue lv(p, n.literal.i64);
			all.main.lvalue = std::move(lv);
			return;
		}
	} else if(n.type == TokenType::LEFT_PAREN){
		/* all.primtype is set to INVALID by default */
		all.main.expr = new Expr(p);
		p.expect_type(TokenType::RIGHT_PAREN);
		return;
	} else {
		p.error("Invalid primary");
	}
}

Primary::~Primary() {
	/* If it's a const type we don't have to do anything */
	if(!isAnyOf(all.primtype, const_types)){
		if(all.primtype == TokenType::CALL){
			// Deallocate the argument array.
			delete all.main.args;
		} else if(all.primtype == TokenType::IDENTIFIER){
			// delete the lvalue
			all.main.lvalue.~LValue();
		} else { /* TokenType::INVALID */
			delete all.main.expr;
		}
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
	~Type() {
		if(all.is_array) {
			delete all.name.rec;
		}
	}
	EType to_etype(Env& env, bool is_top = true) const;
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
		int64_t id = p.expect_type_r(TokenType::IDENTIFIER).literal.i64;
		p.expect_type(TokenType::COLON); // `x: INTEGER`, colon after id
		return id;
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
	FORM(CALL)\
	FORM(RETURN)

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
	bool is_func;
	Block(Parser& p, bool is_func_ = false) : is_func(is_func_) {
		while(isValidStmtStart(p.peek().type) || (is_func && p.peek().type == TokenType::RETURN)){
			stmts.emplace_back(p, is_func);
		}
	}
	const Expr *eval(Env& env) const;
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
	void eval(Env& env) const;
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
		size_t param_count = 0;
		for(;;){
			params.emplace_back(p);
			param_count++;
			if(!p.match_type(TokenType::COMMA)){
				break;
			}
			if(param_count == 64){
				// next one will be 65
				p.error("Cannot declare a function with more than 64 arguments");
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
	void stmt(Parser& p, const Token& t, bool is_func){
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
				blocks.emplace_back(p, is_func);
				if(p.match_type(TokenType::ELSE)){
					blocks.emplace_back(p, is_func);
				}
				p.expect_type(TokenType::ENDIF);
				break;
			CASE(CASE) // lmao
				p.expect_type(TokenType::OF);
				lvalues.emplace_back(p);
				for(;;){
					exprs.emplace_back(p);
					p.expect_type(TokenType::COLON);
					blocks.emplace_back(p, is_func);
					if(p.match_type(TokenType::OTHERWISE)){
						blocks.emplace_back(p, is_func);
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
				blocks.emplace_back(p, is_func);
				p.expect_type(TokenType::NEXT);
				break;
			CASE(REPEAT)
				blocks.emplace_back(p, is_func);
				p.expect_type(TokenType::UNTIL);
				exprs.emplace_back(p);
				break;
			CASE(WHILE)
				exprs.emplace_back(p);
				p.expect_type(TokenType::DO);
				blocks.emplace_back(p, is_func);
				p.expect_type(TokenType::ENDWHILE);
				break;
			CASE(CALL)
				ids.push_back(CONSUME_ID());
				if(p.match_type(TokenType::LEFT_PAREN)){
					exprlist(p);
				}
				p.expect_type(TokenType::RIGHT_PAREN);
				break;
			default:
				if(is_func && t.type == TokenType::RETURN){
					form = StmtForm::RETURN;
					exprs.emplace_back(p);
				} else {
					p.error("Invalid start of statement");
				}
		}
	}
	inline void stmt(Parser& p, bool is_func){
		const Token& t = p.next();
		stmt(p, t, is_func);
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
				blocks.emplace_back(p, /* is_func */ true);
				p.expect_type(TokenType::ENDFUNCTION);
				break;
			default:
				stmt(p, t, /* is not function */ false);
		}
	}
#undef CASE
#undef CONSUME_ID
	Stmt(Parser& p, bool is_func = false){
		if constexpr (TopLevel){
			topstmt(p);
		} else {
			stmt(p, is_func);
		}
	}
	const Expr *eval(Env& env) const;
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

// Parser::{parse, run, ~Parser} {{{

void Parser::parse(){
	output = new Program(*this);
}

 
void Parser::run(Env& env){
	output->eval(env);
}

inline Parser::~Parser() {
	delete output;
}

// }}}

#endif /* PARSER_HPP */

