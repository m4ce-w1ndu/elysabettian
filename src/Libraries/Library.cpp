#include "Library.hpp"

namespace Library {
	Library::Library(const std::unordered_map<std::string, NativeFn> functions,
					const std::unordered_map<std::string, Value> constants)
		: functions(functions), constants(constants)
	{
	}
}