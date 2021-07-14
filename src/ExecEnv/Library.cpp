#include "CommonLibrary.hpp"
#include "Math.hpp"
#include "String.hpp"

#include <random>

namespace Library {
    // Libraries in static vars
    static Math math;
    static String string;

    // Libraries
    const static std::map<std::string, LibraryType> libraries {
        { math.GetName(), math.GetFunctions() },
        { string.GetName(), string.GetFunctions() }
    };

    // Empty library constant.
    const static LibraryType emptyLib;

    // Get library by name
    const LibraryType& GetLibrary(const std::string& libName)
    {
        auto it = libraries.find(libName);
        if (it != libraries.end())
            return it->second;
        return emptyLib;
    }
}
