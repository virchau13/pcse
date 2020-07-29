#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <algorithm>

// encoding independent versions of the C functions

inline bool isDigit(const unsigned char c) noexcept {
	return (c ^ 0x30) <= 9;
}

inline bool isAlpha(const char c) noexcept {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

// useful

template<typename T, typename... Args>
inline bool isAnyOf(T t, Args... args){
	return ((t == args) || ...);
}

template<typename T, typename K>
inline bool isAnyOf(T t, const std::vector<K>& k){
	return std::find(k.begin(), k.end(), t) != k.end();
}

#endif /* UTILS_HPP */
