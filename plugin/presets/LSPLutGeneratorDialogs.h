#pragma once

#include <string>

/** macOS NSSavePanel for .cube; runs on main queue. Empty if cancelled. */
std::string LSPLutGenShowSaveLUTDialog(const char* p_DefaultDir);
