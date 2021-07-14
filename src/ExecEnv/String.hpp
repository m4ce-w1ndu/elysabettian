#ifndef STRING_HPP
#define STRING_HPP

#include "VirtualMachine.hpp"
#include "CommonLibrary.hpp"

#include <utility>
#include <vector>
#include <string>
#include <memory>

namespace Library {
	class String : public GenericLibrary {
	private:
		const std::string name;
		const LibraryType functions;
	public:
		String();
		const std::string& GetName() override;
		const LibraryType& GetFunctions() override;
	};
}

#endif