// simtool — PC command-line tool for inspecting The Sims 1 game files.
//
//   simtool far-list    <file.far>
//   simtool far-extract <file.far> <entry name> <out file>
//   simtool iff-list    <file.iff>
//   simtool bhav-dump   <file.iff> [bhav id]
//   simtool far-iff-list <file.far> <entry.iff>
//   simtool far-bhav-dump <file.far> <entry.iff> [bhav id]

#include "formats/bhav.h"
#include "formats/far.h"
#include "formats/iff.h"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

using namespace pb;

static int usage() {
    std::puts(
        "simtool - The Sims 1 file inspector\n"
        "  simtool far-list      <file.far>\n"
        "  simtool far-extract   <file.far> <entry> <out>\n"
        "  simtool iff-list      <file.iff>\n"
        "  simtool bhav-dump     <file.iff> [bhav id]\n"
        "  simtool far-iff-list  <file.far> <entry.iff>\n"
        "  simtool far-bhav-dump <file.far> <entry.iff> [bhav id]");
    return 1;
}

static void listIff(const IffFile& iff) {
    std::printf("%s\n%zu chunks\n", iff.headerText().c_str(), iff.chunks().size());
    for (const auto& c : iff.chunks()) {
        std::printf("  %-4s #%-6u %7zu bytes  \"%s\"\n", c.type.c_str(), c.id, c.data.size(), c.label.c_str());
    }
}

static void dumpBhavs(const IffFile& iff, int onlyId) {
    for (const IffChunk* c : iff.ofType("BHAV")) {
        if (onlyId >= 0 && c->id != onlyId) continue;
        try {
            Bhav b = parseBhav(c->data, c->id, c->label);
            std::fputs(disassemble(b).c_str(), stdout);
            std::putchar('\n');
        } catch (const std::exception& e) {
            std::printf("BHAV #%u \"%s\": parse error: %s\n\n", c->id, c->label.c_str(), e.what());
        }
    }
}

static int parseId(const char* s) { return (int)std::strtol(s, nullptr, 0); }

int main(int argc, char** argv) {
    if (argc < 3) return usage();
    std::string cmd = argv[1];
    try {
        if (cmd == "far-list") {
            FarArchive far;
            far.open(argv[2]);
            std::printf("%zu entries (FAR v1%s)\n", far.entries().size(), far.isV1b() ? "b" : "a");
            for (const auto& e : far.entries())
                std::printf("  %9u  %s\n", e.size, e.name.c_str());
        } else if (cmd == "far-extract" && argc >= 5) {
            FarArchive far;
            far.open(argv[2]);
            const FarEntry* e = far.find(argv[3]);
            if (!e) { std::fprintf(stderr, "entry not found: %s\n", argv[3]); return 2; }
            auto bytes = far.read(*e);
            FILE* f = std::fopen(argv[4], "wb");
            if (!f) { std::fprintf(stderr, "cannot write %s\n", argv[4]); return 2; }
            std::fwrite(bytes.data(), 1, bytes.size(), f);
            std::fclose(f);
            std::printf("wrote %zu bytes to %s\n", bytes.size(), argv[4]);
        } else if (cmd == "iff-list") {
            IffFile iff;
            iff.open(argv[2]);
            listIff(iff);
        } else if (cmd == "bhav-dump") {
            IffFile iff;
            iff.open(argv[2]);
            dumpBhavs(iff, argc >= 4 ? parseId(argv[3]) : -1);
        } else if ((cmd == "far-iff-list" || cmd == "far-bhav-dump") && argc >= 4) {
            FarArchive far;
            far.open(argv[2]);
            const FarEntry* e = far.find(argv[3]);
            if (!e) { std::fprintf(stderr, "entry not found: %s\n", argv[3]); return 2; }
            IffFile iff;
            iff.openFromMemory(far.read(*e));
            if (cmd == "far-iff-list") listIff(iff);
            else dumpBhavs(iff, argc >= 5 ? parseId(argv[4]) : -1);
        } else {
            return usage();
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 2;
    }
    return 0;
}
