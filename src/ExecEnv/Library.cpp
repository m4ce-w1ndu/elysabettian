#include "CommonLibrary.hpp"
#include "Math.hpp"

#include <random>

namespace Library {
    // Libraries in static vars
    static Math math;

    // Libraries
    const static std::map<std::string, LibraryType> libraries {
        { math.GetName(), math.GetFunctions() }
    };

    static LibraryType emptyLib;

    const LibraryType& GetLibrary(const std::string& libName)
    {
        auto it = libraries.find(libName);
        if (it != libraries.end())
            return it->second;
        return emptyLib;
    }
}
