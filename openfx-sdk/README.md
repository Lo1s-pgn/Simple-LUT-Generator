# Vendored OpenFX 1.5.1 + Support (minimal)

This tree is the subset required to build **LSP - Simple LUT Generator** (C API headers, Support headers, `ofxsSupportPrivate.h` beside the Support `.cpp` files, and the eight `ofxs*.cpp` translation units compiled by the project `Makefile`).

- **include/** — OFX C API (`ofx*.h`), from [AcademySoftwareFoundation/openfx](https://github.com/AcademySoftwareFoundation/openfx) tag **OFX_Release_1.5.1**.
- **Support/include/** — OFX C++ Support layer (`ofxs*.h`).
- **Support/Library/** — the `.cpp` files listed in the Simple LUT Generator **Makefile** plus `ofxsSupportPrivate.h` (upstream location).

Override path with **`OFX_SDK_PATH`** if you want to point at a different SDK checkout instead of this folder.
