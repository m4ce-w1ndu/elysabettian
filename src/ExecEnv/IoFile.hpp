#ifndef IO_FILE_HPP
#define IO_FILE_HPP

#include "CommonLibrary.hpp"

namespace Library {
	class IoFile : public GenericLibrary {
	private:
		const std::string name;
		const LibraryType functions;
	public:
		IoFile();
		const std::string& GetName() override;
		const LibraryType& GetFunctions() override;
	};
}

#endif