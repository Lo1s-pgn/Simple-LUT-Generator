#ifndef LSP_LUT_GENERATOR_CONSTANTS_H
#define LSP_LUT_GENERATOR_CONSTANTS_H

#include "version_gen.h"

/* Identity and OFX capability flags (see Makefile-generated version_gen.h). */
/* Effect list / node name in the host. Grouping: OFX / separated; Resolve shows LSP → Color. */
#define kPluginName "LSP - Simple LUT Generator " PLUGIN_VERSION_STR
#define kPluginGrouping "LSP/Color"
#define kPluginDescription "LSP - Simple LUT Generator — 3D LUT table and single-input LUT analysis (OFX)."
#define kPluginIdentifier PLUGIN_OFX_IDENTIFIER
#define kPluginVersionMajor PLUGIN_VERSION_MAJOR
#define kPluginVersionMinor PLUGIN_VERSION_MINOR
#define kSupportsTiles false
#define kSupportsMultiResolution false
#define kSupportsMultipleClipPARs false

#define kOperationModeGenerate 0
#define kOperationModeAnalyze 1

/* Help / bug-report buttons (must match public repo). */
#define kLutGenRepoUrl "https://github.com/Lo1s-pgn/Simple-LUT-Generator"
#define kLutGenIssuesUrl "https://github.com/Lo1s-pgn/Simple-LUT-Generator/issues"

#endif
