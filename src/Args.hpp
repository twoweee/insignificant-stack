#pragma once

#include <string>
#include <unordered_map>
#include <cctype>
#include <cstring>

class Args {
public:
    void parse(int argc, char** argv);
    const std::string operator[](const std::string& key) const;
    std::string toString() const;

private:
    std::unordered_map<std::string, std::string> argsMap;
};