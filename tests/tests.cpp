// Self-contained tests: builds tiny synthetic FAR/IFF/BHAV files in memory
// (no EA data needed) and checks the readers and VM against them.

#include "formats/bhav.h"
#include "formats/far.h"
#include "formats/iff.h"
#include "vm/vm.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace pb;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

static void le16(std::vector<uint8_t>& v, uint16_t x) { v.push_back(x & 0xFF); v.push_back(x >> 8); }
static void le32(std::vector<uint8_t>& v, uint32_t x) { for (int i = 0; i < 4; ++i) v.push_back((x >> (8 * i)) & 0xFF); }
static void be16(std::vector<uint8_t>& v, uint16_t x) { v.push_back(x >> 8); v.push_back(x & 0xFF); }
static void be32(std::vector<uint8_t>& v, uint32_t x) { for (int i = 3; i >= 0; --i) v.push_back((x >> (8 * i)) & 0xFF); }
static void str(std::vector<uint8_t>& v, const std::string& s) { v.insert(v.end(), s.begin(), s.end()); }
static void padded(std::vector<uint8_t>& v, const std::string& s, size_t n) {
    for (size_t i = 0; i < n; ++i) v.push_back(i < s.size() ? (uint8_t)s[i] : 0);
}

// BHAV 0x8002 with given instructions
static std::vector<uint8_t> makeBhav(const std::vector<BhavInstruction>& ins, uint8_t args, uint16_t locals) {
    std::vector<uint8_t> v;
    le16(v, 0x8002);
    le16(v, (uint16_t)ins.size());
    v.push_back(0);      // type
    v.push_back(args);
    le16(v, locals);
    le16(v, 0);          // version
    le16(v, 0);          // padding
    for (const auto& i : ins) {
        le16(v, i.opcode);
        v.push_back(i.trueTarget);
        v.push_back(i.falseTarget);
        v.insert(v.end(), i.operands.begin(), i.operands.end());
    }
    return v;
}

static std::vector<uint8_t> makeIff(const std::vector<std::pair<IffChunk, std::vector<uint8_t>>>& chunks) {
    std::vector<uint8_t> v;
    padded(v, "IFF FILE 2.5:TYPE FOLLOWED BY SIZE\0 JAMIE DOLLAR 2001", 60);
    be32(v, 0);
    for (const auto& [c, data] : chunks) {
        str(v, c.type);
        be32(v, (uint32_t)(76 + data.size()));
        be16(v, c.id);
        be16(v, c.flags);
        padded(v, c.label, 64);
        v.insert(v.end(), data.begin(), data.end());
    }
    return v;
}

static std::vector<uint8_t> makeFar(const std::vector<std::pair<std::string, std::vector<uint8_t>>>& files) {
    std::vector<uint8_t> v;
    str(v, "FAR!byAZ");
    le32(v, 1);
    le32(v, 0);   // manifest offset, patched below
    std::vector<uint32_t> offsets;
    for (const auto& f : files) { offsets.push_back((uint32_t)v.size()); v.insert(v.end(), f.second.begin(), f.second.end()); }
    uint32_t manifest = (uint32_t)v.size();
    std::memcpy(&v[12], &manifest, 4);   // little-endian host assumed in tests
    le32(v, (uint32_t)files.size());
    for (size_t i = 0; i < files.size(); ++i) {
        le32(v, (uint32_t)files[i].second.size());
        le32(v, (uint32_t)files[i].second.size());
        le32(v, offsets[i]);
        le32(v, (uint32_t)files[i].first.size());
        str(v, files[i].first);
    }
    return v;
}

static BhavInstruction ins(uint16_t op, uint8_t t, uint8_t f, std::array<uint8_t, 8> ops = {}) {
    BhavInstruction i; i.opcode = op; i.trueTarget = t; i.falseTarget = f; i.operands = ops; return i;
}

int main() {
    // A test primitive (#200): "add operand[0] to temp 0, return true if temp0 < operand[1]"
    // A private BHAV 0x1001 that calls 0x1002, which loops the primitive.
    auto sub = makeBhav({ ins(200, 0, kTargetTrue, {1, 5}) }, 0, 0);                  // loop until temp0 >= 5
    auto main = makeBhav({ ins(0x1002, 1, kTargetFalse), ins(200, kTargetTrue, kTargetFalse, {10, 100}) }, 0, 2);

    IffChunk c1; c1.type = "BHAV"; c1.id = 0x1001; c1.label = "main";
    IffChunk c2; c2.type = "BHAV"; c2.id = 0x1002; c2.label = "count to five";
    IffChunk c3; c3.type = "STR#"; c3.id = 128; c3.label = "names";
    auto iffBytes = makeIff({ {c1, main}, {c2, sub}, {c3, {0xFF, 0xFF, 0, 0}} });
    auto farBytes = makeFar({ {"Objects/Test.iff", iffBytes}, {"readme.txt", {'h', 'i'}} });

    // FAR
    FarArchive far;
    far.openFromMemory(farBytes);
    CHECK(far.entries().size() == 2);
    CHECK(!far.isV1b());
    const FarEntry* e = far.find("objects/test.IFF");
    CHECK(e != nullptr);
    CHECK(far.read(*far.find("readme.txt")).size() == 2);

    // IFF
    IffFile iff;
    iff.openFromMemory(far.read(*e));
    CHECK(iff.chunks().size() == 3);
    CHECK(iff.ofType("BHAV").size() == 2);
    CHECK(iff.find("BHAV", 0x1002)->label == "count to five");

    // BHAV
    Bhav bMain = parseBhav(iff.find("BHAV", 0x1001)->data, 0x1001, "main");
    Bhav bSub = parseBhav(iff.find("BHAV", 0x1002)->data, 0x1002, "count to five");
    CHECK(bMain.instructions.size() == 2);
    CHECK(bMain.locals == 2);
    CHECK(bSub.instructions[0].operands[1] == 5);
    std::string dis = disassemble(bMain);
    CHECK(dis.find("call private BHAV 0x1002") != std::string::npos);
    std::fputs(dis.c_str(), stdout);
    std::fputs(disassemble(bSub).c_str(), stdout);

    // VM
    VM vm;
    vm.addBhav(&bMain);
    vm.addBhav(&bSub);
    int calls = 0;
    vm.registerPrimitive(200, [&](VMThread& t, StackFrame&, const BhavInstruction& in) {
        ++calls;
        t.vm().temps[0] += in.operands[0];
        return t.vm().temps[0] < in.operands[1] ? ExitCode::GoToTrue : ExitCode::GoToFalse;
    });
    VMEntity obj;
    VMThread th(vm, obj);
    th.call(&bMain);
    th.tick();
    CHECK(th.finished());
    CHECK(vm.temps[0] == 15);     // 5 from the loop, +10 in main
    CHECK(calls == 6);
    CHECK(th.lastResult() == true);

    std::printf("%s (%d failures)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures);
    return failures ? 1 : 0;
}
