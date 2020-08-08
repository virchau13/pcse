#ifndef FRACTION_HPP
#define FRACTION_HPP

#include <cstdint>
#include <numeric>
#include <limits>
#include <type_traits>
#include <ostream>
#include "error.hpp"

/* In PCSE, the REAL datatype is supposed to represent any real number.
 * This comes with a variety of problems, most notably that this is computationally impossible.
 * However, in PCSE there are no predeclared irrational constants, 
 * nor are there functions to get them.
 * In other words, we can fake real numbers by only considering the rationals.
 * This is what this class is for.
 */

/* Guarantees that GCD(numerator, denominator) = 1. */
template<typename num_t = int32_t>
class Fraction {

	static_assert(std::numeric_limits<num_t>::is_signed && std::numeric_limits<num_t>::is_integer, "must be a signed integer type");
	static const num_t NUM_MAX = std::numeric_limits<num_t>::max();
	static const num_t NUM_MIN = std::numeric_limits<num_t>::min();
protected:
	/* gcd(top, bot) will always be 1 */
	num_t top, bot;
private:
// Overflow / underflow checks {{{

	inline bool mul_will_flow(num_t a, num_t b) const {
		// two complement check (cuz -INT_MIN == INT_MAX+1)
		if(a == -1 && b == NUM_MIN) return true;
		if(b == -1 && a == NUM_MIN) return true;
		// general case
		if(a > NUM_MAX/b) return true;
		if(b > NUM_MAX/a) return true;
		return false;
	}

	inline bool add_will_flow(num_t a, num_t b) const {
		if(b > 0 && a < NUM_MAX - b) return true;
		if(b < 0 && a < NUM_MIN - b) return true;
		return false;
	}

	inline bool sub_will_flow(num_t a, num_t b) const {
		if(b < 0 && a < NUM_MAX + b) return true;
		if(b > 0 && a < NUM_MIN + b) return true;
		return false;
	}

	inline bool div_will_flow(num_t a, num_t b) const {
		// INT_MIN/(-1) = INT_MAX+1
		if(a == NUM_MIN && b == -1) return true;
		return false;
	}
	
	// }}}
public:
	using num_type = num_t;

	inline void inverse_inplace() {
		num_t tmp = top;
		top = bot;
		bot = tmp;
		div_by_0();
	}

	inline void simplify() {
		num_t div = std::gcd(top, bot);
		top /= div;
		bot /= div;
	}

	inline Fraction inverse() const {
		Fraction res = *this;
		res.inverse_inplace();
		return res;
	}

	inline void div_by_0() const {
		if(bot == 0){
			throw RuntimeError("Cannot divide by zero");
		}
	}

	inline double to_double() const { return (double)top / (double)bot; }

	inline int64_t to_int() const { return top / bot; }

	// Constructors and operator= {{{

	explicit inline Fraction(num_t x) : top(x), bot(1) {}

	inline Fraction(num_t top_, num_t bottom_): top(top_), bot(bottom_) {
		div_by_0();
		simplify();
	}

	// }}}
	
	// Operators like *= {{{
	
	// don't take fractions by reference - too expensive

	inline Fraction& operator*=(const Fraction<num_t> other) {
		/* (a/b) * (c/d) = (ac / bd)
		 * but we want to avoid overflow, 
		 * so we can't just do this then simplify.
		 * We can be a little bit smarter.
		 * 
		 * Remember that the final result is
		 * g <- gcd(ac, bd)
		 * (ac / g) / (bd / g).
		 * But gcd(a, b) = 1 and gcd(c, d) = 1,
		 * which means that gcd(ac, bd) = gcd(a, d) * gcd(c, b).
		 * So then we can use that.
		 */
		num_t x = std::gcd(top, other.bot);
		num_t y = std::gcd(other.top, bot);

		top = (top / x) * (other.top / y);
		bot = (bot / y) * (other.bot / x);

		return *this;
	}
	template<typename Int>
	typename std::enable_if_t<std::numeric_limits<Int>::is_integer, Fraction&> operator*=(const Int other) {
		num_t g = std::gcd(other, bot);
		top *= other / g;
		bot /= g;
		return *this;
	}

