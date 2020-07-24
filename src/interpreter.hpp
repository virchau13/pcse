#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include "parser.hpp"


template<typename... Args>
void expectTypeEqual(const EType& t1, const Args&... args){
	if(!isAnyOf(t1, args...)){
		throw TypeError("Bad type " + t1.to_str() + ", expected any of: " + (EType(args).to_str() + ...));
	}
}

/* More understandable error messages when we only expect one type */
void expectTypeEqual(const EType& t1, const EType& t2){
	if(t1 != t2){
		throw TypeError("Bad type " + t1.to_str() + ", expected " + t2.to_str());
	}
}

// defFunc, callFunc {{{

void defFunc(Env& env, const Stmt<true> &stmt){
	uint_least8_t arity = stmt.params.size();
	env.functable.try_emplace(stmt.ids[0], arity, EFunc::What::RUNTIME);
	EFunc& func = env.functable[stmt.ids[0]];
	func.func_loc = (void *)&stmt.blocks[0];
	for(size_t i = 0; i < stmt.params.size(); i++){
		const Param &param = stmt.params[i];
		if(param.byref) throw RuntimeError("BYREF is not supported");
		func.ids[i] = param.ident;
		func.types[i] = param.type.to_etype(env);
	}
	if(stmt.types.size()) {
		func.ret_type = stmt.types[0].to_etype(env);
	}
}

// Calls a function.
const std::optional<EValue> callFunc(Env& env, int64_t id, const std::vector<Expr>& args) {
	const EFunc &func = env.functable[id];
	if(args.size() != func.arity){
		throw RuntimeError("Invalid number of parameters for function");
	}
	const std::unique_ptr<EValue[]> argvals(new EValue[func.arity]);
	for(size_t i = 0; i < args.size(); i++){
		expectTypeEqual(args[i].type(env), func.types[i]);
		argvals[i] = args[i].eval(env);
	}
	std::optional<EValue> retval = std::nullopt;
	if(func.what == EFunc::What::BUILTIN){ // builtin function
		// Builtin functions take an array of `EValue`s and return an EValue
		auto func_ptr = (EValue (*)(EValue *))func.func_loc;
		EValue ret = func_ptr(argvals.get());
		if(func.ret_type != Primitive::INVALID){
			retval = ret;
		}
	} else { // runtime function
		std::vector<EType> old_types(func.arity);
		std::vector<EValue> old_vals(func.arity);
		std::vector<int32_t> old_levels(func.arity);
		{
			// Keep track of the old variables.
			for(size_t i = 0; i < args.size(); i++){
				int64_t ident = func.ids[i];
				old_types[i] = env.getType(ident);
				if(old_types[i] != Primitive::INVALID){
					old_vals[i] = env.getValue(ident);
					old_levels[i] = env.getLevel(ident);
				}
			}
			// Put in the new ones.
			env.call_number++;
			for(size_t i = 0; i < args.size(); i++){
				int64_t ident = func.ids[i];
				env.deleteVar(ident);
				env.copyVar(argvals[i], func.types[i], env.call_number, ident);
			}
		}
		const Expr *ret = ((Block *)func.func_loc)->eval(env);
		if(ret == nullptr && func.ret_type != Primitive::INVALID){ // should have returned, but didn't
			throw TypeError("Function didn't return");
		}
		if(ret != nullptr){
			// make sure the return type and the expr are equal
			expectTypeEqual(ret->type(env), func.ret_type);
			retval = ret->eval(env);
		}
		env.call_number--;
		// Restore the old variables.
		for(size_t i = 0; i < func.arity; i++){
			int64_t varid = func.ids[i];
			env.deleteVar(varid);
			if(old_types[i] != Primitive::INVALID){
				env.initVar(varid, old_levels[i], old_types[i], old_vals[i]);
			}
		}
	}
	return retval;
}

// }}}

// Primary, (Unary|Bin)Expr (all the eval() functions which return `EValue`s) {{{

