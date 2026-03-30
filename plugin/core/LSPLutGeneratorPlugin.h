#ifndef LSP_LUT_GENERATOR_PLUGIN_H
#define LSP_LUT_GENERATOR_PLUGIN_H

#include "ofxsImageEffect.h"

// Registers the effect with OFX; load/unload are no-ops (standard Support pattern).
class LSPLutGeneratorPluginFactory : public OFX::PluginFactoryHelper<LSPLutGeneratorPluginFactory> {
public:
    LSPLutGeneratorPluginFactory();
    void load() override {}
    void unload() override {}
    void describe(OFX::ImageEffectDescriptor& p_Desc) override;
    void describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum p_Context) override;
    OFX::ImageEffect* createInstance(OfxImageEffectHandle p_Handle, OFX::ContextEnum p_Context) override;
};

#endif