	inline Fraction& operator/=(const Fraction<num_t> other) {
		return operator*=(other.inverse());
	}
	template<typename Int>
	typename std::enable_if_t<std::numeric_limits<Int>::is_integer, Fraction&> operator/=(const Int other) {
		num_t g = std::gcd(other, top);
		top /= g;
		bot *= other / g;
	}
	inline typename std::enable_if_t<sizeof(num_t) <= 4, Fraction&> operator+=(const Fraction<num_t> other){
		// (a/b) + (c/d) = (ad/bd) + (cb/db) = (ad + cb)/db
		int64_t a = top, b = bot, c = other.top, d = other.bot;
		int64_t topr = a*d + c*b, botr = d*b;
		int64_t g = std::gcd(topr, botr);
		top = topr / g;
		bot = botr / g;
		return *this;
	}

	/*
	typename std::enable_if_t<(sizeof(num_t) > 4), Fraction&> operator+=(const Fraction<num_t> other) {
		// TODO: explain this
		num_t x = std::gcd(bot, other.bot);
		bot /= x;
		top = top * (other.bot / x) + other.top * bot;
		x = std::gcd(top, x);
		top /= x;
		bot *= other.bot * x;
		return *this;
	} */

	template<typename Int>
	inline typename std::enable_if_t<std::numeric_limits<Int>::is_integer, Fraction&> operator+=(const Int other) {
		top += other * bot;
		return *this;
	}

	inline Fraction& operator-=(const Fraction<num_t> other) {
		// operator+= but with -other.top
		return operator+=(-other);
	}

	template<typename Int>
	inline typename std::enable_if_t<std::numeric_limits<Int>::is_integer, Fraction&> operator-=(const Int other) {
		top -= other * bot;
		return *this;
	}

	// }}}
	
	// Comparison operators {{{
	inline bool operator==(const Fraction<num_t> other) const {
		return top == other.top && bot == other.bot;
	}

	template<typename Int>
	inline typename std::enable_if_t<std::numeric_limits<Int>::is_integer, bool> operator==(const Int other) const {
		return top == other && bot == 1;
	}

	template<typename T>
	inline bool operator!=(const T other) const {
		return !operator==(other);
	}

	inline std::enable_if_t<sizeof(num_t) <= 4, bool> operator<(const Fraction<num_t> other) const {
		// literally just 64 bit hacks lmao
		// a/b < c/d
		// ad/bd < cb/db
		// ad < cb
		int64_t a = top, b = bot, c = other.top, d = other.bot;
		// Maximum magnitude this can be is
		// (-2147483648) * (-2147483648)
		// which is below int64_t maximum
		return (a*d) < (c*b);
	}

	template<typename Int>
	inline typename std::enable_if_t<std::numeric_limits<Int>::is_integer, bool> operator<(const Int other) const {
		// just check if the integer division
		// compares
		num_t a = top / bot;
		return a < other;
	}

	template<typename T>
	inline bool operator<=(const T other) const {
		return operator==(other) || operator<(other);
	}

	template<typename T>
	inline bool operator>(const T other) const {
		return !operator<=(other);
	}

	template<typename T>
	inline bool operator>=(const T other) const {
		return !operator<(other);
	}

	// }}}

	// Generic operators like * {{{
	template<typename T>
	inline Fraction operator*(const T other) const {
		Fraction res = *this;
		res *= other;
		return res;
	}
	template<typename T>
	inline Fraction operator/(const T other) const {
		Fraction res = *this;
		res /= other;
		return res;
	}
	template<typename T>
	inline Fraction operator+(const T other) const {
		Fraction res = *this;
		res += other;
		return res;
	}
	template<typename T>
	inline Fraction operator-(const T other) const {
		Fraction res = *this;
		res -= other;
		return res;
	}

	// Unary minus
	inline Fraction operator-() const {
		Fraction res = *this;
		res.top = -res.top;
		return res;
	}
	// }}}
	
	// Friends {{{
	friend std::ostream& operator<<(std::ostream& os, const Fraction<num_t>& frac) {
		os << frac.top;
		os << '/';
		os << frac.bot;
		return os;
	}
	// }}}
	
	static Fraction<num_t> fromValidStr(const std::string_view& sv){
		// parse fraction
		int32_t top = 0, bot = 1;
		int32_t mul = 1;
		for(ssize_t i = sv.size()-1; i >= 0; i--){
			if(sv[i] == '.'){
				bot = mul;
			} else {
				top += (int32_t)(sv[i] - '0') * mul;
				mul *= 10;
			}
		}
		return Fraction<>(top, bot);
	}
};

#endif /* FRACTION_HPP */