EValue Primary::eval(Env& env) const {
#define IF(x) if(all.primtype == TokenType:: x) 
	IF(REAL_C) return main().lt.frac;
	IF(INT_C) return main().lt.i64;
	IF(CHAR_C) return main().lt.c;
	IF(TRUE) return true;
	IF(FALSE) return false;
	IF(DATE_C) return main().lt.date;
	IF(STR_C) return main().lt.str;
	IF(IDENTIFIER) return all.main.lvalue.eval(env);
	IF(CALL) {
		// Typechecking should be done for us. :P
		const std::optional<EValue> retval = callFunc(env, all.func_id, *all.main.args);
		if(!retval) {
			throw TypeError("Cannot call procedure without using CALL");
		}
		return retval.value();
	}
	IF(INVALID) return all.main.expr->eval(env);
	throw RuntimeError("Invalid primary type. (INTERNAL ERROR)");
}

EType Primary::type(Env& env) const {
#define RET(x) return Primitive:: x
	IF(REAL_C) RET(REAL);
	IF(INT_C) RET(INTEGER);
	IF(CHAR_C) RET(CHAR);
	IF(TRUE) RET(BOOLEAN);
	IF(FALSE) RET(BOOLEAN);
	IF(DATE_C) RET(DATE);
	IF(STR_C) RET(STRING);
	IF(IDENTIFIER) return all.main.lvalue.type(env);
	IF(CALL){
		const auto type = env.functable[all.func_id].ret_type;
		if(type == Primitive::INVALID){
			throw RuntimeError("Cannot call procedure and use it as a value");
		}
		return type;
	}
	IF(INVALID) return all.main.expr->type(env);
#undef IF
#undef RET
	throw RuntimeError("Invalid primary type. (INTERNAL ERROR)");
}

EValue LValue::eval(Env& env) const {
	const EType& type = env.getType(id);
	if(indexes != nullptr){
		const EValue *val = &env.getValue(id);
		if(indexes->size() != type.bounds.size()){
			throw TypeError("[] used on int TODO finish this error");
		}
		for(size_t i = 0; i < type.bounds.size(); i++){
			expectTypeEqual((*indexes)[i].type(env), Primitive::INTEGER);
			const int64_t index = (*indexes)[i].eval(env).i64;
			if(index < type.bounds[i].first || index > type.bounds[i].second){
				throw RuntimeError("Out-of-bounds index " + std::to_string(index));
			}
			val = &(*val->vals)[index - type.bounds[i].first];
		}
		return *val;
	} else {
		return env.getValue(id);
	}
}

EValue& LValue::ref(Env& env) const {
	const EType& type = env.getType(id);
	if(indexes != nullptr){
		EValue *val = &env.value(id);
		if(indexes->size() != type.bounds.size()){
			throw TypeError("[] used on int TODO finish this error");
		}
		for(size_t i = 0; i < type.bounds.size(); i++){
			expectTypeEqual((*indexes)[i].type(env), Primitive::INTEGER);
			const int64_t index = (*indexes)[i].eval(env).i64;
			if(index < type.bounds[i].first || index > type.bounds[i].second){
				throw RuntimeError("Out-of-bounds index " + std::to_string(index));
			}
			val = &(*val->vals)[index - type.bounds[i].first];
		}
		return *val;
	} else {
		return env.value(id);
	}
}

EValue UnaryExpr::eval(Env& env) const {
	if(op == TokenType::INVALID){
		return main.primary->eval(env);
	} else if(op == TokenType::NOT){
		expectTypeEqual(main.unexpr->type(env), Primitive::INTEGER);
		/* Did you know C++ has a `not` keyword? :) */
		return not (main.primary->eval(env).b);
	} else if(op == TokenType::MINUS){
		const auto& type = main.unexpr->type(env);
		expectTypeEqual(type, Primitive::INTEGER, Primitive::REAL);
		if(type == Primitive::INTEGER) return -main.unexpr->eval(env).i64;
		else /* if(type == Primitive::REAL) */ return -main.unexpr->eval(env).frac;
	} else {
		throw RuntimeError("Invalid unary expr operator. This should not have happened!");
	}
}

