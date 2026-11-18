#include "bhav.h"

#include "binary_reader.h"

#include <cstdio>
#include <sstream>
#include <stdexcept>

namespace pb {

Bhav parseBhav(const std::vector<uint8_t>& data, uint16_t id, const std::string& label) {
    Bhav b;
    b.id = id;
    b.label = label;
    BinaryReader r(data);
    b.signature = r.u16le();
    uint32_t count = 0;
    switch (b.signature) {
        case 0x8000:
        case 0x8001:
            count = r.u16le();
            r.skip(8);
            break;
        case 0x8002:
            count = r.u16le();
            b.type = r.u8();
            b.args = r.u8();
            b.locals = r.u16le();
            b.version = r.u16le();
            r.skip(2);
            break;
        case 0x8003:
            b.type = r.u8();
            b.args = r.u8();
            b.locals = r.u8();
            r.skip(2);
            b.version = r.u16le();
            count = r.u32le();
            break;
        default: {
            char buf[64];
            std::snprintf(buf, sizeof buf, "unknown BHAV signature 0x%04X", b.signature);
            throw std::runtime_error(buf);
        }
    }
    if (count > 253) throw std::runtime_error("BHAV has too many instructions");
    b.instructions.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        BhavInstruction in;
        in.opcode = r.u16le();
        in.trueTarget = r.u8();
        in.falseTarget = r.u8();
        r.bytes(in.operands.data(), 8);
        b.instructions.push_back(in);
    }
    return b;
}

// Primitive names as used by the community / FreeSO. Treat as a starting
// point: confirm each one against FreeSO's VMContext before implementing it.
static const char* kPrimitiveNames[] = {
    "Sleep",                        // 0
    "Generic Sims Call",            // 1
    "Expression",                   // 2
    "Find Best Interaction",        // 3
    "Grab",                         // 4
    "Drop",                         // 5
    "Change Suit/Accessory",        // 6
    "Refresh",                      // 7
    "Random Number",                // 8
    "Burn",                         // 9
    "Tutorial",                     // 10
    "Get Distance To",              // 11
    "Get Direction To",             // 12
    "Push Interaction",             // 13
    "Find Best Object For Function",// 14
    "Breakpoint",                   // 15
    "Find Location For",            // 16
    "Idle For Input",               // 17
    "Remove Object Instance",       // 18
    "Make New Character",           // 19
    "Run Functional Tree",          // 20
    "Show String",                  // 21
    "Look Towards",                 // 22
    "Play Sound Event",             // 23
    "Old Relationship",             // 24
    "Alter Budget",                 // 25
    "Relationship",                 // 26
    "Go To Relative Position",      // 27
    "Run Tree By Name",             // 28
    "Set Motive Change",            // 29
    "Gosub Found Action",           // 30
    "Set To Next",                  // 31
    "Test Object Type",             // 32
    "Find 5 Worst Motives",         // 33
    "UI Effect",                    // 34
    "Special Effect",               // 35
    "Dialog (Private)",             // 36
    "Test Sim Interacting With",    // 37
    "Dialog (Global)",              // 38
    "Dialog (Semi-Global)",         // 39
    "Online Jobs Call",             // 40
    "Set Balloon/Headline",         // 41
    "Create Object Instance",       // 42
    "Drop Onto",                    // 43
    "Animate Sim",                  // 44
    "Go To Routing Slot",           // 45
    "Snapshot",                     // 46
    "Retrieve Item From Slot",      // 47
    "Stop All Sounds",              // 48
    "Notify Stack Object Out Of Idle", // 49
    "Change Action String",         // 50
};

const char* primitiveName(uint16_t opcode) {
    constexpr size_t n = sizeof(kPrimitiveNames) / sizeof(kPrimitiveNames[0]);
    if (opcode < n) return kPrimitiveNames[opcode];
    return nullptr;
}

std::string describeOpcode(uint16_t opcode) {
    char buf[96];
    if (opcode < 256) {
        const char* name = primitiveName(opcode);
        if (name) std::snprintf(buf, sizeof buf, "%s", name);
        else std::snprintf(buf, sizeof buf, "Primitive #%u", opcode);
    } else if (opcode < 0x1000) {
        std::snprintf(buf, sizeof buf, "call global BHAV 0x%04X", opcode);
    } else if (opcode < 0x2000) {
        std::snprintf(buf, sizeof buf, "call private BHAV 0x%04X", opcode);
    } else {
        std::snprintf(buf, sizeof buf, "call semi-global BHAV 0x%04X", opcode);
    }
    return buf;
}

static std::string targetStr(uint8_t t) {
    switch (t) {
        case kTargetTrue: return "return TRUE";
        case kTargetFalse: return "return FALSE";
        case kTargetError: return "ERROR";
        default: return "goto " + std::to_string(t);
    }
}

std::string disassemble(const Bhav& b) {
    std::ostringstream o;
    char head[160];
    std::snprintf(head, sizeof head, "BHAV 0x%04X \"%s\"  (sig 0x%04X, args %u, locals %u, %zu instructions)\n",
                  b.id, b.label.c_str(), b.signature, b.args, b.locals, b.instructions.size());
    o << head;
    for (size_t i = 0; i < b.instructions.size(); ++i) {
        const auto& in = b.instructions[i];
        char line[256];
        std::snprintf(line, sizeof line,
                      "  %3zu: %-34s ops[%02X %02X %02X %02X %02X %02X %02X %02X]  T:%-13s F:%s\n",
                      i, describeOpcode(in.opcode).c_str(),
                      in.operands[0], in.operands[1], in.operands[2], in.operands[3],
                      in.operands[4], in.operands[5], in.operands[6], in.operands[7],
                      targetStr(in.trueTarget).c_str(), targetStr(in.falseTarget).c_str());
        o << line;
    }
    return o.str();
}

} // namespace pb
