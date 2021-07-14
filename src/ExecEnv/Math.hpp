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
#include <random>

namespace Library {

    class Math : public GenericLibrary {
    private:
        const std::string name;
        const LibraryType functions;
    public:
        Math();
        const std::string& GetName() override;
        const LibraryType& GetFunctions() override;
    };
}

#endif