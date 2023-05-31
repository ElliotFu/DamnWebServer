#pragma once

#include <string>

enum Version
{
    UNKNOWN, HTTP10, HTTP11
};

std::string version_to_string(Version v)
{
    std::string result;
    switch (v)
    {
    case HTTP10:
        result = "1.0";
        break;
    case HTTP11:
        result = "1.1";
        break;
    default:
        result = "1.1";
        break;
    };

    return result;
}