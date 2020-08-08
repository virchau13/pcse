#ifndef ERROR_HPP
#define ERROR_HPP
#include <stdexcept>

class RuntimeError : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

#endif /* ERROR_HPP */
