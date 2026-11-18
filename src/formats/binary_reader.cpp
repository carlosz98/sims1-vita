#include "binary_reader.h"

#include <cstdio>

namespace pb {

std::vector<uint8_t> readFile(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) throw std::runtime_error("cannot open file: " + path);
    std::fseek(f, 0, SEEK_END);
    long len = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> buf(len > 0 ? (size_t)len : 0);
    if (!buf.empty() && std::fread(buf.data(), 1, buf.size(), f) != buf.size()) {
        std::fclose(f);
        throw std::runtime_error("short read: " + path);
    }
    std::fclose(f);
    return buf;
}

} // namespace pb
