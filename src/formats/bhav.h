#pragma once
// BHAV = one SimAntics function (a flowchart of instructions).
// Each instruction: uint16 opcode, uint8 true target, uint8 false target, 8 operand bytes.
// Targets 0..252 jump to that instruction index; special values:
//   253 = error, 254 = return true, 255 = return false.
// Opcodes: 0..255 primitives, 0x100..0x0FFF global BHAVs,
//          0x1000..0x1FFF private (same object), 0x2000+ semi-global.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace pb {

struct BhavInstruction {
    uint16_t opcode = 0;
    uint8_t trueTarget = 0;
    uint8_t falseTarget = 0;
    std::array<uint8_t, 8> operands{};
};

struct Bhav {
    uint16_t id = 0;
    std::string label;
    uint16_t signature = 0;   // 0x8000..0x8003 (format revision)
    uint8_t type = 0;
    uint8_t args = 0;
    uint16_t locals = 0;
    uint16_t version = 0;
    std::vector<BhavInstruction> instructions;
};

constexpr uint8_t kTargetError = 253;
constexpr uint8_t kTargetTrue = 254;
constexpr uint8_t kTargetFalse = 255;

Bhav parseBhav(const std::vector<uint8_t>& data, uint16_t id = 0, const std::string& label = "");

const char* primitiveName(uint16_t opcode);   // best-effort names, verify against FreeSO
std::string describeOpcode(uint16_t opcode);
std::string disassemble(const Bhav& b);

} // namespace pb
