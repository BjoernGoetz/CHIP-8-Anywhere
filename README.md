# CHIP-8 Anywhere

A portable CHIP-8 emulator in C. One core, many platforms.

The emulator core is plain C with no operating system or platform calls.
Each platform is a thin layer around it.

## Status

Work in progress. Nothing runs yet.

## Platforms

Planned so far, with more to come:

- Terminal
- Desktop window (SDL)
- Browser (WebAssembly)
- Bare metal: boots without an operating system

## Layout

- `core/`: the portable emulator core
- `platform/`: one folder per platform

## License

MIT. See [LICENSE](LICENSE).
