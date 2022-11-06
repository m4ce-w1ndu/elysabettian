#include "library.hpp"

namespace stdlib {
	library_t::library_t(const std::unordered_map<std::string, native_fn_t> functions,
					const std::unordered_map<std::string, value_t> constants)
		: functions(functions), constants(constants)
	{
	}
}