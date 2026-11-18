#include "iff.h"

#include "binary_reader.h"

#include <stdexcept>

namespace pb {

static constexpr size_t kFileHeaderSize = 64;
static constexpr size_t kChunkHeaderSize = 76;

void IffFile::open(const std::string& path) {
    openFromMemory(readFile(path));
}

void IffFile::openFromMemory(const std::vector<uint8_t>& data) {
    chunks_.clear();
    BinaryReader r(data);
    header_ = r.fixedString(60);
    if (header_.rfind("IFF FILE", 0) != 0) throw std::runtime_error("not an IFF file");
    r.u32be(); // rsmp offset (we scan chunks directly instead)

    while (r.remaining() >= kChunkHeaderSize) {
        size_t start = r.pos();
        IffChunk c;
        c.type = r.bytesAsString(4);
        uint32_t size = r.u32be();
        c.id = r.u16be();
        c.flags = r.u16be();
        c.label = r.fixedString(64);
        if (size < kChunkHeaderSize || start + size > data.size()) {
            throw std::runtime_error("bad chunk size in " + c.type + " #" + std::to_string(c.id));
        }
        size_t payload = size - kChunkHeaderSize;
        c.data.resize(payload);
        if (payload) r.bytes(c.data.data(), payload);
        chunks_.push_back(std::move(c));
    }
    (void)kFileHeaderSize;
}

const IffChunk* IffFile::find(const std::string& type, uint16_t id) const {
    for (const auto& c : chunks_)
        if (c.type == type && c.id == id) return &c;
    return nullptr;
}

std::vector<const IffChunk*> IffFile::ofType(const std::string& type) const {
    std::vector<const IffChunk*> out;
    for (const auto& c : chunks_)
        if (c.type == type) out.push_back(&c);
    return out;
}

} // namespace pb
