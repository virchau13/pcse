#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <random>
#include <map>
#include <sstream>
#include <list>
#include "value.hpp"

namespace builtin {
	static std::random_device rd;
	static std::mt19937_64 gen(rd());
	namespace rnd {
		inline Fraction<> rnd(){
			std::uniform_int_distribution<uint16_t> d;
			return Fraction<>(d(gen), 65535);
		}
		inline EValue rnd_wrapper(EValue *){
			return rnd();
		}
	}
	namespace randombetween {
		inline int64_t randombetween(int64_t min, int64_t max){
			std::uniform_int_distribution<int64_t> d(min, max);
			return d(gen);
		}
		inline EValue randombetween_wrapper(EValue *args){
			return randombetween(args[0].i64, args[1].i64);
		}
		inline EType types[] = { Primitive::INTEGER, Primitive::INTEGER };
	}
	namespace int_f {
		inline int64_t int_f(Fraction<> f){
			return f.to_int();
		}
		inline EValue int_f_wrapper(EValue *arg){
			return int_f(arg->frac);
		}
		inline EType types[] = { Primitive::REAL };
	}


	const std::map<std::string_view, EFunc> global_funcs = {
		{ "RND", EFunc::make_builtin(0, nullptr, (void *)rnd::rnd_wrapper, Primitive::REAL) },
		{ "RANDOMBETWEEN", EFunc::make_builtin(2, 
				randombetween::types, 
				(void *)randombetween::randombetween_wrapper, 
				Primitive::INTEGER) 
		},
		{ "INT", EFunc::make_builtin(1, int_f::types, (void *)int_f::int_f_wrapper, Primitive::INTEGER) }
	};
}

namespace global {
	static std::list<std::string> strings; // so string_view doesn't leak
	inline std::string_view toStrView(const std::string& str){
		strings.push_back(str);
		return strings.back();
	}
	static std::istringstream dummy_stream;
}

#endif /* GLOBALS_HPP */
