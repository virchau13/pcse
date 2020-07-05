#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <cstdint>
#include <vector>
#include <map>

#include "lexer.hpp"

enum class Primitive {
	INTEGER,
	STRING,
	CHAR,
	REAL,
	DATE,
	BOOLEAN,
	INVALID
};

class EnvError : public std::runtime_error {
public:
	template<typename... Args>
	EnvError(Args... args): std::runtime_error(args...) {}
};

/* Space for variables and such. */
class Env {
public:
	struct EType {
		bool is_array;
		uint16_t dim; /* array dimensions */
		std::vector<std::pair<int64_t,int64_t>> bounds; /* array [start, end] */
		Primitive primtype;
		EType() : primtype(Primitive::INVALID) {}
		EType(Primitive primtype_):  is_array(false), dim(0), primtype(primtype_) {}
		EType(bool is_arr, uint16_t dim_, std::vector<std::pair<int64_t,int64_t>> bounds_, Primitive primtype_):
			is_array(is_arr), dim(dim_), bounds(bounds_), primtype(primtype_) {}
		inline bool operator==(const EType& et) const noexcept {
			return 
					primtype == et.primtype &&
					is_array == et.is_array &&
					dim == et.dim &&
					bounds == et.bounds;
		}
		inline bool operator!=(const EType& et) const noexcept {
			return !operator==(et);
		}
	};
	union Value {
		std::string_view str;
		int64_t i64;
		Fraction<> frac;
		char c;
		bool b;
		// TODO: Date
	};
private:
	std::vector<EType> var_types;
	std::vector<Value> var_vals;
public:
	const EType& getType(int64_t var) const noexcept {
		return var_types[var];
	}
	void expectType(int64_t var, const EType& type) const {
		// TODO
	}
	Env(int64_t identifier_count) : var_types(identifier_count+1), var_vals(identifier_count+1) {}
};

#endif /* ENVIRONMENT_HPP */

