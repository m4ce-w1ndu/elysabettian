//
//  Math.hpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#ifndef MATH_HPP
#define MATH_HPP

#include "VirtualMachine.hpp"
#include "CommonLibrary.hpp"

#include <utility>
#include <vector>
#include <string>
#include <memory>

namespace Library {
    class Math : public Library {
    private:
    public:
        Math(const LibraryType& libFunctions)
        : Library("math", libFunctions) {}
    };
}

#endif