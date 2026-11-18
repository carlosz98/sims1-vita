#include "far.h"

#include "binary_reader.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace pb {

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

void FarArchive::open(const std::string& path) {
    data_ = readFile(path);
    parse();
}

void FarArchive::openFromMemory(std::vector<uint8_t> data) {
    data_ = std::move(data);
    parse();
}

void FarArchive::parse() {
    BinaryReader r(data_);
    if (r.bytesAsString(8) != "FAR!byAZ") throw std::runtime_error("not a FAR archive");
    uint32_t version = r.u32le();
    if (version != 1) throw std::runtime_error("unsupported FAR version " + std::to_string(version));

    // v1a (uint32 name length) is what The Sims 1 ships; fall back to v1b.
    if (tryParseManifest(false)) { v1b_ = false; return; }
    if (tryParseManifest(true)) { v1b_ = true; return; }
    throw std::runtime_error("could not parse FAR manifest");
}

bool FarArchive::tryParseManifest(bool v1b) {
    entries_.clear();
    try {
        BinaryReader r(data_);
        r.seek(12);
        uint32_t manifestOffset = r.u32le();
        r.seek(manifestOffset);
        uint32_t count = r.u32le();
        if (count > 100000) return false;
        entries_.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            FarEntry e;
            e.size = r.u32le();
            e.size2 = r.u32le();
            e.offset = r.u32le();
            uint32_t nameLen = v1b ? r.u16le() : r.u32le();
            if (nameLen > 1024) return false;
            e.name = r.bytesAsString(nameLen);
            if ((uint64_t)e.offset + e.size > data_.size()) return false;
            entries_.push_back(std::move(e));
        }
        return r.remaining() == 0;
    } catch (const std::exception&) {
        entries_.clear();
        return false;
    }
}

const FarEntry* FarArchive::find(const std::string& name) const {
    std::string want = lower(name);
    for (const auto& e : entries_) {
        if (lower(e.name) == want) return &e;
    }
    return nullptr;
}

std::vector<uint8_t> FarArchive::read(const FarEntry& e) const {
    return std::vector<uint8_t>(data_.begin() + e.offset, data_.begin() + e.offset + e.size);
}

} // namespace pb
