# Vendored OpenFX 1.4 + Support (minimal)

This tree is the subset required to build **LSP LUT Generator** (C API headers, Support headers, and the eight `ofxs*.cpp` translation units compiled by the project `Makefile`).

- **OpenFX-1.4/include/** — OFX C API (`ofx*.h`).
- **Support/include/** — OFX C++ Support layer (`ofxs*.h`).
- **Support/Library/** — only the `.cpp` files listed in the LUT Generator **Makefile** (not the full upstream Support library).

Source: OpenFX / Open Effects Association SDK; original license and copyright notices are in the individual files.

Override path with **`OFX_SDK_PATH`** if you want to point at a different SDK checkout instead of this folder.
