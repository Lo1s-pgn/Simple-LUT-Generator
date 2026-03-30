# LSP LUT Generator OFX — macOS (CPU-only render)
# VERSION (first line) = MAJ.MIN.PATCH (e.g. 1.0.0) → PLUGIN_VERSION_STR and bundle LSP_LutGenerator_<version>.ofx.bundle
# Intermediates (.o, linked .ofx, Info.plist) → build/ ; installable .ofx.bundle → release/

BUILDDIR := build
DISTDIR := release
VERSION_FILE := VERSION
VERSION_GEN := plugin/version_gen.h
VERSION_LINE := $(shell head -n1 $(VERSION_FILE) 2>/dev/null | tr -d ' \r\n')
VERSION_MAJ := $(shell echo "$(VERSION_LINE)" | cut -d. -f1)
VERSION_MIN := $(shell echo "$(VERSION_LINE)" | cut -d. -f2)
VERSION_PATCH := $(shell echo "$(VERSION_LINE)" | cut -d. -f3)
PLUGIN_DISPLAY := $(VERSION_MAJ).$(VERSION_MIN).$(VERSION_PATCH)
OFX_BUNDLE_STEM := LSP_LutGenerator_$(PLUGIN_DISPLAY)
OFX_BUNDLE := $(DISTDIR)/$(OFX_BUNDLE_STEM).ofx.bundle
OFX_BINARY := $(BUILDDIR)/$(OFX_BUNDLE_STEM).ofx
BUNDLE_DIR = $(OFX_BUNDLE)/Contents/MacOS/
RESOURCES_DIR = $(OFX_BUNDLE)/Contents/Resources

# Vendored minimal SDK in-repo; override with OFX_SDK_PATH=/path/to/sdk if needed.
OFX_SDK_PATH ?= openfx-sdk
ARCH_FLAGS = -arch arm64 -arch x86_64
CXXFLAGS = -std=c++20 -O2 -Wno-dynamic-exception-spec -fvisibility=hidden -Iplugin -Iplugin/core -Iplugin/presets -I"$(OFX_SDK_PATH)/include" -I"$(OFX_SDK_PATH)/Support/include" -I"$(OFX_SDK_PATH)/Support/Library" $(ARCH_FLAGS)
LDFLAGS = -bundle -fvisibility=hidden -framework Foundation -framework AppKit -Wl,-rpath,@loader_path $(ARCH_FLAGS)

PLUGIN_ICON_NAME := com.LSP.LutGenerator.$(PLUGIN_DISPLAY).png

PLUGIN_OBJ = $(BUILDDIR)/LSPLutGeneratorPlugin.o $(BUILDDIR)/LSPLutGeneratorDescribe.o $(BUILDDIR)/LSPLutGeneratorProcessor.o $(BUILDDIR)/LSPLutGeneratorPattern.o $(BUILDDIR)/LSPLutGeneratorCube.o $(BUILDDIR)/LSPLutGeneratorDialogs.o
SUPPORT_OBJ = $(BUILDDIR)/ofxsCore.o $(BUILDDIR)/ofxsImageEffect.o $(BUILDDIR)/ofxsInteract.o $(BUILDDIR)/ofxsLog.o \
	$(BUILDDIR)/ofxsMultiThread.o $(BUILDDIR)/ofxsParams.o $(BUILDDIR)/ofxsProperty.o $(BUILDDIR)/ofxsPropertyValidation.o

all: $(OFX_BUNDLE) resources

resources: $(OFX_BUNDLE)
	@if [ -f ICON.png ]; then cp ICON.png $(RESOURCES_DIR)/$(PLUGIN_ICON_NAME); echo "Icon: $(RESOURCES_DIR)/$(PLUGIN_ICON_NAME)"; else echo "No ICON.png — effect icon skipped."; fi

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(OFX_BINARY): $(PLUGIN_OBJ) $(SUPPORT_OBJ)
	$(CXX) $^ -o $@ $(LDFLAGS)
	strip -x $@

