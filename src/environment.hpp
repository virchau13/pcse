#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <cstdint>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>

#include "fraction.hpp"
#include "date.hpp"

struct RuntimeError : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

enum class Primitive {
	INTEGER,
	STRING,
	CHAR,
	REAL,
	DATE,
	BOOLEAN,
	INVALID
};

std::vector<std::string_view> primitive_to_str = {
	"INTEGER", "STRING", "CHAR", "REAL", "DATE", "BOOLEAN", "INVALID"
};

std::string_view primitiveToStr(const Primitive p){
	return primitive_to_str[static_cast<int>(p)];
}

struct EType {
	bool is_array;
	std::vector<std::pair<int64_t,int64_t>> bounds; /* array [start, end], where length is level of nesting */
	Primitive primtype;
	EType() : primtype(Primitive::INVALID) {}
	EType(Primitive primtype_):  is_array(false), primtype(primtype_) {}
	EType(bool is_arr, std::vector<std::pair<int64_t,int64_t>> bounds_, Primitive primtype_):
		is_array(is_arr), bounds(bounds_), primtype(primtype_) {}
	inline bool is_primitive() const noexcept { return is_array == false; }
	inline bool operator==(const EType& et) const noexcept {
		return 
			primtype == et.primtype &&
			is_array == et.is_array &&
			bounds == et.bounds;
	}
	inline bool operator!=(const EType& et) const noexcept {
		return !operator==(et);
	}
	inline std::string to_str() const noexcept {
		std::string res;
		if(is_array){
			for(const auto& b : bounds){
				res += "ARRAY[";
				res += std::to_string(b.first);
				res += ':';
				res += std::to_string(b.second);
				res += "] OF ";
			}
		}
		res += primitiveToStr(primtype);
		return res;
	}
	// Friend operator<< {{{
	inline friend std::ostream& operator<<(std::ostream& os, const EType et){
		os << et.to_str();
		return os;
	}
	// }}}
};

inline bool operator==(const Primitive prim, const EType& e) noexcept {
	return e == prim;
}

// (defined later, in interpreter.hpp)
template<bool TopLevel>
class Stmt;
// Function
struct EFunc {
	// points to where it is defined
	const Stmt<true> *def;
	EFunc(const Stmt<true> *def_ = nullptr): def(def_) {}
};

class EnvError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

class TypeError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/* Space for variables and such. */
class Env {
public:
	union Value {
		std::string_view str;
		int64_t i64;
		Fraction<> frac;
		char c;
		bool b;
		Date date;
		std::vector<Value> *vals;
		inline Value(){}
		inline Value(const std::string_view str_): str(str_) {}
		inline Value(const int64_t i64_): i64(i64_) {}
		inline Value(const Fraction<> frac_): frac(frac_) {}
		inline Value(const char c_): c(c_) {}
		inline Value(const bool b_): b(b_) {}
		inline Value(const Date date_): date(date_) {}
		inline Value(std::vector<Value> * const val_): vals(val_) {}
	};
private:
	std::vector<EType> var_types;
	std::vector<Value> var_vals;
	std::map<int64_t, EFunc> functable;
	/* This 'var_call_level' thing specifies what _call frame_
	 * the variable is in. 
	 * 0 is for global variables, and 1 is the _global_ call frame.
	 * Any function call, since it creates its own scope, has a _call frame number_
	 * assigned to it, (>= 2) and its variables are put inside this with that
	 * call frame number.
	 * A function cannot access anything except for 0 and its own call number.
	 */
	std::vector<int32_t> var_call_level;
public:
	// See the above for the call number.
	int32_t call_number = 1;
	
	size_t line_number = 1;

	inline bool checkLevel(int64_t var) const noexcept {
		const auto level = getLevel(var);
		return !(level && level != call_number);
	}

	inline Value& value(int64_t var){
		if(!checkLevel(var) || getType(var) == Primitive::INVALID){
			throw RuntimeError("Undefined variable");
		}
		return var_vals[var];
	}
	inline Value& value_unchecked(int64_t var) noexcept {
		return var_vals[var];
	}
	inline const Value& getValue(int64_t var) const {
		if(!checkLevel(var) || getType(var) == Primitive::INVALID){
			throw RuntimeError("Undefined variable");
		}
		return var_vals[var];
	}
	inline const EType& getType(int64_t var) const noexcept {
		return var_types[var];
	}
	inline int32_t getLevel(int64_t var) const noexcept {
		return var_call_level[var];
	}
	inline void setLevel(int64_t var, int32_t level) noexcept {
		var_call_level[var] = level;
	}
	inline void setType(int64_t var, EType type){
		if(var_types[var].primtype != Primitive::INVALID){
			throw TypeError("Variable already has type " + var_types[var].to_str()
					+ ", but was attempted to be redeclared with " + type.to_str());
		}
		var_types[var] = type;
	}
	inline void deleteVar(int64_t var) noexcept {
		var_types[var].primtype = Primitive::INVALID;
	}
	inline void expectType(int64_t var, const EType& type) const {
		if(var_types[var] != type){
			throw TypeError("Expected type " + type.to_str()
					+ ", but got type " + var_types[var].to_str());
		}
	}
	inline void defFunc(int64_t id, EFunc func) noexcept {
		functable[id] = func;
	}
	inline EFunc getFunc(int64_t id) const noexcept {
		return functable.at(id);
	}
	Env(int64_t identifier_count) : var_types(identifier_count+1), var_vals(identifier_count+1),
	var_call_level(identifier_count+1, 0) {}

#ifdef TESTS
	std::stringstream out;
#else 
	std::ostream& out = std::cout;
#endif

	void output(const Env::Value val, const EType& type){
#define IFTYPE(x) if(type == Primitive:: x)
		IFTYPE(INTEGER) out << val.i64;
		else IFTYPE(REAL) out << val.frac.to_double();
		else IFTYPE(BOOLEAN) out << (val.b ? "true" : "false");
		else IFTYPE(CHAR) out << val.c;
		else IFTYPE(DATE) out << val.date;
		else IFTYPE(STRING) out << val.str;
		else {
			throw TypeError("Cannot output array");
		}
#undef IFTYPE
	}
};

#endif /* ENVIRONMENT_HPP */

