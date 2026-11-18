#pragma once
// Small bounds-checked reader over a byte buffer.
// The Sims 1 mixes endianness: IFF chunk headers are big-endian,
// chunk contents (BHAV, STR#, ...) and FAR archives are little-endian.

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace pb {

class BinaryReader {
public:
    BinaryReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}
    explicit BinaryReader(const std::vector<uint8_t>& buf) : data_(buf.data()), size_(buf.size()) {}

    size_t pos() const { return pos_; }
    size_t size() const { return size_; }
    size_t remaining() const { return size_ - pos_; }
    void seek(size_t p) {
        if (p > size_) throw std::runtime_error("seek out of range");
        pos_ = p;
    }
    void skip(size_t n) { need(n); pos_ += n; }

    uint8_t u8() { need(1); return data_[pos_++]; }

    uint16_t u16le() { need(2); uint16_t v = data_[pos_] | (data_[pos_ + 1] << 8); pos_ += 2; return v; }
    uint32_t u32le() {
        need(4);
        uint32_t v = (uint32_t)data_[pos_] | ((uint32_t)data_[pos_ + 1] << 8) |
                     ((uint32_t)data_[pos_ + 2] << 16) | ((uint32_t)data_[pos_ + 3] << 24);
        pos_ += 4;
        return v;
    }
    uint16_t u16be() { need(2); uint16_t v = (data_[pos_] << 8) | data_[pos_ + 1]; pos_ += 2; return v; }
    uint32_t u32be() {
        need(4);
        uint32_t v = ((uint32_t)data_[pos_] << 24) | ((uint32_t)data_[pos_ + 1] << 16) |
                     ((uint32_t)data_[pos_ + 2] << 8) | (uint32_t)data_[pos_ + 3];
        pos_ += 4;
        return v;
    }
    int16_t s16le() { return (int16_t)u16le(); }

    std::string fixedString(size_t n) {
        need(n);
        const char* p = reinterpret_cast<const char*>(data_ + pos_);
        size_t len = strnlen(p, n);
        pos_ += n;
        return std::string(p, len);
    }
    std::string bytesAsString(size_t n) {
        need(n);
        std::string s(reinterpret_cast<const char*>(data_ + pos_), n);
        pos_ += n;
        return s;
    }
    std::string cString() {
        std::string s;
        while (true) {
            char c = (char)u8();
            if (c == 0) break;
            s.push_back(c);
        }
        return s;
    }
    std::string pascalString() { uint8_t n = u8(); return bytesAsString(n); }

    void bytes(uint8_t* out, size_t n) { need(n); std::memcpy(out, data_ + pos_, n); pos_ += n; }

private:
    void need(size_t n) const {
        if (pos_ + n > size_) throw std::runtime_error("unexpected end of data");
    }
    const uint8_t* data_;
    size_t size_;
    size_t pos_ = 0;
};

std::vector<uint8_t> readFile(const std::string& path);

} // namespace pb
