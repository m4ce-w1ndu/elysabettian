#ifndef COMMONLIBRARY_HPP
#define COMMONLIBRARY_HPP

#include "Value.hpp"

#include <map>
#include <string>
#include <vector>

namespace Library {
	using FuncType = std::pair<std::string, NativeFn>;
	using LibraryType = std::vector<FuncType>;

	const LibraryType& GetLibrary(const std::string& name);

	class GenericLibrary {
	public:
		virtual const std::string& GetName() = 0;
		virtual const LibraryType& GetFunctions() = 0;
	};
}

#endif