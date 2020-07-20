#ifndef FRACTION_HPP
#define FRACTION_HPP

#include <cstdint>
#include <numeric>
#include <limits>
#include <type_traits>
#include <ostream>

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

	inline bool mul_will_flow(num_t a, num_t b) const noexcept {
		// two complement check (cuz -INT_MIN == INT_MAX+1)
		if(a == -1 && b == NUM_MIN) return true;
		if(b == -1 && a == NUM_MIN) return true;
		// general case
		if(a > NUM_MAX/b) return true;
		if(b > NUM_MAX/a) return true;
		return false;
	}

	inline bool add_will_flow(num_t a, num_t b) const noexcept {
		if(b > 0 && a < NUM_MAX - b) return true;
		if(b < 0 && a < NUM_MIN - b) return true;
		return false;
	}

	inline bool sub_will_flow(num_t a, num_t b) const noexcept {
		if(b < 0 && a < NUM_MAX + b) return true;
		if(b > 0 && a < NUM_MIN + b) return true;
		return false;
	}

	inline bool div_will_flow(num_t a, num_t b) const noexcept {
		// INT_MIN/(-1) = INT_MAX+1
		if(a == NUM_MIN && b == -1) return true;
		return false;
	}
	
	// }}}
public:
	using num_type = num_t;

	inline void inverse_inplace() noexcept {
		num_t tmp = top;
		top = bot;
		bot = tmp;
	}

	inline void simplify() noexcept {
		num_t div = std::gcd(top, bot);
		top /= div;
		bot /= div;
	}

	inline Fraction inverse() const noexcept {
		Fraction res = *this;
		res.inverse_inplace();
		return res;
	}

	// Constructors and operator= {{{

	explicit inline Fraction(num_t x) : top(x), bot(1) {}

	inline Fraction(num_t top_, num_t bottom_): top(top_), bot(bottom_) {
		simplify();
	}

	// }}}
	
	// Operators like *= {{{
	
	// don't take fractions by reference - too expensive

	inline Fraction& operator*=(const Fraction<num_t> other) noexcept {
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
	typename std::enable_if_t<std::numeric_limits<Int>::is_integer, Fraction&> operator*=(const Int other) noexcept {
		num_t g = std::gcd(other, bot);
		top *= other / g;
		bot /= g;
		return *this;
	}

	inline Fraction& operator/=(const Fraction<num_t> other) noexcept {
		// operator* but with inverse.
		num_t g1 = std::gcd(bot, other.bot);
		num_t g2 = std::gcd(other.top, top);

		top = (bot / g1) * (other.top / g2);
		bot = (top / g2) * (other.bot / g1);

		return *this;
	}
	template<typename Int>
	typename std::enable_if_t<std::numeric_limits<Int>::is_integer, Fraction&> operator/=(const Int other) noexcept {
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
	typename std::enable_if_t<(sizeof(num_t) > 4), Fraction&> operator+=(const Fraction<num_t> other) noexcept {
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
	inline typename std::enable_if_t<std::numeric_limits<Int>::is_integer, Fraction&> operator+=(const Int other) noexcept {
		top += other * bot;
		return *this;
	}

	Fraction& operator-=(const Fraction<num_t> other) noexcept {
		// operator+= but with -other.top
		num_t x = std::gcd(bot, other.bot);
		top = top * (other.bot / x) - other.top * bot;
		x = std::gcd(top, x);
		top /= x;
		bot *= other.bot / x;
		return *this;
	}

	template<typename Int>
	inline typename std::enable_if_t<std::numeric_limits<Int>::is_integer, Fraction&> operator-=(const Int other) noexcept {
		top -= other * bot;
		return *this;
	}

	// }}}
	
	// Comparison operators {{{
	inline bool operator==(const Fraction<num_t> other) const noexcept {
		return top == other.top && bot == other.bot;
	}

	template<typename Int>
	inline typename std::enable_if_t<std::numeric_limits<Int>::is_integer, bool> operator==(const Int other) const noexcept {
		return top == other && bot == 1;
	}

	template<typename T>
	inline bool operator!=(const T other) const noexcept {
		return !operator==(other);
	}

	inline std::enable_if_t<sizeof(num_t) <= 4, bool> operator<(const Fraction<num_t> other) const noexcept {
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
	inline typename std::enable_if_t<std::numeric_limits<Int>::is_integer, bool> operator<(const Int other) const noexcept {
		// just check if the integer division
		// compares
		num_t a = top / bot;
		return a < other;
	}

	template<typename T>
	inline bool operator<=(const T other) const noexcept {
		return operator==(other) || operator<(other);
	}

	template<typename T>
	inline bool operator>(const T other) const noexcept {
		return !operator<=(other);
	}

	template<typename T>
	inline bool operator>=(const T other) const noexcept {
		return !operator<(other);
	}

	// }}}

	// Generic operators like * {{{
	template<typename T>
	inline Fraction operator*(const T other) const noexcept {
		Fraction res = *this;
		res *= other;
		return res;
	}
	template<typename T>
	inline Fraction operator/(const T other) const noexcept {
		Fraction res = *this;
		res /= other;
		return res;
	}
	template<typename T>
	inline Fraction operator+(const T other) const noexcept {
		Fraction res = *this;
		res += other;
		return res;
	}
	template<typename T>
	inline Fraction operator-(const T other) const noexcept {
		Fraction res = *this;
		res -= other;
		return res;
	}

	// Unary minus
	inline Fraction operator-() const noexcept {
		Fraction res = *this;
		res.top = -res.top;
		return res;
	}
	// }}}
	
	// Friends {{{
	friend std::ostream& operator<<(std::ostream& os, const Fraction<num_t>& frac) noexcept {
		os << frac.top;
		os << '/';
		os << frac.bot;
		return os;
	}
	// }}}
};

#endif /* FRACTION_HPP */
