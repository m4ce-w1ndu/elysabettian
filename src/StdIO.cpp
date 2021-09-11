#include "Library.hpp"

#include <optional>
#include <cstdio>
#include <cstring>

namespace Library {

	static inline std::optional<bool> ToBoolean(const char* value)
	{
		if (strcmp("false", value) == 0) return false;
		else if (strcmp("true", value) == 0) return true;
		return std::nullopt;
	}

	const auto MAX_LINE = 8192;

	StdIO::StdIO()
	: Library({
		// Functions
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
					if (!file->IsOpen()) {
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

		{ "fread", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "fread(file) expects 1 parameters. Got %d.'\n", argc);
				return std::monostate();
			}

			try {
				auto file = std::get<File>(*args);

				if (!file->IsOpen()) {
					fprintf(stderr, "Error: file %s is not open.\n", file->path.c_str());
					return std::monostate();
				}

				// going to end of file
				fseek(file->file, 0, SEEK_END);
				// getting file size in bytes
				auto fileSize = ftell(file->file);
				// resetting file pointer
				fseek(file->file, 0, SEEK_SET);
				// creating new buffer for file contents
				auto buff = std::make_unique<char>(fileSize);
				// reading contents
				fread(buff.get(), sizeof(char), fileSize, file->file);
				// creating string
				return std::string(buff.get());
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}},

		{ "freadLine", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "Error: freadLine(file) expects 1 parameter. Got %d.", argc);
				return std::monostate();
			}

			try {
				auto file = std::get<File>(*args);
				// reading file
				if (!file->IsOpen()) {
					fprintf(stderr, "Error: file %s is not open.\n", file->path.c_str());
					return std::monostate();
				}

				// line buffer
				char line[MAX_LINE];
				// reading single line
				fgets(line, MAX_LINE, file->file);
				// returning line
				return std::string(line);
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}},

		{ "isEof", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "Error: isEof(file) expects 1 parameter. Got %d.", argc);
				return std::monostate();
			}

			try {
				auto file = std::get<File>(*args);

				if (!file->IsOpen()) {
					fprintf(stderr, "Error: file %s is not open.\n", file->path.c_str());
					return std::monostate();
				}

				return static_cast<bool>(feof(file->file));
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}},

		{ "freset", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "Error: freset(file) expects 1 parameter. Got %d.", argc);
				return std::monostate();
			}

			try {
				auto file = std::get<File>(*args);

				if (!file->IsOpen()) {
					fprintf(stderr, "Error: file %s is not open.\n", file->path.c_str());
					return std::monostate();
				}

				fseek(file->file, 0, SEEK_SET);
				return file;
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}},

		{ "fclose", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc < 1 || argc > 1) {
				fprintf(stderr, "Error: fclose(file) expects 1 parameter. Got %d.", argc);
				return std::monostate();
			}
			try {
				auto file = std::get<File>(*args);
				if (!file->IsOpen())
					return file;

				fclose(file->file);
				return file;
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: file is not a file object." << std::endl;
				return std::monostate();
			}
		}},
		{ "read", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc > 1) {
				fprintf(stderr, "Error: read(prompt) expects 1 parameter. Got %d.", argc);
				return std::monostate();
			}

			try {
				if (argc > 0) {
					auto message = std::get<std::string>(*args);
					std::cout << message;
				}
				std::string input;
				std::getline(std::cin, input);

				// trying to parse input
				try {
					return std::stod(input);
				}
				catch (std::invalid_argument&) {
					auto asBoolean = ToBoolean(input.c_str());
					if (asBoolean.has_value()) return *asBoolean;

					return input;
				}
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: read(prompt) parameter must be of string or number type." << std::endl;
				return std::monostate();
			}
		}},
			
		{ "fprint", [](int argc, std::vector<Value>::iterator args) -> Value {
			if (argc > 2 || argc < 2) {
				fprintf(stderr, "Error: print(stream, message) expects 2 parameter. Got %d.", argc);
				return std::monostate();
			}
			try {
				auto stream = std::get<File>(*args);
				auto message = std::get<std::string>(*(args + 1));

				fprintf(stream->file, "%s", message.c_str());
				return stream->file;
			}
			catch (std::bad_variant_access&) {
				std::cerr << "Error: fprint(stream, message) parameters must be of stream and printable type." << std::endl;
				return std::monostate();
			}
		}}
	},

	// Constants
	{
		{ "stdin", stdin },
		{"stdout", stdout },
		{"stderr", stderr }
	}) {}
}
