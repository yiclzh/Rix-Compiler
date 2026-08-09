#pragma once
#include <string>

namespace rix {

    // Tag is a compile time only nominal marker (Prices, Returns, CovMatrix...)
    // It erases during codegen -- no runtime representation, no cost.

    struct Tag
    {
        /* data */
        std::string name;
        bool isVar = false;

        bool operator==(const Tag& other) const {
            return name == other.name && isVar == other.isVar;
        }

        bool operator!=(const Tag& other) const {
            return !(*this == other);
        }
    };

    // unified with anything
    inline const Tag UNTAGGED{"Untagged", false};
    


}