$(OFX_BUNDLE): $(OFX_BINARY) $(BUILDDIR)/Info.plist $(VERSION_GEN)
	@mkdir -p $(DISTDIR)
	rm -rf $(OFX_BUNDLE)
	mkdir -p $(BUNDLE_DIR)
	mkdir -p $(RESOURCES_DIR)
	cp $(OFX_BINARY) $(BUNDLE_DIR)$(OFX_BUNDLE_STEM).ofx
	cp $(BUILDDIR)/Info.plist $(OFX_BUNDLE)/Contents/Info.plist
	@exe=$$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$(OFX_BUNDLE)/Contents/Info.plist" 2>/dev/null); \
	if [ -z "$$exe" ]; then echo "ERROR: Could not read CFBundleExecutable from Info.plist"; exit 1; fi; \
	if [ ! -f "$(OFX_BUNDLE)/Contents/MacOS/$$exe" ]; then \
	  echo "ERROR: CFBundleExecutable ($$exe) must match the Mach-O basename in Contents/MacOS/."; \
	  exit 1; \
	fi; \
	bundle_base=$$(basename "$(OFX_BUNDLE)" .bundle); \
	if [ "$$bundle_base" != "$$exe" ]; then \
	  echo "ERROR: Bundle folder (minus .bundle) should equal executable name: expected $$exe, got $$bundle_base"; exit 1; \
	fi; \
	echo "Bundle ready: $(OFX_BUNDLE) (executable $$exe OK)"

$(VERSION_GEN) $(BUILDDIR)/Info.plist: Info.plist.in $(VERSION_FILE) | $(BUILDDIR)
	@v=$$(head -n1 $(VERSION_FILE) | tr -d ' \r\n'); \
	maj=$$(echo "$$v" | cut -d. -f1); min=$$(echo "$$v" | cut -d. -f2); pat=$$(echo "$$v" | cut -d. -f3); \
	display="$$maj.$$min.$$pat"; \
	numeric="$$maj.$$min.$$pat"; \
	ofxid="com.LSP.LutGenerator.$$display"; \
	{ echo "/* Generated from VERSION (first line) = MAJ.MIN.PATCH — do not edit */"; \
	  echo "#ifndef VERSION_GEN_H"; \
	  echo "#define VERSION_GEN_H"; \
	  echo "#define PLUGIN_VERSION_STR \"$$display\""; \
	  echo "#define PLUGIN_VERSION_MAJOR $$maj"; \
	  echo "#define PLUGIN_VERSION_MINOR $$min"; \
	  echo "#define PLUGIN_VERSION_PATCH $$pat"; \
	  echo "#define PLUGIN_CHANGELOG_TRIPLET \"$$v\""; \
	  echo "#define PLUGIN_OFX_IDENTIFIER \"$$ofxid\""; \
	  echo "#endif"; \
	} > $(VERSION_GEN); \
	stem="LSP_LutGenerator_$$display"; \
	exe="$$stem.ofx"; \
	sed -e "s|@PLUGIN_NUMERIC_VERSION@|$$numeric|g" \
	    -e "s|@OFX_EXECUTABLE_NAME@|$$exe|g" Info.plist.in > $(BUILDDIR)/Info.plist; \
	echo "Generated $(VERSION_GEN) + $(BUILDDIR)/Info.plist ($$display, $$exe)"

