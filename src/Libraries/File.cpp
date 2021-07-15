#include "Library.hpp"

#include <fstream>

namespace Library {

	// File modes
	static const std::unordered_map<std::string, std::ios::openmode> OpenModes({
		{"r", std::ios::in},
		{"r+", std::ios::in | std::ios::out},
		{"w", std::ios::out},
		{"w+", std::ios::in | std::ios::out | std::ios::trunc},
		{"a", std::ios::app},
		{"a+", std::ios::out | std::ios::in | std::ios::app},
		{"rb", std::ios::binary | std::ios::in},
		{"wb", std::ios::binary | std::ios::out},
		{"ab", std::ios::binary | std::ios::app},
		{"r+b", std::ios::binary | std::ios::out | std::ios::in},
		{"w+b", std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc},
		{"a+b", std::ios::binary | std::ios::out | std::ios::in | std::ios::app}
	});

	FileIO::FileIO()
	: Library(
		// Functions
		{
			{ "open", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 2 || argc > 2) {
				fprintf(stderr, "open(filename, mode) expects 2 parameters. Got %d.\n", argc);
				return std::monostate();
			}
			try {
				auto name = std::get<std::string>(*args);
				auto mode = std::get<std::string>(*(args + 1));

				
				try {
					auto fileMode = OpenModes.at(mode);
					return std::make_shared<FileObject>(name, fileMode);
				}
				catch (std::out_of_range&) {
					std::cerr << "Error: invalid file mode." << std::endl;
					return std::monostate();
				}
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

				if (!file->IsOpen()) {
					fprintf(stderr, "Error: file %s is not open.\n", file->path.c_str());
					return std::monostate();
				}

				return std::string((std::istreambuf_iterator<char>(file->file)),
					std::istreambuf_iterator<char>());
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}},

		{ "readLine", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "Error: readLine(file) expects 1 parameter. Got %d.", argc);
				return std::monostate();
			}

			try {
				auto file = std::get<File>(*args);
				// reading file
				if (!file->IsOpen()) {
					fprintf(stderr, "Error: file %s is not open.\n", file->path.c_str());
					return std::monostate();
				}
				
				std::string line;
				std::getline(file->file, line);
				return line;
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}},

		{ "isEof", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "Error: readLine(file) expects 1 parameter. Got %d.", argc);
				return std::monostate();
			}

			try {
				auto file = std::get<File>(*args);

				if (!file->IsOpen()) {
					fprintf(stderr, "Error: file %s is not open.\n", file->path.c_str());
					return std::monostate();
				}

				return file->file.eof();
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}},

		{ "reset", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "Error: readLine(file) expects 1 parameter. Got %d.", argc);
				return std::monostate();
			}

			try {
				auto file = std::get<File>(*args);

				if (!file->IsOpen()) {
					fprintf(stderr, "Error: file %s is not open.\n", file->path.c_str());
					return std::monostate();
				}

				file->file.seekg(0);
				return file;
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}},

		{ "close", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "Error: readLine(file) expects 1 parameter. Got %d.", argc);
				return std::monostate();
			}
			try {
				auto file = std::get<File>(*args);
				if (!file->IsOpen())
					return file;

				file->file.close();
				return file;
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}}
		},

		// Constants
		{}
	) {}
}