#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <cstdint>
#include <vector>
#include <map>

#include "lexer.hpp"
#include "date.hpp"

enum class Primitive {
	INTEGER,
	STRING,
	CHAR,
	REAL,
	DATE,
	BOOLEAN,
	INVALID,
};

std::vector<std::string_view> primitive_to_str = {
	"INTEGER", "STRING", "CHAR", "REAL", "DATE", "BOOLEAN", "INVALID"
};

std::string_view primitiveToStr(const Primitive p){
	return primitive_to_str[static_cast<int>(p)];
}

class EnvError : public std::runtime_error {
public:
	template<typename... Args>
	EnvError(Args... args): std::runtime_error(args...) {}
};

class TypeError : public std::runtime_error {
public:
	template<typename... Args>
	TypeError(Args... args): std::runtime_error(args...) {}
};

/* Space for variables and such. */
class Env {
public:
	struct EType {
		bool is_array;
		std::vector<std::pair<int64_t,int64_t>> bounds; /* array [start, end], where length is level of nesting */
		Primitive primtype;
		EType() : primtype(Primitive::INVALID) {}
		EType(Primitive primtype_):  is_array(false), primtype(primtype_) {}
		EType(bool is_arr, std::vector<std::pair<int64_t,int64_t>> bounds_, Primitive primtype_):
			is_array(is_arr), bounds(bounds_), primtype(primtype_) {}
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
	};
	union Value {
		std::string_view str;
		int64_t i64;
		Fraction<> frac;
		char c;
		bool b;
		Date date;
	};
private:
	std::vector<EType> var_types;
	std::vector<Value> var_vals;
public:
	inline EType& type(int64_t var) noexcept {
		return var_types[var];
	}
	inline void expectType(int64_t var, const EType& type) const {
		if(var_types[var] != type){
			throw TypeError("Expected type " + type.to_str()
					+ ", but got type " + var_types[var].to_str());
		}
	}
	Env(int64_t identifier_count) : var_types(identifier_count+1), var_vals(identifier_count+1) {}
};

#endif /* ENVIRONMENT_HPP */

