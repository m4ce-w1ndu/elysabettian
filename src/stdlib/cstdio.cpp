#include "provider/cstdio.h"

#include <cstdio>

namespace stdlib {

	CStdio::CStdio() : ELibrary(
		{{ "puts", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc != 1) {
				fmt::print(stderr, "Error: puts(str) expects 1 argument. Got {}.\n", argc);
				return std::monostate();
			}

			try {
				const auto& str = std::get<std::string>(*args);
				return static_cast<double>(std::puts(str.c_str()));
			}
			catch (std::bad_variant_access&) {
				fmt::print(stderr, "Error: puts(str) expects a string argument.\n");
				return std::monostate();
			}
		}},
		

		{ "putc", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc != 2) {
				fmt::print(stderr, "Error: putc(char, stream) expects 2 arguments. Got {}\n", argc);
				return std::monostate();
			}

			try {
				const auto& str = std::get<std::string>(*args);
				const auto& stream = std::get<FILE*>(*(args + 1));

				if (nullptr == stream) {
					fmt::print(stderr, "Error: output stream cannot be null!\n");
					return std::monostate();
				}

				if (str.length() > 1) {
					fmt::print(stderr, "Error: putc(char, stream) expects a single char argument!\n");
					return std::monostate();
				}

				return static_cast<bool>(std::putc(static_cast<int>(str[0]), stream));
			}
			catch (std::bad_variant_access&) {
				fmt::print(stderr, "Error: invalid type\n");
				return std::monostate();
			}
		}},

		{ "putchar", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc != 1) {
				fmt::print(stderr, "Error: putchar(char) expects 1 argument. Got {}.\n", argc);
				return std::monostate();
			}

			try {
				const auto& str = std::get<std::string>(*args);

				if (str.length() > 1) {
					fmt::print(stderr, "Error: putchar(char) expects a single char argument!\n");
					return std::monostate();
				}

				return static_cast<double>(std::putchar(static_cast<int>(str[0])));
			}
			catch (std::bad_variant_access&) {
				fmt::print(stderr, "Error: putchar(char) expects a single char argument!\n");
				return std::monostate();
			}
		}}
		
		},
		{ { "stdin", stdin },
		  { "stdout", stdout },
		  { "stderr", stderr } }
	) {};

}