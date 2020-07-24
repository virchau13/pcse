#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <cstdint>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <charconv>

#include "utils.hpp"
#include "value.hpp"
#include "globals.hpp"

struct RuntimeError : public std::runtime_error {
	using std::runtime_error::runtime_error;
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
	std::vector<std::string> strings; // so string_view doesn't leak.
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

	Env(int64_t identifier_count, std::map<std::string_view, int64_t>& id_map) : var_types(identifier_count+1, Primitive::INVALID), var_vals(identifier_count+1),
	var_call_level(identifier_count+1, 0) {
		// check for inbuilt functions
		for(const auto& func : builtin::global_funcs){
			auto it = id_map.find(func.first);
			if(it != id_map.end()){
				functable.insert(std::make_pair(it->second, func.second));
			}
		}
	}

#ifdef TESTS
public:
	std::ostringstream out;
	std::istringstream in;
#else 
	std::ostream& out = std::cout;
	std::istream& in = std::cin;
#endif
	void input(EValue *val, const EType type){
		if(type.is_array) throw TypeError("Cannot input array");
		switch(type.primtype){
#define CASE(x) case Primitive:: x
			CASE(INTEGER):
				{
					std::string str;
					std::getline(in, str);
					if(!in) goto fail_i;
					{
						auto res = std::from_chars(str.data(), str.data() + str.size(), val->i64);
						if(res.ptr != str.data() + str.size()) goto fail_i;
					}
					goto pass_i;
fail_i:
					throw RuntimeError("User did not input INTEGER correctly");
pass_i:
					;
				}
				break;
			CASE(REAL):
				{
					std::string str;
					std::getline(in, str);
					bool dot = true;
					for(size_t i = 0; i < str.size(); i++){
						if(isDigit(str[i]) || (dot && str[i] == '.')){
							dot &= (str[i] != '.');
						} else {
							throw RuntimeError("User did not input REAL correctly");
						}
					}
					val->frac = Fraction<>::fromValidStr(str);
				}
				break;
			CASE(BOOLEAN):
				{
					std::string str;
					std::getline(in, str);
					if(str == "TRUE") val->b = true;
					else if(str == "FALSE") val->b = false;
					else throw RuntimeError("User did not input BOOLEAN correctly");
				}
				break;	
			CASE(CHAR):
				{
					std::string str;
					std::getline(in, str);
					val->c = str[0];
					if(!in) throw RuntimeError("End of input reached");
				}
				break;
			CASE(DATE):
				{
					uint16_t day, month;
					uint16_t year;
					std::string str;
					std::getline(in, str);
					if(!in) goto fail;
					{
						const char *curr = str.data(), *end = str.data() + str.size();
						auto res = std::from_chars(curr, end, day);
						if(res.ptr == end || *res.ptr != '/') goto fail;
						curr = res.ptr+1;
						res = std::from_chars(curr, end, month);
						if(res.ptr == end || *res.ptr != '/') goto fail;
						curr = res.ptr + 1;
						res = std::from_chars(curr, end, year);
						if(res.ptr != end) goto fail;
					}
					goto pass;
fail:
					throw RuntimeError("User did not input DATE correctly");
pass:
					val->date = Date(day, month, year);
				}
				break;
			CASE(STRING):
				{
					std::string str;
					std::getline(in, str);
					if(!in) throw RuntimeError("End of input reached");
					strings.push_back(str);
					val->str = strings.back();
				}
				break;
			default:
				throw TypeError("Cannot input an undefined variable");
#undef CASE
		}
	}
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
