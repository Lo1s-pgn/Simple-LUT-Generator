#ifndef LSP_LUT_GENERATOR_CONSTANTS_H
#define LSP_LUT_GENERATOR_CONSTANTS_H

#include "version_gen.h"

#define kPluginName "LSP LUT Generator " PLUGIN_VERSION_STR
#define kPluginGrouping "LSP - Color"
#define kPluginDescription "LSP LUT Generator — 3D LUT table and single-input LUT analysis (OFX)."
#define kPluginIdentifier PLUGIN_OFX_IDENTIFIER
#define kPluginVersionMajor PLUGIN_VERSION_MAJOR
#define kPluginVersionMinor PLUGIN_VERSION_MINOR
#define kSupportsTiles false
#define kSupportsMultiResolution false
#define kSupportsMultipleClipPARs false

#define kOperationModeGenerate 0
#define kOperationModeAnalyze 1

#endif
