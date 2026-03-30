#pragma once

#include <string>

/* macOS only: NSSavePanel must run on the main thread (OFX UI rule). Empty string if cancelled. */
std::string LSPLutGenShowSaveLUTDialog(const char* p_DefaultDir);
