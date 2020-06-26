#ifndef CSTR_HPP 
#define CSTR_HPP

#include <cstring>
#include <string>
#include <ostream>

/* C-style string managing classes */

class CStr;

class CStr_view {
public:
	const char * str;
	CStr_view() = delete;
	CStr_view(const char * str_): str(str_) {}
	CStr_view(const std::string& s): str(s.c_str()) {}
	CStr_view(const CStr s);

	// Operators {{{
	inline bool operator<(const CStr_view other) const noexcept {
		return strcmp(str, other.str) < 0;
	}
	inline bool operator<=(const CStr_view other) const noexcept {
		return strcmp(str, other.str) <= 0;
	}
	inline bool operator>(const CStr_view other) const noexcept {
		return strcmp(str, other.str) > 0;
	}
	inline bool operator>=(const CStr_view other) const noexcept {
		return strcmp(str, other.str) >= 0;
	}
	inline bool operator==(const CStr_view other) const noexcept {
		return strcmp(str, other.str) == 0;
	}
	char operator[](const size_t idx) noexcept {
		return str[idx];
	}
	void destroy() noexcept {
		free((void *)str);
		str = nullptr;
	}
	friend std::ostream& operator<<(std::ostream& os, const CStr_view& s) noexcept {
		os << s.str;
		return os;
	}
	// }}}
};

class CStr {
public:
	char *str;
	CStr() = delete;
	CStr(const char * const str_): str(strdup(str_)) {}
	CStr(const std::string& s): CStr(s.c_str()) {}
	static inline char * dup_string_view(const std::string_view sv){
		char *res = (char *)malloc(sv.length() + 1);
		memcpy(res, sv.data(), sv.length());
		res[sv.length()] = '\0';
		return res;
	}
	CStr(const std::string_view sv): str(dup_string_view(sv)) {}
	CStr(const CStr& other): CStr(other.str) {}
	CStr(CStr&& other): str(other.str) { other.str = NULL; }

	~CStr(){
		free(str);
	}

	// Operators {{{
	inline bool operator<(const CStr other) const noexcept {
		return strcmp(str, other.str) < 0;
	}
	inline bool operator<=(const CStr other) const noexcept {
		return strcmp(str, other.str) <= 0;
	}
	inline bool operator>(const CStr other) const noexcept {
		return strcmp(str, other.str) > 0;
	}
	inline bool operator>=(const CStr other) const noexcept {
		return strcmp(str, other.str) >= 0;
	}
	inline bool operator==(const CStr other) const noexcept {
		return strcmp(str, other.str) == 0;
	}
	char& operator[](const size_t idx) noexcept {
		return str[idx];
	}
	// }}}
};

CStr_view::CStr_view(const CStr cs) : str(cs.str) {}

#endif /* CSTR_HPP */