$(BUILDDIR)/LSPLutGeneratorPlugin.o: plugin/core/LSPLutGeneratorPlugin.cpp $(VERSION_GEN) plugin/core/LSPLutGeneratorPlugin.h plugin/core/LSPLutGeneratorDescribe.h plugin/core/LSPLutGeneratorProcessor.h plugin/core/LSPLutGeneratorConstants.h plugin/core/LSPLutGeneratorCube.h plugin/core/LSPLutGeneratorLog.h plugin/presets/LSPLutGeneratorDialogs.h plugin/core/LSPLutGeneratorPattern.h | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/LSPLutGeneratorDescribe.o: plugin/core/LSPLutGeneratorDescribe.cpp plugin/core/LSPLutGeneratorDescribe.h plugin/core/LSPLutGeneratorConstants.h | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/LSPLutGeneratorProcessor.o: plugin/core/LSPLutGeneratorProcessor.cpp plugin/core/LSPLutGeneratorProcessor.h plugin/core/LSPLutGeneratorPattern.h plugin/core/LSPLutGeneratorConstants.h | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/LSPLutGeneratorPattern.o: plugin/core/LSPLutGeneratorPattern.cpp plugin/core/LSPLutGeneratorPattern.h | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/LSPLutGeneratorCube.o: plugin/core/LSPLutGeneratorCube.cpp plugin/core/LSPLutGeneratorCube.h | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/LSPLutGeneratorDialogs.o: plugin/presets/LSPLutGeneratorDialogs.mm plugin/presets/LSPLutGeneratorDialogs.h | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/ofxsCore.o: $(OFX_SDK_PATH)/Support/Library/ofxsCore.cpp | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)
$(BUILDDIR)/ofxsImageEffect.o: $(OFX_SDK_PATH)/Support/Library/ofxsImageEffect.cpp | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)
$(BUILDDIR)/ofxsInteract.o: $(OFX_SDK_PATH)/Support/Library/ofxsInteract.cpp | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)
$(BUILDDIR)/ofxsLog.o: $(OFX_SDK_PATH)/Support/Library/ofxsLog.cpp | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)
$(BUILDDIR)/ofxsMultiThread.o: $(OFX_SDK_PATH)/Support/Library/ofxsMultiThread.cpp | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)
$(BUILDDIR)/ofxsParams.o: $(OFX_SDK_PATH)/Support/Library/ofxsParams.cpp | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)
$(BUILDDIR)/ofxsProperty.o: $(OFX_SDK_PATH)/Support/Library/ofxsProperty.cpp | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)
$(BUILDDIR)/ofxsPropertyValidation.o: $(OFX_SDK_PATH)/Support/Library/ofxsPropertyValidation.cpp | $(BUILDDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

clean:
	@chmod -R u+w $(BUILDDIR) 2>/dev/null || true
	rm -rf $(BUILDDIR)
	@if [ -d "$(DISTDIR)" ]; then \
	  chmod -R u+w "$(DISTDIR)" 2>/dev/null || true; \
	  rm -rf "$(DISTDIR)"/LSP_LutGenerator_*.ofx.bundle 2>/dev/null || true; \
	  rm -f "$(DISTDIR)"/*.zip 2>/dev/null || true; \
	fi

# Resolve OFX cache lives in the installing user's home. Under `sudo`, HOME is root — use SUDO_USER
# (set by sudo) and Directory Service to find that user's NFSHomeDirectory; not a hardcoded account.
ifeq ($(SUDO_USER),)
RESOLVE_OFX_CACHE_HOME := $(HOME)
else
RESOLVE_OFX_CACHE_HOME := $(shell dscl . -read /Users/$(SUDO_USER) NFSHomeDirectory 2>/dev/null | sed 's/^[^:]*: *//')
ifeq ($(RESOLVE_OFX_CACHE_HOME),)
RESOLVE_OFX_CACHE_HOME := $(HOME)
endif
endif
RESOLVE_OFX_CACHE_V2 := $(RESOLVE_OFX_CACHE_HOME)/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml
RESOLVE_OFX_CACHE_V1 := $(RESOLVE_OFX_CACHE_HOME)/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCache.xml

install: all
	cp -R $(OFX_BUNDLE) /Library/OFX/Plugins
	@if [ "$(SKIP_RESOLVE_OFX_CACHE_PURGE)" != 1 ]; then \
	  $(MAKE) purge; \
	  echo "Quit Resolve if it was open, then restart."; \
	else \
	  echo "Skipped OFX cache purge (SKIP_RESOLVE_OFX_CACHE_PURGE=1)."; \
	fi
	@echo "Installed $(OFX_BUNDLE_STEM).ofx.bundle — restart your OFX host."

purge_resolve_ofx_cache: purge

purge:
	@rm -f "$(RESOLVE_OFX_CACHE_V2)" "$(RESOLVE_OFX_CACHE_V1)" 2>/dev/null || true
	@echo "Purged Resolve OFX cache for $(RESOLVE_OFX_CACHE_HOME):"
	@echo "  $(RESOLVE_OFX_CACHE_V2)"
	@echo "  $(RESOLVE_OFX_CACHE_V1)"

.PHONY: all clean install resources purge_resolve_ofx_cache purge