template<uint16_t Level>
EType BinExpr<Level>::type(Env& env) const {
	const EType ltype = left.type(env);
	if(opt.op == TokenType::INVALID) return ltype; /* no operators */
	if constexpr (Level <= 2) {
		return Primitive::BOOLEAN;	
	} else { 
		const EType rtype = opt.right->type(env);
		if constexpr (Level == 3){
			// Plus, Minus
			// Choose which one is a REAL
			if(rtype == Primitive::REAL) return rtype;
			else if(ltype == Primitive::REAL) return ltype;
			/* Check if they're both ints */
			if(ltype == Primitive::INTEGER && rtype == Primitive::INTEGER) return Primitive::INTEGER;
			// Any other case is mathematically invalid			
			throw TypeError("Invalid type applied to math expression");
		} else {
			// First enforce that they're either INTEGERs or REALs
			if(!(isAnyOf(ltype, Primitive::REAL, Primitive::INTEGER) &&
				 isAnyOf(rtype, Primitive::REAL, Primitive::INTEGER))){
				throw TypeError("Invalid type applied to math expression");
			}
			// We have to account for the slash
			if(opt.op == TokenType::SLASH) return Primitive::REAL;
			else if(opt.op == TokenType::STAR) {
				// REAL op INTEGER => REAL
				if(rtype == Primitive::REAL) return rtype;
				else return ltype;
			}
			// MOD, DIV both only take integers
			else {
				expectTypeEqual(ltype, Primitive::INTEGER);
				expectTypeEqual(rtype, Primitive::INTEGER);
				return Primitive::INTEGER;
			}
		}
	}
}

