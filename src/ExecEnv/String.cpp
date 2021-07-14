#include "String.hpp"

namespace Library {
	String::String()
		: name("string"), functions({
			{ "strlen", [](int argc, std::vector<Value>::iterator args) -> Value {
				if (argc < 1 || argc > 2) {
					fprintf(stderr, "strlen(str) expects 1 parameter. Got %d.", argc);
					return std::monostate();
				}
				try {
					auto value = std::get<std::string>(*args);
					return static_cast<double>(value.length());
				}
				catch (std::bad_variant_access) {
					std::cerr << "Parameter must be of string type.";
					return std::monostate();
				}
			}},
			{ "strcat",[](int argc, std::vector<Value>::iterator args) -> Value {
				if (argc < 2 || argc > 2) {
					fprintf(stderr, "strcat(str1, str2) expects 2 parameter. Got %d.", argc);
					return std::monostate();
				}
				try {
					auto str1 = std::get<std::string>(*args);
					auto str2 = std::get<std::string>(*(args + 1));
					return str1 + str2;
				}
				catch (std::bad_variant_access) {
					std::cerr << "Parameter must be of string type.";
					return std::monostate();
				}
			}},
			{ "tostring", [](int argc, std::vector<Value>::iterator args) -> Value {
				if (argc < 1 || argc > 1) {
					fprintf(stderr, "tostring(obj) expects 1 parameter. Got %d.", argc);
					return std::monostate();
				}
				try {
					auto val = std::get<double>(*args);
					return std::to_string(val);
				}
				catch (std::bad_variant_access) {
					auto val = std::get<std::monostate>(*args);
					return "null";
				}
				catch (std::exception) {
					std::cerr << "Unknown conversion type.";
					return std::monostate();
				}
			}}
		})
	{}

	const std::string& String::GetName()
	{
		return name;
	}

	const LibraryType& String::GetFunctions()
	{
		return functions;
	}
}