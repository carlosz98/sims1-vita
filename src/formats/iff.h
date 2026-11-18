#pragma once
// IFF container reader (The Sims 1 variant).
// Layout:
//   char[60] header text, e.g. "IFF FILE 2.5:TYPE FOLLOWED BY SIZE\0 JAMIE DOLLAR 2001"
//   uint32 BE  offset of the rsmp (resource map) chunk, 0 if none
//   chunks, each with a 76-byte header (big-endian):
//     char[4] type, uint32 size (includes this header), uint16 id, uint16 flags, char[64] label
// Chunk payloads themselves are little-endian.

#include <cstdint>
#include <string>
#include <vector>

namespace pb {

struct IffChunk {
    std::string type;      // "BHAV", "OBJD", "STR#", "TTAB", "SPR2", ...
    uint16_t id = 0;
    uint16_t flags = 0;
    std::string label;
    std::vector<uint8_t> data;   // payload, header stripped
};

class IffFile {
public:
    void open(const std::string& path);
    void openFromMemory(const std::vector<uint8_t>& data);

    const std::vector<IffChunk>& chunks() const { return chunks_; }
    const IffChunk* find(const std::string& type, uint16_t id) const;
    std::vector<const IffChunk*> ofType(const std::string& type) const;
    const std::string& headerText() const { return header_; }

private:
    std::string header_;
    std::vector<IffChunk> chunks_;
};

} // namespace pb