template<uint16_t Level>
EValue BinExpr<Level>::eval(Env& env) const {
	EValue leftval = left.eval(env);
	if(opt.op == TokenType::INVALID) return leftval;
	const EType ltype = left.type(env);
	EValue rightval = opt.right->eval(env);
	if constexpr (Level == 0) {
		// OR
		leftval.b |= rightval.b;
		return leftval;
	} else if constexpr (Level == 1) {
		// AND
		leftval.b &= rightval.b;
		return leftval;
	} else if constexpr (Level == 2){
		// all the comparison operators
		const EType rtype = opt.right->type(env);
#define OPCASE(l, r, x, op) \
		case TokenType:: x: \
			return l op r; \
			break; 
#define OPAPPLY(l, r, optok) \
	switch(optok){ \
		OPCASE(l, r, EQ, ==) \
		OPCASE(l, r, GT, >) \
		OPCASE(l, r, LT, <) \
		OPCASE(l, r, GT_EQ, >=) \
		OPCASE(l, r, LT_EQ, <=) \
		OPCASE(l, r, LT_GT, !=) \
		default: throw RuntimeError("Invalid operator for comparison expr. (INTERNAL ERROR)");\
	}
		if(ltype == Primitive::REAL && rtype == Primitive::INTEGER){
			OPAPPLY(leftval.frac, rightval.i64, opt.op);
		} else if(ltype == Primitive::INTEGER && rtype == Primitive::REAL){
			OPAPPLY(rightval.frac, leftval.i64, opt.op);
		}
		if(ltype != rtype) throw TypeError("Cannot compare two different types");
		if(ltype.is_array) throw TypeError("Cannot compare arrays");
#define IFTYPE(x, l, r) if(ltype == Primitive:: x){ OPAPPLY(l, r, opt.op); }
		IFTYPE(INTEGER, leftval.i64, rightval.i64);
		IFTYPE(REAL, leftval.frac, rightval.frac);
		IFTYPE(CHAR, leftval.c, rightval.c);
		IFTYPE(BOOLEAN, leftval.b, rightval.b);
		IFTYPE(STRING, leftval.str, rightval.str);
		throw RuntimeError("Invalid types! (INTERNAL ERROR)");
#undef IFTYPE
#undef OPAPPLY
#undef OPCASE
	} else if constexpr (Level == 3){
		// PLUS, MINUS
		const EType rtype = opt.right->type(env);
#define OPCASE(op) \
	if(ltype == Primitive::REAL){\
		if(rtype == Primitive::REAL) leftval.frac op##= rightval.frac;\
		else leftval.frac op##= rightval.i64;\
	} else {\
		if(rtype == Primitive::REAL){\
			leftval.frac = Fraction<>(leftval.i64);\
			leftval.frac op##= rightval.frac;\
		} else {\
			leftval.i64 op##= rightval.i64;\
		}\
	}\
	return leftval;
		if(opt.op == TokenType::PLUS){
			OPCASE(+);
		} else if(opt.op == TokenType::MINUS){
			OPCASE(-);
		} else {
			throw RuntimeError("Invalid operator for +- expr. (INTERNAL ERROR)");
		}
#undef OPCASE
	} else /* if constexpr (Level == 4) */ {
		const EType rtype = opt.right->type(env);
#define OPCASE(op) \
	if(ltype == Primitive::REAL){ \
		if(rtype == Primitive::REAL) leftval.frac op##= rightval.frac;\
		else leftval.frac op##= rightval.i64; \
	} else { \
		if(rtype == Primitive::REAL){ \
			leftval.frac = Fraction<>(leftval.i64);\
			leftval.frac -= rightval.frac;\
		}\
		else leftval.i64 op##= rightval.i64; \
	}\
	return leftval;
		
		switch(opt.op){
			case TokenType::STAR:
				OPCASE(*);
				break;
			case TokenType::SLASH:
				if(ltype == Primitive::INTEGER){
					auto tmp = leftval.i64;
					leftval.frac = Fraction<>(tmp);
				}
				if(rtype == Primitive::INTEGER){
					auto tmp = rightval.i64;
					rightval.frac = Fraction<>(tmp);
				}
				return leftval.frac / rightval.frac;
			case TokenType::MOD:
			case TokenType::DIV:
				expectTypeEqual(ltype, Primitive::INTEGER);
				expectTypeEqual(rtype, Primitive::INTEGER);
				return (opt.op == TokenType::DIV ? 
						leftval.i64 / rightval.i64 :
						leftval.i64 % rightval.i64);
			default:
				throw RuntimeError("Invalid operator for *,/,MOD,DIV expr. (INTERNAL ERROR)");
		}
	}
}

// }}}

// {Stmt<>, Block, Program}::{eval, type} {{{

EType Type::to_etype(Env& env, bool is_top) const {
	if(is_array()){
		auto nextType = all.name.rec->to_etype(env, false);
		if(!(all.start->type(env) == Primitive::INTEGER && all.end->type(env) == Primitive::INTEGER)){
			throw RuntimeError("The start and end types must be INTEGERs");
		}
		const auto startIdx = all.start->eval(env).i64;
		const auto endIdx = all.end->eval(env).i64;
		nextType.is_array = true;
		nextType.bounds.emplace_back(startIdx, endIdx);
		if(is_top){
			// this is the last type to return
			// so we reverse the array
			std::reverse(nextType.bounds.begin(), nextType.bounds.end());
		}
		return nextType;
	} else {
		switch(all.name.tok){
#define CASE(x) case TokenType:: x: return Primitive:: x;
			CASE(INTEGER);
			CASE(STRING);
			CASE(REAL);
			CASE(CHAR);
			CASE(BOOLEAN);
			CASE(DATE);
			default: throw RuntimeError("Invalid type primitive. (INTERNAL ERROR)");
#undef CASE
		}
	}
}

