#include "LSPLutGeneratorDescribe.h"
#include "LSPLutGeneratorConstants.h"

namespace {
OFX::GroupParamDescriptor* addGroup(OFX::ImageEffectDescriptor& p_Desc, const char* p_Name, const char* p_Label, const char* p_Hint, bool p_Open) {
    OFX::GroupParamDescriptor* g = p_Desc.defineGroupParam(p_Name);
    g->setLabels(p_Label, p_Label, p_Label);
    g->setHint(p_Hint);
    g->setOpen(p_Open);
    return g;
}
} // namespace

void describeLutGeneratorInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum /*p_Context*/) {
    OFX::ClipDescriptor* srcClip = p_Desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    OFX::ClipDescriptor* dstClip = p_Desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    dstClip->setSupportsTiles(kSupportsTiles);

    OFX::PageParamDescriptor* page = p_Desc.definePageParam("Controls");

    OFX::GroupParamDescriptor* mainG = addGroup(p_Desc, "lutGenMainGroup", "LUT GENERATOR", "3D LUT table fill and single-input LUT analysis.", true);

    OFX::ChoiceParamDescriptor* mode = p_Desc.defineChoiceParam("operationMode");
    mode->setLabels("Operation", "Operation", "Operation mode.");
    mode->setHint("Generate: auto LUT table at the largest feasible size for the frame. Analyze: pass-through; LUT size applies to Export only.");
    mode->appendOption("Generate LUT Table");
    mode->appendOption("Analyze & export LUT");
    mode->setDefault(0);
    mode->setAnimates(false);
    mode->setParent(*mainG);

    OFX::ChoiceParamDescriptor* lutSize = p_Desc.defineChoiceParam("lutExportSize");
    lutSize->setLabels("Export LUT size", "Export LUT size", "Analyze mode only: edge length of the exported .cube (17, 32, 64, or 128).");
    lutSize->appendOption("17 × 17 × 17");
    lutSize->appendOption("32 × 32 × 32");
    lutSize->appendOption("64 × 64 × 64");
    lutSize->appendOption("128 × 128 × 128");
    lutSize->setDefault(1);
    lutSize->setAnimates(false);
    lutSize->setParent(*mainG);

    OFX::PushButtonParamDescriptor* exportPb = p_Desc.definePushButtonParam("exportLut");
    exportPb->setLabels("Export LUT", "Export LUT", "Analyze mode: save the analyzed LUT as a .cube file.");
    exportPb->setParent(*mainG);

    OFX::GroupParamDescriptor* supportG = addGroup(p_Desc, "lutGenSupportGroup", "SUPPORT", "Info and log.", false);
    {
        OFX::StringParamDescriptor* creditsParam = p_Desc.defineStringParam("lutGenCreditsLabel");
        creditsParam->setLabels("Credits", "Credits", "Credits");
        creditsParam->setDefault("Made by Loïs Plagnard");
        creditsParam->setStringType(OFX::eStringTypeLabel);
        creditsParam->setAnimates(false);
        creditsParam->setParent(*supportG);
        OFX::PushButtonParamDescriptor* helpBtn = p_Desc.definePushButtonParam("lutGenHelp");
        helpBtn->setLabels("Help", "Help", "Help");
        helpBtn->setHint("Open the project repository on GitHub.");
        helpBtn->setParent(*supportG);
        OFX::PushButtonParamDescriptor* openLogBtn = p_Desc.definePushButtonParam("lutGenOpenLog");
        openLogBtn->setLabels("Open Log", "Open Log", "Open Log");
        openLogBtn->setHint("Open LutGenerator.log (~/Library/Application Support/LSP/).");
        openLogBtn->setParent(*supportG);
    }

    page->addChild(*mainG);
    page->addChild(*supportG);
}
