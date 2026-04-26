#pragma once

#include <string>

/* macOS: panels must run on the main thread (OFX). Empty if cancelled. */
std::string LSPLutGenShowChooseFolderDialog(const char* p_DefaultDir);
