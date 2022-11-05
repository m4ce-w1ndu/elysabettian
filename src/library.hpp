#ifndef LIBRARY_HPP
#define LIBRARY_HPP

#include <string>
#include <utility>
#include <memory>
#include <unordered_map>

#include "value.hpp"

namespace Library {
	struct Library {
		const std::unordered_map<std::string, NativeFn> functions;
		const std::unordered_map<std::string, Value> constants;

		Library(const std::unordered_map<std::string, NativeFn> functions,
				const std::unordered_map<std::string, Value> constants);
	};

	// Math Library, defined in Math.cpp file
	struct Math : public Library {
		Math();
	};
	// Console and File Input/Output library, defined in StdIO.cpp
	struct StdIO : public Library {
		StdIO();
	};
	// Array management library
	struct NativeArray : public Library {
		NativeArray();
	};

	const std::unordered_map<std::string, std::shared_ptr<Library>> Libraries({
		{ "math", std::make_shared<Library>(Math()) },
		{ "stdio", std::make_shared<Library>(StdIO()) }
	});
}

#endif