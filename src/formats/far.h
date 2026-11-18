#pragma once
// FAR archive reader (The Sims 1 uses FAR version 1).
// Layout (little-endian):
//   char[8]  "FAR!byAZ"
//   uint32   version (1)
//   uint32   manifest offset
//   manifest: uint32 count, then per entry:
//     uint32 dataLength, uint32 dataLength2, uint32 dataOffset,
//     filename length (uint32 in v1a, uint16 in v1b), filename bytes

#include <cstdint>
#include <string>
#include <vector>

namespace pb {

struct FarEntry {
    std::string name;
    uint32_t size = 0;
    uint32_t size2 = 0;
    uint32_t offset = 0;
};

class FarArchive {
public:
    // Loads the whole archive into memory. On the Vita we will later switch
    // to reading entries on demand from the file to save RAM.
    void open(const std::string& path);
    void openFromMemory(std::vector<uint8_t> data);

    const std::vector<FarEntry>& entries() const { return entries_; }
    const FarEntry* find(const std::string& name) const;   // case-insensitive
    std::vector<uint8_t> read(const FarEntry& e) const;
    bool isV1b() const { return v1b_; }

private:
    void parse();
    bool tryParseManifest(bool v1b);

    std::vector<uint8_t> data_;
    std::vector<FarEntry> entries_;
    bool v1b_ = false;
};

} // namespace pb
