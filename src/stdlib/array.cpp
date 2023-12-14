#include "provider/library.h"

namespace stdlib {
    libnativearray::libnativearray()
    : library_t({

        // Functions
        {
            { "push", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc < 2) {
                    fmt::print(stderr, "Error: push(arr, [items...]) expects at least 2 paramters. Got {}.\n", argc);
                    return std::monostate();
                }

                // getting array
                try {
                    Array array = std::get<Array>(*args);
                    // inserting elements into array
                    for (int i = 1; i < argc; ++i)
                        array->values.push_back(*(args + i));
                    return *(args + 1);
                }
                catch (std::bad_variant_access&) {
                    fmt::print(stderr, "Error: expected type is array.\n");
					return std::monostate();
				}
            }},
            { "pop", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc < 2) {
                    fmt::print(stderr, "Error: push(arr, [items...]) expects 1 parameter. Got {}.\n", argc);
                    return std::monostate();
                }

                try {
                    Array array = std::get<Array>(*args);
                    array->values.pop_back();

                    return *(args + 1);
                }
                catch (std::bad_variant_access&) {
                    fmt::print(stderr, "Error: push(arr, item) works only on array types!\n");
                    return std::monostate();
                }
            }},
            { "len", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc > 1 || argc < 1) {
                    fmt::print(stderr, "Error: arrayLen(array) expects 1 paramters. Got {}.\n", argc);
                    return std::monostate();
                }

                try {
                    Array array = std::get<Array>(*args);
                    return static_cast<double>(array->values.size());
                }
                catch (std::bad_variant_access&) {
                    fmt::print(stderr, "Error: expected type is array.\n");
					return std::monostate();
				}
            }},
        },

        // Constants
        {}
    }) {}
}
