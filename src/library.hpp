#ifndef LIBRARY_HPP
#define LIBRARY_HPP

#include <string>
#include <utility>
#include <memory>
#include <unordered_map>

#include "value.hpp"

namespace stdlib {
	struct library_t {
		const std::unordered_map<std::string, NativeFn> functions;
		const std::unordered_map<std::string, Value> constants;

		library_t(const std::unordered_map<std::string, NativeFn> functions,
				const std::unordered_map<std::string, Value> constants);
	};

	// Math Library, defined in Math.cpp file
	struct libmath : public library_t {
		libmath();
	};
	// Console and File Input/Output library, defined in StdIO.cpp
	struct libstdio : public library_t {
		libstdio();
	};
	// Array management library
	struct libnativearray : public library_t {
		libnativearray();
	};

	const std::unordered_map<std::string, std::shared_ptr<library_t>> Libraries({
		{ "math", std::make_shared<library_t>(libmath()) },
		{ "stdio", std::make_shared<library_t>(libstdio()) }
	});
}

#endif