// PS Vita entry point — stage 1: checks for the player's game files and
// lists what it finds. Rendering (vitaGL/SDL2) comes in a later stage.

#include "formats/far.h"

#include <psp2/ctrl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>

#include <cstdio>
#include <string>

static const char* kDataDir = "ux0:data/plumbbob/";

static bool exists(const std::string& p) {
    SceIoStat st;
    return sceIoGetstat(p.c_str(), &st) >= 0;
}

int main() {
    FILE* log = std::fopen("ux0:data/plumbbob/log.txt", "w");
    auto say = [&](const std::string& s) { if (log) { std::fputs((s + "\n").c_str(), log); std::fflush(log); } };

    std::string objects = std::string(kDataDir) + "GameData/Objects/Objects.far";
    if (!exists(objects)) {
        say("Game files not found.");
        say("Copy the GameData folder from your own The Sims install to ux0:data/plumbbob/");
    } else {
        try {
            pb::FarArchive far;
            far.open(objects);
            say("Objects.far: " + std::to_string(far.entries().size()) + " entries");
        } catch (const std::exception& e) {
            say(std::string("error: ") + e.what());
        }
    }
    if (log) std::fclose(log);
    sceKernelExitProcess(0);
    return 0;
}