template<bool TopLevel>
const Expr *Stmt<TopLevel>::eval(Env& env) const {
	if constexpr (TopLevel) {
#define CASE(x) case StmtForm:: x
		switch(form){
			CASE(DECLARE):
				{
					const EType type = types[0].to_etype(env);
					env.initVar(ids[0], env.GLOBAL_LEVEL, type, (int64_t)0);
				}
				break;
			CASE(CONSTANT):
				env.initVar(ids[0], env.GLOBAL_LEVEL, exprs[0].type(env), exprs[0].eval(env));
				break;
			CASE(PROCEDURE):
			CASE(FUNCTION):
				defFunc(env, *this);
				break;
			default:
				goto anylevelstmt;
				break;
		}
		return nullptr;
anylevelstmt:
		;
	}
	switch(form){
		CASE(ASSIGN):
			{
				const EType type = lvalues[0].type(env);
				if(type == Primitive::INVALID){
					throw RuntimeError("Undefined variable");
				}
				const EType exprtype = exprs[0].type(env);
				if(type == Primitive::REAL && exprtype == Primitive::INTEGER){
					lvalues[0].ref(env).frac = Fraction<>(exprs[0].eval(env).i64);
				} else {
					expectTypeEqual(exprtype, type);
					env.copyValue(exprs[0].eval(env), type, &lvalues[0].ref(env));
				}
			}
			break;
		CASE(INPUT):
			throw RuntimeError("Inputting not implemented yet");
			// TODO
			break;
		CASE(OUTPUT):
			for(size_t i = 0; i < exprs.size(); i++){
				env.output(exprs[i].eval(env), exprs[i].type(env));
			}
			env.out << '\n';
			break;
		CASE(IF):
			expectTypeEqual(exprs[0].type(env), Primitive::BOOLEAN);
			if(exprs[0].eval(env).b){
				return blocks[0].eval(env);
			} else if(blocks.size() == 2){ // if there is an ELSE statement
				return blocks[1].eval(env);
			}
			break;
		CASE(CASE):
			{
				const EType type = lvalues[0].type(env);
				const EValue& val = lvalues[0].eval(env);
				if(type.is_array){
					throw TypeError("Cannot use array in CASE OF");
				}
				for(size_t i = 0; i < exprs.size(); i++){
					bool result = false;
					const EType exprtype = exprs[i].type(env);
					if(exprtype.is_array){
						throw TypeError("Cannot use array in CASE OF case");
					}
					if(isAnyOf(Primitive::REAL, type, exprtype) && type != exprtype){
						// INTEGER, REAL or vice versa
						if(type == Primitive::INTEGER){
							result = (exprs[i].eval(env).frac == val.i64);
						} else if(exprtype == Primitive::INTEGER){
							result = (val.frac == exprs[i].eval(env).i64);
						} else {
							throw TypeError("Cannot convert condition to REAL");
						}
					} else {
						expectTypeEqual(exprtype, type);
						EValue exprval = exprs[i].eval(env);
#define PRIM(t, n) case Primitive:: t: result = (exprval. n == val. n); break;
						switch(type.primtype){
							PRIM(DATE, date);
							PRIM(CHAR, c);
							PRIM(STRING, str);
							PRIM(BOOLEAN, b);
							PRIM(INTEGER, i64);
							PRIM(REAL, frac);
							default: throw TypeError("Use of unassigned type within CASE statement");
						}
#undef PRIM
					}
					if(result){
						return blocks[i].eval(env);
						goto endcase;
					}
				}
				if(blocks.size() > exprs.size()){
					// the last block is an OTHERWISE
					return blocks.back().eval(env);
				}
endcase:
				; /* no-op */
			}
			break;
		CASE(FOR):
			{
				EType types[3];
				bool is_frac = false;
				for(size_t i = 0; i < exprs.size(); i++){
					types[i] = exprs[i].type(env);
					expectTypeEqual(types[i], Primitive::REAL, Primitive::INTEGER);
					is_frac |= (types[i] == Primitive::REAL);
				}
				EValue vals[3];
				for(size_t i = 0; i < exprs.size(); i++){
					vals[i] = exprs[i].eval(env);
				}
				// Create the loop variable in scope and remove it later.
				// Keep the old var for restoring later.
				const EType old_type = env.getType(ids[0]);
				EValue old_val;
				int32_t old_call_frame;
				if(old_type != Primitive::INVALID){
					old_val = env.getValue(ids[0]);
					old_call_frame = env.getLevel(ids[0]);
				}
				// Delete the old one and put in our own.
				env.deleteVar(ids[0]);
				env.setType(ids[0], is_frac ? Primitive::REAL : Primitive::INTEGER);
				env.setLevel(ids[0], env.call_number); // Assigns the scope. (See environment.hpp).
				// (We'll assign the value in the individual cases.)

				// The loop condition can change depending on how it is written.
				// `FOR i <- 1 TO 10 STEP 2` => `for(i = 1; i <= 10; i += 2)`
				// `FOR i <- 10 TO 1 STEP -2` => `for(i = 10; i >= 1; i += -2)`
				// Note the change in the for loop condition.
				// This macro handles both cases.
#define LOOPCOND(from, to, i) (from <= to ? i <= to : i >= to)
				if(is_frac){
					// "Real" for loop.
					// Cast everything to Fraction first.
					for(size_t i = 0; i < exprs.size(); i++){
						if(types[i] == Primitive::INTEGER){
							auto tmp = vals[i].i64;
							vals[i].frac = Fraction<>(tmp);
						}
					}
					const Fraction<> step(exprs.size() == 3 ? vals[2].frac : Fraction<>(1));
					for(Fraction<> loopvar = vals[0].frac;
						LOOPCOND(vals[0].frac, vals[1].frac, loopvar);
						loopvar += step){
						env.value(ids[0]) = loopvar;
						const Expr *ret = blocks[0].eval(env);
						if(ret != nullptr){
							// The loop returned
							return ret;
						}
					}
				} else {
					// Integer for loop.
					const auto step = (exprs.size() == 3 ? vals[2].i64 : 1);
					for(
						auto loopvar = vals[0].i64;
						LOOPCOND(vals[0].i64, vals[1].i64, loopvar);
						loopvar += step){
						env.value(ids[0]) = loopvar;
						const Expr *ret = blocks[0].eval(env);
						if(ret != nullptr){
							// loop returned
							return ret;
						}
					}
				}
#undef LOOPCOND
				// Restore the old variable.
				env.deleteVar(ids[0]);
				if(old_type != Primitive::INVALID){
					env.setType(ids[0], old_type);
					env.value(ids[0]) = old_val;
					env.setLevel(ids[0], old_call_frame);
				}
			}
			break;
		CASE(REPEAT):
			expectTypeEqual(exprs[0].type(env), Primitive::BOOLEAN);
			do {
				const Expr *ret = blocks[0].eval(env);
				if(ret != nullptr) return ret;
			} while(!exprs[0].eval(env).b);
			break;
		CASE(WHILE):
			expectTypeEqual(exprs[0].type(env), Primitive::BOOLEAN);
			while(exprs[0].eval(env).b){
				const Expr *ret = blocks[0].eval(env);
				if(ret != nullptr) return ret;
			}
			break;
		CASE(CALL):
			// all the typechecking will be done for us
			callFunc(env, ids[0], exprs);
			break;
		default:
			// RETURN will be handled in Block::eval.
			throw RuntimeError("Invalid start of statement. (INTERNAL ERROR)");
	}
	return nullptr;
#undef CASE
}

const Expr *Block::eval(Env& env) const {
	for(const auto& stmt : stmts){
		if(stmt.form == StmtForm::RETURN){
			return &stmt.exprs[0];
		}
		const Expr *ret = stmt.eval(env);
		if(is_func && ret != nullptr){
			// The statement had a RETURN in it.
			// Execution stops here.
			return ret;
		}
	}
	return nullptr;
}

void Program::eval(Env& env) const {
	for(const auto& stmt : stmts){
		stmt.eval(env);
	}
}

// }}}

#endif /* INTERPRETER_HPP */
