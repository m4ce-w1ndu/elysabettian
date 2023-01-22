#include "library.hpp"

namespace stdlib {
	library_t::library_t(const std::unordered_map<std::string, NativeFn> functions,
					const std::unordered_map<std::string, Value> constants)
		: functions(functions), constants(constants)
	{
	}
}