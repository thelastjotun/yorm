#pragma once

#include <algorithm>
#include <string>

extern "C" {
#include <libyang.h>
}

std::string sanitize_name(std::string name)
{
    if (name.empty()) {
        return name;
    }

    std::replace(name.begin(), name.end(), '-', '_');
    std::replace(name.begin(), name.end(), '.', '_');
    std::replace(name.begin(), name.end(), '/', '_');
    std::replace(name.begin(), name.end(), '+', '_');

    return name;
}

std::string get_cpp_type(const lysc_type *type)
{
    if (!type) {
        return "std::string";
    }

    switch (type->basetype) {
    case LY_TYPE_STRING:
        return "std::string";
    case LY_TYPE_UINT16:
        return "uint16_t";
    case LY_TYPE_UINT32:
        return "uint32_t";
    case LY_TYPE_INT16:
        return "int16_t";
    case LY_TYPE_INT32:
        return "int32_t";
    case LY_TYPE_BOOL:
        return "bool";

    default:
        return "std::string";
    }
}
