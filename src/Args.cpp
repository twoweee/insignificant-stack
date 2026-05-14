#include "Args.hpp"


void Args::parse(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        std::string key = argv[i];

        if (key.rfind("--", 0) == 0 && i + 1 < argc) {
            argsMap[key] = std::string(argv[i + 1]);
            i++;
        }
    }
}

// note for self:
// 1st const - return read only, 3rd const - dont modify Args obj
// & is reference, 2nd const - read only
// reference cant be null and cant change what it points to... 
const std::string Args::operator[](const std::string& key) const {
    auto argIter = argsMap.find(key);
    if (argIter == argsMap.end())
        return "\0";
    return argIter->second;
}

std::string Args::toString() const {
    std::string output;

    if (argsMap.size() < 1) {
        output = "No args parsed!\n";
    } else {
        output += "Args:\n";
        for (const auto& [key, value] : argsMap) {
            output += key + " = " + value + "\n";
        }
    }
    output += "\n";

    return output;
}