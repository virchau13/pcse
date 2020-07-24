#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <cstdint>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
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

const std::vector<std::string> primitive_to_str = {
	"INTEGER", "STRING", "CHAR", "REAL", "DATE", "BOOLEAN", "INVALID"
};

inline std::string_view primitiveToStr(const Primitive p){
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
		bool res = 
			primtype == et.primtype && 
			is_array == et.is_array &&
			bounds.size() == et.bounds.size();
		if(!res) return false;
		// The bounds check is more complicated than it seems.
		// The pseudocode guide _explicitly says_ that 
		/*
		 * DECLARE arr: ARRAY[0:1] OF INTEGER
		 * DECLARE bar: ARRAY[1:2] OF INTEGER
		 * bar <- arr
		 */
		// should compile, since they have the same size and default type.
		// So that's what we need to check.
		for(size_t i = 0; i < bounds.size(); i++){
			if(et.bounds[i].second - et.bounds[i].first
				!= bounds[i].second - bounds[i].first){
				return false;
			}
		}
		return true;
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

union EValue {
	std::string_view str;
	int64_t i64;
	Fraction<> frac;
	char c;
	bool b;
	Date date;
	std::vector<EValue> *vals;
	inline EValue(){}
	inline EValue(const std::string_view str_): str(str_) {}
	inline EValue(const int64_t i64_): i64(i64_) {}
	inline EValue(const Fraction<> frac_): frac(frac_) {}
	inline EValue(const char c_): c(c_) {}
	inline EValue(const bool b_): b(b_) {}
	inline EValue(const Date date_): date(date_) {}
	inline EValue(std::vector<EValue> * const val_): vals(val_) {}
};


// Function
struct EFunc {
	// max 64 args
	const uint_least8_t arity;
	const std::unique_ptr<EType[]> types;
	const std::unique_ptr<int64_t[]> ids;
	EType ret_type = Primitive::INVALID;
	const enum class What {
		RUNTIME,
		BUILTIN
	} what;
	void *func_loc = nullptr;
	EFunc(uint_least8_t arity_, What what_):
		arity(arity_), types(new EType[arity]), ids(new int64_t[arity]), what(what_) {}
	EFunc(): arity(0), what(What::RUNTIME) {}
	EFunc(EFunc& e) = delete;
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
private:
	std::vector<EType> var_types;
	std::vector<EValue> var_vals;
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

	const int32_t GLOBAL_LEVEL = 0;
	
	std::map<int64_t, EFunc> functable;
	
	size_t line_number = 1;

	inline bool checkLevel(int64_t var) const noexcept {
		const auto level = getLevel(var);
		return !(level != GLOBAL_LEVEL && level != call_number);
	}

	inline EValue& value(int64_t var){
		if(!checkLevel(var) || getType(var) == Primitive::INVALID){
			throw RuntimeError("Undefined variable");
		}
		return var_vals[var];
	}
	inline EValue& value_unchecked(int64_t var) noexcept {
		return var_vals[var];
	}
	inline const EValue& getValue(int64_t var) const {
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
		var_types[var] = Primitive::INVALID;
	}
	inline void expectType(int64_t var, const EType& type) const {
		if(var_types[var] != type){
			throw TypeError("Expected type " + type.to_str()
					+ ", but got type " + var_types[var].to_str());
		}
	}
private:
	void allocArr(EValue *val, const Primitive primtype, const std::vector<std::pair<int64_t,int64_t>> bounds, size_t currpos){
		if(currpos >= bounds.size()){
			allocVar(val, primtype);
			return;
		}
		// e.g. ARRAY[10:0]
		if(bounds[currpos].first > bounds[currpos].second){
			throw TypeError("Cannot have array with larger start index than end");
		}
		val->vals = new std::vector<EValue>(bounds[currpos].second - bounds[currpos].first + 1);
		for(EValue& v : *val->vals){
			allocArr(&v, primtype, bounds, currpos+1);
		}
	}

	inline void allocVar(EValue *val, const EType& etype){
		// Only arrays need allocation (for now).
		if(etype.is_array){
			allocArr(val, etype.primtype, etype.bounds, 0);
		}
	}
	inline void copyArr(EValue *og, EValue *val, size_t dim){
		if(dim == 0){
			*val = *og;
		} else {
			val->vals = new std::vector<EValue>(og->vals->size());
			for(size_t i = 0; i < og->vals->size(); i++){
				copyArr(&(*og->vals)[i], &(*val->vals)[i], dim-1);
			}
		}
	}
public:
	inline void allocVar(int64_t id, const EType& type){
		allocVar(&value_unchecked(id), type);
	}
	inline void copyValue(EValue val, const EType& type, EValue *target) {
		if(type.is_array){
			copyArr(&val, target, type.bounds.size());
		} else {
			*target = val;
		}
	}
	inline void copyVar(EValue val, const EType& type, int32_t level, int64_t target_id) {
		if(type == Primitive::INVALID) {
			throw RuntimeError("Attempt to copy variable which doesn't exist");
		}
		setType(target_id, type);
		setLevel(target_id, level);
		copyValue(val, type, &value(target_id));
	}
	inline void initVar(int64_t id, int32_t call_level, const EType& type, const EValue val){
		if(getType(id) != Primitive::INVALID){
			throw RuntimeError("Cannot initialize already-initialized variable");
		}
		setType(id, type);
		var_vals[id] = val;
		var_call_level[id] = call_level;
		allocVar(id, type);
	}

	Env(int64_t identifier_count) : var_types(identifier_count+1, Primitive::INVALID), var_vals(identifier_count+1),
	var_call_level(identifier_count+1, 0) {}

#ifdef TESTS
public:
	std::ostringstream out;
#else 
	std::ostream& out = std::cout;
#endif

	void output(const EValue val, const EType& type){
#define IFTYPE(x) if(type == Primitive:: x)
		IFTYPE(INTEGER) out << val.i64;
		else IFTYPE(REAL) out << val.frac.to_double();
		else IFTYPE(BOOLEAN) out << (val.b ? "TRUE" : "FALSE");
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

