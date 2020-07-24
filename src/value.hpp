#ifndef VALUE_HPP
#define VALUE_HPP

#include <vector>
#include "fraction.hpp"
#include "date.hpp"

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
	EType *types;
	int64_t *ids;
	EType ret_type = Primitive::INVALID;
	enum class What {
		RUNTIME,
		BUILTIN
	} what;
	void *func_loc = nullptr;
	EFunc(uint_least8_t arity_, What what_, EType *types_, int64_t *ids_, void *func, EType ret_type_):
		arity(arity_), types(types_), ids(ids_), ret_type(ret_type_), what(what_), func_loc(func) {}
	EFunc(uint_least8_t arity_, What what_):
		arity(arity_), types(new EType[arity]), ids(new int64_t[arity]), what(what_) {}
	EFunc(): arity(0), what(What::RUNTIME) {}
	EFunc(const EFunc& e):
		arity(e.arity), types(new EType[arity]), ids(new int64_t[arity]), ret_type(e.ret_type),
		what(e.what), func_loc(e.func_loc) 
	{
		if(e.types != nullptr) std::copy(e.types, e.types+arity, types);
		if(e.ids != nullptr) std::copy(e.ids, e.ids+arity, ids);
	}
	EFunc(EFunc& e): EFunc((const EFunc&)e) {}
	EFunc(const EFunc&& e) = delete;
	EFunc(EFunc&& e) : arity(e.arity), types(e.types), ids(e.ids), ret_type(e.ret_type), what(e.what), func_loc(e.func_loc) {
		e.ids = nullptr;
		e.types = nullptr;
	}
	~EFunc() {
		if(what == What::RUNTIME){
			delete[] types;
			delete[] ids;
		}
	}
	static inline EFunc make_builtin(uint_least8_t arity, EType *types, void *func, EType ret_type) {
		return EFunc(arity, What::BUILTIN, types, nullptr, func, ret_type);
	}
};

#endif /* VALUE_HPP */
