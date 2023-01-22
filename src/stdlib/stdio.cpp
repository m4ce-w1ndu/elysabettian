#include "library.hpp"

#include <optional>
#include <cstdio>
#include <cstring>

namespace stdlib {

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

	libstdio::libstdio()
	: library_t({
		// Console I/O
		{ "read", [](int argc, std::vector<value_t>::iterator args) -> value_t {
			if (argc > 0 && argc < 2) {
				try {
					const auto param = std::get<std::string>(*args);
					fmt::print("{}", param);
				} catch (std::bad_variant_access&) {
					fmt::print(stderr, "Error: expected type is string.\n");
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

		{ "fopen", [](int argc, std::vector<value_t>::iterator args) -> value_t {
			if (argc < 2 || argc > 2) {
				fmt::print(stderr, "fopen(filename, mode) expects 2 parameters. Got {}.\n", argc);
				return std::monostate();
			}
			try {
				auto name = std::get<std::string>(*args);
				auto mode = std::get<std::string>(*(args + 1));

				try {
					auto file = std::make_shared<file_obj>(name, mode);
					if (!file->is_open()) {
						fmt::print(stderr, "Error: file {} is not open.\n", file->path.c_str());
						return std::monostate();
					}
					return file;
				}
				catch (std::out_of_range&) {
					fmt::print(stderr, "Error: invalid file mode.");
					return std::monostate();
				}
			}
			catch (std::bad_variant_access&) {
				fmt::print(stderr, "Error: expected types are {string} {string}\n");
				return std::monostate();
			}
		}},

		{ "fclose", [](int argc, std::vector<value_t>::iterator args) -> value_t {
			if (argc < 1 || argc > 1) {
				fmt::print(stderr, "fclose(filestream) expects 1 parameter. Got {}.\n", argc);
				return std::monostate();
			}
			try {
				auto file_ptr = std::get<file_t>(*args);
				if (!file_ptr->is_open()) {
					fmt::print(stderr, "Error: file {} is not open.\n", file_ptr->path.c_str());
					return std::monostate();
				}

				file_ptr->close();
				return file_ptr;

			} catch (std::bad_variant_access&) {
				fmt::print(stderr, "Error: expected type is {file}\n");
				return std::monostate();
			}
		}},

		{ "fflush", [](int argc, std::vector<value_t>::iterator args) -> value_t {
			if (argc < 1 || argc > 1) {
				fmt::print(stderr, "fflush(filestream) expects 1 parameter. Got {}.\n", argc);
				return std::monostate();
			}
			try {
				auto file_ptr = std::get<file_t>(*args);
				if (!file_ptr->is_open()) {
					fmt::print(stderr, "Error: file {} is not open.\n", file_ptr->path.c_str());
					return std::monostate();
				}

				file_ptr->flush();
				return file_ptr;
			} catch (std::bad_variant_access&) {
				fmt::print(stderr, "Error: expected type is {file}\n");
				return std::monostate();
			}
		}},

		{ "fread", [](int argc, std::vector<value_t>::iterator args) -> value_t {
			if (argc < 1 || argc > 1) {
				fmt::print(stderr, "fread(filestream) expects 1 parameter. Got {}.\n", argc);
				return std::monostate();
			}
			try {
				auto file_ptr = std::get<file_t>(*args);
				if (!file_ptr->is_open()) {
					fmt::print(stderr, "Error: file {} is not open.\n", file_ptr->path.c_str());
					return std::monostate();
				}

				return file_ptr->read_all();
			} catch (std::bad_variant_access&) {
				fmt::print(stderr, "Error: expected type is {file}\n");
				return std::monostate();
			}
		}},
		
		{ "fwrite", [](int argc, std::vector<value_t>::iterator args) -> value_t {
			if (argc < 2 || argc > 2) {
				fmt::print(stderr, "fwrite(filestream, data) expects 2 parameters. Got {}.\n", argc);
				return std::monostate();
			}
			try {
				auto file_ptr = std::get<file_t>(*args);
				if (!file_ptr->is_open()) {
					fmt::print(stderr, "Error: file {} is not open.\n", file_ptr->path.c_str());
					return std::monostate();
				}

				auto data = std::get<std::string>(*(args + 1));
				file_ptr->write_all(data);
				return file_ptr;
			} catch (std::bad_variant_access&) {
				fmt::print(stderr, "Error: expected types are {file} {string}\n");
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
