#include "Library.hpp"

#include <optional>

namespace Library {

	static inline std::optional<bool> ToBoolean(const char* value)
	{
		if (strcmp("false", value) == 0) return false;
		else if (strcmp("true", value) == 0) return true;
		return std::nullopt;
	}

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

	StdIO::StdIO()
	: Library(
		// Functions
		{
			{ "fopen", [](int argc, std::vector<Value>::iterator args) -> Value {
				if (argc < 2 || argc > 2) {
					fprintf(stderr, "fopen(filename, mode) expects 2 parameters. Got %d.\n", argc);
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

					return std::string((std::istreambuf_iterator<char>(file->file)),
						std::istreambuf_iterator<char>());
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
					fprintf(stderr, "Error: isEof(file) expects 1 parameter. Got %d.", argc);
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

					file->file.seekg(0);
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

					file->file.close();
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
					auto stream = std::get<NativeOutputStream>(*args);
					auto message = *(args + 1);

					(*stream) << message;
					return stream;
				}
				catch (std::bad_variant_access&) {
					try {
						auto file = std::get<File>(*args);
						auto message = *(args + 1);

						auto& stream = file->file;
						stream << message;
						return file;
					}
					catch (std::bad_variant_access&) {
						std::cerr << "Error: fprint(stream, message) parameters must be of stream and printable type." << std::endl;
						return std::monostate();
					}
					std::cerr << "Error: fprint(stream, message) parameters must be of stream and printable type." << std::endl;
					return std::monostate();
				}
			}}
		},

		// Constants
		{
			{ "stdin", std::make_shared<std::ifstream>(stdin) },
			{ "stdout", std::make_shared<std::ofstream>(stdout) },
			{ "stderr", std::make_shared<std::ofstream>(stderr) }
		}
	) {}
}