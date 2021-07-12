#include "CommonLibrary.hpp"
#include "Math.hpp"

namespace Library {

    Math math( {
        { "acos", [](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1 || argc > 1) {
                fprintf(stderr, "Error: acos(x) expectes 1 argument. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto value = std::get<double>(*args);
                return std::acos(value);
            }
        catch (std::bad_variant_access) {
            fprintf(stderr, "Error: acos(x) operand must be of numeric type.");
            return std::monostate();
        }
        }},
        { "abs", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "abs(x) expects 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto val = std::get<double>(*args);
                    if (val < 0) return -val;
                    return val;
                }
        catch (std::bad_variant_access) {
            fprintf(stderr, "Operand must be of numeric type.");
            return std::monostate();
        }
        }},
        { "pow", [](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc != 2) {
                fprintf(stderr, "Error: expected 2 arguments. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto base = std::get<double>(*args);
                auto ex = std::get<double>(*(args + 1));
                return std::pow(base, ex);
            }
        catch (std::bad_variant_access) {
            std::cerr << "Operands must be numbers.";
            return std::monostate();
        }
        catch (std::length_error) {
            return std::monostate();
        }
        }},
        { "sqrt", [](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc != 1) {
                fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto value = std::get<double>(*args);
                return sqrt(value);
            }
        catch (std::bad_variant_access) {
            std::cerr << "Operand must be a number.";
            return "";
        }
        catch (std::length_error) {
            return "";
        }
        }},
        { "sum", [](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1) {
                fprintf(stderr, "Error: expected at least 1 argument. Got %d", argc);
                return std::monostate();
            }
            double sumVal = 0;
            try {
                for (int i = 0; i < argc; ++i)
                    sumVal += std::get<double>(*(args + i));
                return sumVal;
            }
            catch (std::bad_variant_access) {
                fprintf(stderr, "Invalid operand detected in sum(). Only numbers are allowed.");
                return "";
            }
        }}
    });

    // Libraries
    std::map<std::string, LibraryType> libraries {
        { "math", math.GetFunctions() }
    };

    static LibraryType emptyLib;

    const LibraryType& GetFunctions(const std::string& libName)
    {
        auto it = libraries.find(libName);
        if (it != libraries.end())
            return it->second;
        return emptyLib;
    }
}
