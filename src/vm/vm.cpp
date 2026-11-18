#include "vm.h"

#include <stdexcept>

namespace pb {

void VMThread::call(const Bhav* routine, std::vector<int16_t> args) {
    if (!routine) throw std::runtime_error("call to missing BHAV");
    StackFrame f;
    f.routine = routine;
    f.args = std::move(args);
    f.args.resize(std::max<size_t>(f.args.size(), 4), 0);
    f.locals.assign(routine->locals, 0);
    stack_.push_back(std::move(f));
}

void VMThread::returnFromFrame(bool result) {
    stack_.pop_back();
    lastResult_ = result;
    if (stack_.empty()) return;
    // The caller's current instruction was the subroutine call: branch on its result.
    StackFrame& caller = stack_.back();
    const BhavInstruction& in = caller.routine->instructions[caller.pc];
    uint8_t target = result ? in.trueTarget : in.falseTarget;
    if (target == kTargetTrue || target == kTargetFalse || target == kTargetError)
        returnFromFrame(target == kTargetTrue);
    else
        caller.pc = target;
}

void VMThread::tick(int budget) {
    while (!stack_.empty() && budget-- > 0) {
        StackFrame& f = stack_.back();
        if (f.pc >= f.routine->instructions.size())
            throw std::runtime_error("pc out of range in BHAV " + std::to_string(f.routine->id));
        const BhavInstruction& in = f.routine->instructions[f.pc];

        if (in.opcode >= 256) {
            const Bhav* sub = vm_.findBhav(in.opcode);
            std::vector<int16_t> args;
            for (int i = 0; i < 4; ++i)
                args.push_back((int16_t)(in.operands[i * 2] | (in.operands[i * 2 + 1] << 8)));
            call(sub, std::move(args));
            continue;
        }

        const Primitive* p = vm_.primitive(in.opcode);
        if (!p) throw std::runtime_error(std::string("unimplemented primitive: ") + describeOpcode(in.opcode));
        ExitCode r = (*p)(*this, f, in);
        if (r == ExitCode::Yield) return;
        if (r == ExitCode::Continue) continue;
        if (r == ExitCode::Error) throw std::runtime_error("primitive error");

        uint8_t target = (r == ExitCode::GoToTrue) ? in.trueTarget : in.falseTarget;
        if (target == kTargetTrue || target == kTargetFalse || target == kTargetError)
            returnFromFrame(target == kTargetTrue);
        else
            f.pc = target;
    }
}

} // namespace pb
