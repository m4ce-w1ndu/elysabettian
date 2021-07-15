#ifndef LIBRARY_HPP
#define LIBRARY_HPP

#include <string>
#include <utility>
#include <memory>
#include <unordered_map>

#include <ExecEnv/Value.hpp>

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
	// Input/Output file library, defined in File.cpp
	struct FileIO : public Library {
		FileIO();
	};

	const std::unordered_map<std::string, std::shared_ptr<Library>> Libraries({
		{ "math", std::make_shared<Library>(Math()) },
		{ "io", std::make_shared<Library>(FileIO()) }
	});
}

#endif