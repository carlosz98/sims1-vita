#pragma once
// SimAntics VM — skeleton. Primitives get added one by one in later stages.

#include "formats/bhav.h"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace pb {

class VMThread;
struct VMEntity;

enum class ExitCode { GoToTrue, GoToFalse, Continue, Yield, Error };

struct StackFrame {
    const Bhav* routine = nullptr;
    uint8_t pc = 0;
    std::vector<int16_t> args;
    std::vector<int16_t> locals;
    VMEntity* stackObject = nullptr;
};

struct VMEntity {
    uint16_t objectId = 0;
    std::vector<int16_t> attributes;
    std::vector<int16_t> objectData;
    std::vector<int16_t> motives;   // Sims only
};

using Primitive = std::function<ExitCode(VMThread&, StackFrame&, const BhavInstruction&)>;

class VM {
public:
    void registerPrimitive(uint16_t opcode, Primitive p) { primitives_[opcode] = std::move(p); }
    const Primitive* primitive(uint16_t opcode) const {
        auto it = primitives_.find(opcode);
        return it == primitives_.end() ? nullptr : &it->second;
    }
    void addBhav(const Bhav* b) { bhavs_[b->id] = b; }
    const Bhav* findBhav(uint16_t id) const {
        auto it = bhavs_.find(id);
        return it == bhavs_.end() ? nullptr : it->second;
    }
    std::vector<int16_t> globals = std::vector<int16_t>(64, 0);
    std::vector<int16_t> temps = std::vector<int16_t>(20, 0);

private:
    std::unordered_map<uint16_t, Primitive> primitives_;
    std::unordered_map<uint16_t, const Bhav*> bhavs_;   // TODO: split global/private/semi-global lookup
};

class VMThread {
public:
    VMThread(VM& vm, VMEntity& self) : vm_(vm), self_(self) {}
    void call(const Bhav* routine, std::vector<int16_t> args = {});
    // Runs until the thread yields, finishes, or hits the instruction budget.
    void tick(int budget = 500);
    bool finished() const { return stack_.empty(); }
    bool lastResult() const { return lastResult_; }
    VM& vm() { return vm_; }
    VMEntity& self() { return self_; }

private:
    void returnFromFrame(bool result);
    VM& vm_;
    VMEntity& self_;
    std::vector<StackFrame> stack_;
    bool lastResult_ = false;
};

} // namespace pb
