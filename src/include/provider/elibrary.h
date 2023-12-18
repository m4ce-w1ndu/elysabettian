#ifndef ELY_RT_LIBRARY_H
#define ELY_RT_LIBRARY_H

#include <string>
#include <utility>
#include <memory>
#include <unordered_map>

#include "runtime/value.h"

namespace stdlib {
	/**
	 * @brief Generic standard library template.
	*/
	struct ELibrary {
		// Library functions
		const std::unordered_map<std::string, NativeFn> functions;

		// Library static constant values
		const std::unordered_map<std::string, Value> constants;

		/**
		 * @brief Creates a new standard library function group.
		 * @param functions: functions to be included in this library.
		 * @param constants: fonstants to be included in this library.
		*/
		ELibrary(const std::unordered_map<std::string, NativeFn>& functions,
				 const std::unordered_map<std::string, Value>& constants)
		: functions(functions), constants(constants) {};
	};
}

#endif