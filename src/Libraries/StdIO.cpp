#include "Library.hpp"

#include <optional>
#include <cstdio>
#include <cstring>

namespace Library {

	static const inline bool is_number(const std::string& str)
	{
		// check if the string is empty
		if (str.empty())
			return false;

		// check if the string is a number
		for (char c : str) {
			if (!isdigit(c) && c != '.')
				return false;
		}

		return true;
	}

	// constexpr function to check if string is a valid boolean
	static const inline bool is_boolean(const std::string& str)
	{
		return str == "true" || str == "false";
	}


	StdIO::StdIO()
	: Library({
		// Console I/O
		{ "read", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc > 0 && argc < 2) {
				try {
					const auto param = std::get<std::string>(*args);
					std::cout << param;
				} catch (std::bad_variant_access&) {
					std::cerr << "Error: expected type is string." << std::endl;
					return std::monostate();
				}
			}

			std::string input;
			std::getline(std::cin, input);

			if (is_number(input))
				return std::stod(input);
			else if (is_boolean(input)) {
				if (input == "true")
					return true;
				else
					return false;
			}
			else
				return input;
		}},

		{ "fopen", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 2 || argc > 2) {
				fprintf(stderr, "fopen(filename, mode) expects 2 parameters. Got %d.\n", argc);
				return std::monostate();
			}
			try {
				auto name = std::get<std::string>(*args);
				auto mode = std::get<std::string>(*(args + 1));

				try {
					auto file = std::make_shared<FileObject>(name, mode);
					if (!file->is_open()) {
						fprintf(stderr, "Error: file %s is not open.\n", file->path.c_str());
						return std::monostate();
					}
					return file;
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

		{ "fclose", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "fclose(filestream) expects 1 parameter. Got %d.\n", argc);
				return std::monostate();
			}
			try {
				auto file_ptr = std::get<File>(*args);
				if (!file_ptr->is_open()) {
					fprintf(stderr, "Error: file %s is not open.\n", file_ptr->path.c_str());
					return std::monostate();
				}

				file_ptr->close();
				return file_ptr;

			} catch (std::bad_variant_access&) {
				std::cerr << "Error: expected type is {file}" << std::endl;
				return std::monostate();
			}
		}},

		{ "fflush", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "fflush(filestream) expects 1 parameter. Got %d.\n", argc);
				return std::monostate();
			}
			try {
				auto file_ptr = std::get<File>(*args);
				if (!file_ptr->is_open()) {
					fprintf(stderr, "Error: file %s is not open.\n", file_ptr->path.c_str());
					return std::monostate();
				}

				file_ptr->flush();
				return file_ptr;
			} catch (std::bad_variant_access&) {
				std::cerr << "Error: expected type is {file}" << std::endl;
				return std::monostate();
			}
		}},

		{ "fread", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "fread(filestream) expects 1 parameter. Got %d.\n", argc);
				return std::monostate();
			}
			try {
				auto file_ptr = std::get<File>(*args);
				if (!file_ptr->is_open()) {
					fprintf(stderr, "Error: file %s is not open.\n", file_ptr->path.c_str());
					return std::monostate();
				}

				return file_ptr->read_all();
			} catch (std::bad_variant_access&) {
				std::cerr << "Error: expected type is {file}" << std::endl;
				return std::monostate();
			}
		}},
		
		{ "fwrite", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 2 || argc > 2) {
				fprintf(stderr, "fwrite(filestream, data) expects 2 parameters. Got %d.\n", argc);
				return std::monostate();
			}
			try {
				auto file_ptr = std::get<File>(*args);
				if (!file_ptr->is_open()) {
					fprintf(stderr, "Error: file %s is not open.\n", file_ptr->path.c_str());
					return std::monostate();
				}

				auto data = std::get<std::string>(*(args + 1));
				file_ptr->write_all(data);
				return file_ptr;
			} catch (std::bad_variant_access&) {
				std::cerr << "Error: expected types are {file} {string}" << std::endl;
				return std::monostate();
			}
		}}
	},

	// Constants
	{
		{ "stdin",  stdin  },
		{ "stdout", stdout },
		{ "stderr", stderr }
	}) {}
}
