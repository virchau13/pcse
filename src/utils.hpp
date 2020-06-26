#ifndef UTILS_HPP
#define UTILS_HPP

// encoding independent versions of the C functions

inline bool isDigit(const char c) noexcept {
	return (c ^ 0x30) <= 9;
}

inline bool isAlpha(const char c) noexcept {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

#endif /* UTILS_HPP */
