#ifndef VM_HPP
#define VM_HPP

#include <iostream>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include "date.hpp"
#include "fraction.hpp"
#include "bytecode.hpp"

static_assert(sizeof(size_t) >= 4, "must have at least 32-bit size_t");

class VMFileError : public std::runtime_error {
public:
	template<typename... Args> 
	VMFileError(Args... args) : std::runtime_error(args...) {}
};

class VMRuntimeError : public std::runtime_error {
	public:
	template<typename... Args> 
		VMRuntimeError(Args... args) : std::runtime_error(args...) {}
};

/* Read `Int` as little-endian. */
template<typename Int>
Int readAsLE(const uint8_t *buf){
	Int shift = 0;
	Int res = 0;
	for(size_t i = 0; i < sizeof(Int); i++){
		res += (buf[i] << shift);
		shift += 8;
	}
	return res;
}

class VM {

	union Value {
		int64_t i64;
		Fraction<> frac;
		std::string *str;
		bool b;
		Date date;
		char c;
		inline Value(int64_t i): i64(i) {}
		inline Value(Fraction<> f): frac(f) {}
		inline Value(std::string *s): str(s) {}
		inline Value(bool a): b(a) {}
		inline Value(Date d): date(d) {}
		inline Value(char a): c(a) {}
	};

public:
	std::vector<Value> const_pool; // constant pool
	std::vector<Value> stack;
	std::vector<int32_t> instr;
private:
	std::vector<int32_t>::const_iterator ip; /* Instruction pointer */
	size_t bp = 0; /* Stack base pointer */
	size_t sp = 0; /* Stack top pointer */

	inline Value& top() noexcept {
		return stack[sp-1];
	}

	inline Value& atLoc(int32_t loc){
		int32_t b = loc & TOPMOST_BIT32;
		loc ^= b;
		if(b){
			if(stack.size() <= sp+loc) throw VMRuntimeError("Location too large for stack");
			return stack[sp+loc];
		} else {
			if(const_pool.size() <= (size_t)loc) throw VMRuntimeError("Location too large for const_pool");
			return const_pool[loc];
		}
	}

	inline void push(Value val) noexcept {
		if(sp == stack.size()) stack.emplace_back(val);
		else stack[sp] = val;
		++sp;
	}

	void run(){
		ip = instr.begin();
		while(ip < instr.end()){
			const uint8_t 
				t1 = *ip & (0xFF << 24),
				t2 = *ip & (0xFF << 16),
				t3 = *ip & (0xFF << 8),
				ins = *ip & 0xFF;
			++ip;
			switch(ins){
#define I_F_BINOP(op) do { \
	push(atLoc(*ip)); \
	++ip; \
	const Value& v = atLoc(*ip); \
	if(t1 == PT_REAL){ \
		if(t2 == PT_REAL) top().frac op##= v.frac; \
		else if(t2 == PT_INTEGER) top().frac op##= v.i64; \
		else throw VMRuntimeError("Invalid math operator type"); \
	} else if(t1 == PT_INTEGER){ \
		if(t2 == PT_INTEGER) top().i64 op##= v.i64; \
		else if(t2 == PT_REAL){ \
			int64_t tmp = top().i64; \
			top().frac = tmp; \
			top().frac op##= v.frac; \
		} \
	} \
	++ip; \
} while(0);
				case OP_ADD: I_F_BINOP(+); break;
				case OP_SUB: I_F_BINOP(-); break;
				case OP_MUL: I_F_BINOP(*); break;
				case OP_DIV: I_F_BINOP(/); break;
#undef I_F_BINOP
				case OP_NOT:
					push(!atLoc(*ip).b);
					++ip;
					break;
				case OP_NEG:
					if(t1 == PT_REAL) push(-atLoc(*ip).frac);
					if(t1 == PT_INTEGER) push(-atLoc(*ip).i64);
					++ip;
					break;
#define BOOL_BINOP(op) do { \
	const Value& v1 = atLoc(*ip); \
	++ip; \
	const Value& v2 = atLoc(*ip); \
	++ip; \
	if(t1 == t2){ \
		if(t1 == PT_INTEGER) push(v1.i64 == v2.i64); \
		if(t1 == PT_STRING) push(*v1.str == *v2.str); \
		if(t1 == PT_CHAR) push(*v1.str == *v2.str); \
			}
		}
	}

};

#endif /* VM_HPP */
