#include "IoFile.hpp"

#include <cstdio>

namespace Library {

	IoFile::IoFile()
	: name("io"), functions({

		{ "open", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 2 || argc > 2) {
				fprintf(stderr, "open(filename, mode) expects 2 parameters. Got %d.\n", argc);
				return std::monostate();
			}
			try {
				auto name = std::get<std::string>(*args);
				auto mode = std::get<std::string>(*(args + 1));

				auto ret = std::make_shared<FileObject>(name, mode);
				if (!ret->isOpen) return std::monostate();
				return ret;
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: expected types are {string} {string}" << std::endl;
				return std::monostate();
			}
		}},

		{ "read", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "read(file) expects 1 parameters. Got %d.'\n", argc);
				return std::monostate();
			}

			try {
				auto file = std::get<File>(*args);

				if (!file->isOpen) {
					fprintf(stderr, "Error: file %s is not open.\n", file->name.c_str());
					return std::monostate();
				}

				fseek(file->file, 0, SEEK_END);
				auto fsize = ftell(file->file);
				fseek(file->file, 0, SEEK_SET);

				// Allocating new string
				char* buf = new char[fsize];
				fread(buf, sizeof(char), fsize, file->file);
				std::string newStr(buf);

				// deleting temp buffer
				delete[] buf;
				return newStr;
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}}
	})
	{

	}

	const std::string& IoFile::GetName()
	{
		return name;
	}

	const LibraryType& IoFile::GetFunctions()
	{
		return functions;
	}
}