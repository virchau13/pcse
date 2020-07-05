#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include <cstdint>

enum PrimType : uint8_t {
	PT_INTEGER,
	PT_STRING,
	PT_CHAR,
	PT_REAL,
	PT_BOOLEAN,
	PT_DATE,
	/* number of things in PrimType */
	PT_LENGTH
};

// The VM's instructions are an int32_t[].
// Everything listed here is stored in exactly 1 int32_t.
/*
 * loc -> 
 *   if topmost bit is set, abs(loc) specifies an offset from the _stack base pointer_.
 *   if topmist bit isn't set, abs(loc) specifies an offset inside the _constant pool_.
 *
 * Every instruction is stored as a int32_t with the lowest byte
 * as the instruction and the highest (0 <= N <= 3) bytes as the param types.
 *
 * Every binary operator (ADD, SUB, etc.) takes 2 `loc`s and pushes the result onto the stack.
 * Every unary operator (NOT, NEG) takes 1 `loc` and pushes the result onto the stack.
 * JMP is an unconditional jump, taking 1 value: `ip`.
 * CJMP is a conditional jump, taking a `loc` and an `ip`. If `loc->b`, jump to `ip`.
 */ 

const int32_t TOPMOST_BIT32 = (1 << 31);

#define IREP2(name, a, b) I(name##a) I(name##b)
#define IREP4(name, a, b, c, d) IREP2(name, a, b) IREP2(name, c, d)
#define IREP6(name, a, b, c, d, e, f) IREP2(name, a, b) IREP4(name, c, d, e, f)

#define INSTRUCTIONS \
	/* Operators */ \
	I(ADD) \
	I(SUB) \
	I(MUL) \
	I(DIV) \
	I(NEG) \
	I(EQ) \
	I(GT) \
	I(LT) \
	I(GTEQ) \
	I(LTEQ) \
	I(NEQ) \
	I(AND) \
	I(OR) \
	I(NOT) \
	/* Flow */ \
	I(JMP) \
	I(CJMP) \

enum Op : uint8_t {
#define I(x) OP_##x,
	INSTRUCTIONS
#undef I
	OP_LENGTH
};

#endif /* BYTECODE_HPP */
