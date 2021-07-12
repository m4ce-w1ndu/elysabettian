#ifndef COMMONLIBRARY_HPP
#define COMMONLIBRARY_HPP

#include "Value.hpp"

#include <map>
#include <string>
#include <vector>

namespace Library {
	using FuncType = std::pair<std::string, NativeFn>;
	using LibraryType = std::vector<FuncType>;

	class Library {
		LibraryType libFunctions;
		std::string libName;
	public:
		Library(const std::string& libName, const LibraryType& libFunctions)
			: libName(libName), libFunctions(std::move(libFunctions)) {}
		virtual LibraryType GetFunctions()
		{
			return libFunctions;
		}
		virtual std::string GetName()
		{
			return libName;
		}
	};

	const LibraryType& GetFunctions(const std::string& name);
}

#endif