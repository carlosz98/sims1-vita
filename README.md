# PlumbBob Vita

An open-source engine reimplementation of **The Sims 1** for the **PS Vita**, written in C++.

> **This project contains no EA game data.** To play, you need your own copy of
> *The Sims* (Complete Collection or Legacy Edition) and must copy its game files
> to your Vita. This is an unofficial fan project, not affiliated with or endorsed by
> Electronic Arts or Maxis. "The Sims" is a trademark of Electronic Arts Inc.

## How it works

The Sims 1 keeps almost all of its game logic inside its data files. Every object
(fridge, shower, the Sims themselves) carries its own scripts written in
**SimAntics**, a bytecode language stored in `BHAV` chunks inside `.iff` files,
which are packed into `.far` archives. PlumbBob is an engine that reads those
files and runs that bytecode, so it reproduces the game without shipping any of it.

Reference: the [FreeSO](https://github.com/riperiperi/FreeSO) and
[Simitone](https://github.com/riperiperi/Simitone) projects (C#), which
reimplement the same engine for PC and mobile.

## Status

| Stage | What | Status |
|---|---|---|
| 1 | FAR / IFF readers, BHAV disassembler, VM core loop | ✅ done (tested on synthetic files) |
| 2 | Test against real game files, dump objects | ⏳ next |
| 3 | Sprites (SPR2) → render a lot (floors, walls, objects) | |
| 4 | Sims and animations | |
| 5 | SimAntics primitives (Expression, Idle, Animate, Routing, …) | |
| 6 | UI, controls, memory tuning for the Vita (512 MB) | |

## Building (PC tools)

```bash
cmake -S . -B build
cmake --build build
./build/pbtests                      # self-tests, no game files needed
./build/simtool far-list  "path/to/GameData/Objects/Objects.far"
./build/simtool far-iff-list  Objects.far <entry.iff>
./build/simtool far-bhav-dump Objects.far <entry.iff>
```

## Building for the Vita

Requires [VitaSDK](https://vitasdk.org).

```bash
cmake -S . -B build-vita -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake
cmake --build build-vita
```

Install `PlumbBobVita.vpk` with VitaShell, then copy the `GameData` folder from
your own The Sims install to `ux0:data/plumbbob/GameData/`.

## License

GPL-3.0
