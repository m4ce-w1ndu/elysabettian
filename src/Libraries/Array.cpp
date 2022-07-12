#include "Library.hpp"

namespace Library {
    NativeArray::NativeArray()
    : Library({

        // Functions
        {
            { "__Array", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc > 0) {
                    fprintf(stderr, "Error: Array() constructor expects 0 paramters. Got %d.\n", argc);
                    return std::monostate();
                }
                return std::make_shared<ArrayObject>();
            }},
            { "__arrayPush", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc < 2) {
                    fprintf(stderr, "Error: arrayPush(array, [items...]) expects at least 2 paramters. Got %d.\n", argc);
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
					std::cerr << "Error: expected type is array." << std::endl;
					return std::monostate();
				}
            }},
            { "__arrayGet", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc > 2 || argc < 2) {
                    fprintf(stderr, "Error: arrayGet(array, index) expects 2 paramters. Got %d.\n", argc);
                    return std::monostate();
                }

                try {
                    Array array = std::get<Array>(*args);
                    // getting index
                    try {
                        auto index = static_cast<size_t>(std::get<double>(*(args + 1)));

                        return array->values[index];
                    }
                    catch (std::bad_variant_access&) {
                        std::cerr << "Error: expected type is number." << std::endl;
                        return std::monostate();
                    }
                }
                catch (std::bad_variant_access&) {
					std::cerr << "Error: expected type is array." << std::endl;
					return std::monostate();
				}
            }},
            { "__arraySet", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc > 3 || argc < 3) {
                    fprintf(stderr, "Error: arraySet(array, index, item) expects 3 paramters. Got %d.\n", argc);
                    return std::monostate();
                }

                size_t index;
                try {
                    Array array = std::get<Array>(*args);
                    // getting index
                    try {
                        index = static_cast<size_t>(std::get<double>(*(args + 1)));
                    }
                    catch (std::bad_variant_access&) {
                        std::cerr << "Error: expected type is number." << std::endl;
                        return std::monostate();
                    }

                    // setting value
                    array->values[index] = *(args + 2);
                    return *(args + 2);
                }
                catch (std::bad_variant_access&) {
					std::cerr << "Error: expected type is array." << std::endl;
					return std::monostate();
                }
            }},
            { "__arrayLen", [](int argc, std::vector<Value>::iterator args) -> Value {
                if (argc > 1 || argc < 1) {
                    fprintf(stderr, "Error: arrayLen(array) expects 1 paramters. Got %d.\n", argc);
                    return std::monostate();
                }

                try {
                    Array array = std::get<Array>(*args);
                    return static_cast<double>(array->values.size());
                }
                catch (std::bad_variant_access&) {
					std::cerr << "Error: expected type is array." << std::endl;
					return std::monostate();
				}
            }},
        },

        // Constants
        {}
    }) {}
}
