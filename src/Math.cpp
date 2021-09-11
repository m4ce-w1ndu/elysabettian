#include "Library.hpp"

#include <random>
#include <cmath>

namespace Library {
	Math::Math() : Library(
		// Functions
		{ 
            {"acos", [](int argc, std::vector<Value>::iterator args) -> Value {
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
            {"abs", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "abs(x) expects 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto val = std::get<double>(*args);
                    return std::abs(val);
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
            { "acosh", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::acosh(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "asin", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::asin(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "asinh", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::asinh(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "atan2", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 2) {
                    fprintf(stderr, "Error: expected 2 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    auto value1 = std::get<double>(*(args + 1));
                    return std::atan2(value, value1);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "atanh", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::atanh(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "cbrt", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::cbrt(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "ceil", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::ceil(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "cos", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::cos(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "cosh", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::cosh(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "exp", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::exp(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "expm1", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::expm1(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "floor", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::floor(value);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "roundf", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto value = std::get<double>(*args);
                    return std::roundf(static_cast<float>(value));
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "hypot", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 2) {
                    fprintf(stderr, "Error: expected 2 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto a = std::get<double>(*args);
                    auto b = std::get<double>(*(args + 1));
                    return std::hypot(a, b);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "log", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto a = std::get<double>(*args);
                    return std::log(a);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "log10", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto a = std::get<double>(*args);
                    return std::log10(a);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "log1p", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto a = std::get<double>(*args);
                    return std::log1p(a);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "log2", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto a = std::get<double>(*args);
                    return std::log2(a);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "max", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc < 1) {
                    fprintf(stderr, "Error: expected at least 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto max = std::get<double>(*args);
                    for (int i = 0; i < argc; ++i)
                        if (std::get<double>(*(args + i)) > max)
                            max = std::get<double>(*(args + 1));
                    return max;
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "min", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc < 1) {
                    fprintf(stderr, "Error: expected at least 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto min = std::get<double>(*args);
                    for (int i = 0; i < argc; ++i)
                        if (std::get<double>(*(args + i)) < min)
                            min = std::get<double>(*(args + 1));
                    return min;
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "random", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 2) {
                    fprintf(stderr, "Error: expected at least 2 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto min = std::get<double>(*args);
                    auto max = std::get<double>(*(args + 1));
                    std::default_random_engine reng(std::random_device{}());
                    std::uniform_real_distribution<double> dist(min, max);
                    return dist(reng);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "signbit", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected at least 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto min = std::get<double>(*args);
                    return std::signbit(min);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "sin", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected at least 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto min = std::get<double>(*args);
                    return std::sin(min);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
                    return "";
                }
            }},
            { "sinh", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc != 1) {
                    fprintf(stderr, "Error: expected at least 1 argument. Got %d.", argc);
                    return std::monostate();
                }
                try {
                    auto min = std::get<double>(*args);
                    return std::sinh(min);
                }
                catch (std::bad_variant_access) {
                    std::cerr << "Operand must be a number.";
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
		},

		// Constants
		{
            { "PI", std::atan(1.0) * 4 }
        }
	) {}
}