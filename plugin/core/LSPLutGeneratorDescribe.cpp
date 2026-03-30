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
static void describeLutGeneratorSupportParams(OFX::ImageEffectDescriptor& p_Desc, OFX::GroupParamDescriptor& p_Group) {
    OFX::StringParamDescriptor* creditsParam = p_Desc.defineStringParam("lutGenCreditsLabel");
    creditsParam->setLabels("Credits", "Credits", "Credits");
    creditsParam->setDefault("Made by Loïs Plagnard");
    creditsParam->setStringType(OFX::eStringTypeLabel);
    creditsParam->setAnimates(false);
    creditsParam->setParent(p_Group);

    OFX::PushButtonParamDescriptor* reportBugBtn = p_Desc.definePushButtonParam("lutGenReportBug");
    reportBugBtn->setLabels("Report Bug", "Report Bug", "Report Bug");
    reportBugBtn->setHint("Report a bug.");
    reportBugBtn->setParent(p_Group);

    OFX::PushButtonParamDescriptor* helpBtn = p_Desc.definePushButtonParam("lutGenHelp");
    helpBtn->setLabels("Help", "Help", "Help");
    helpBtn->setHint("Help and documentation.");
    helpBtn->setParent(p_Group);

    OFX::PushButtonParamDescriptor* openLogBtn = p_Desc.definePushButtonParam("lutGenOpenLog");
    openLogBtn->setLabels("Open Log", "Open Log", "Open Log");
    openLogBtn->setHint("Open LutGenerator.log (~/Library/Application Support/LSP/).");
    openLogBtn->setParent(p_Group);
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
    mode->setHint("Generate LUT Table: max feasible n^3 (16-128) for this frame. Analyze: export size divides max; .cube may downsample. Refresh after timeline resize.");
    mode->appendOption("Generate LUT Table");
    mode->appendOption("Analyze & export LUT");
    mode->setDefault(0);
    mode->setAnimates(false);
    mode->setParent(*mainG);

    OFX::StringParamDescriptor* maxInfo = p_Desc.defineStringParam("maxLutPatternInfo");
    maxInfo->setLabels("Max LUT size", "Max LUT size", "Generate LUT Table uses this n^3; Analyze solves this n^3 then may downsample. Source RoD or project size; click Refresh after timeline resize.");
    maxInfo->setDefault("");
    maxInfo->setAnimates(false);
    maxInfo->setParent(*mainG);

    OFX::PushButtonParamDescriptor* refreshPb = p_Desc.definePushButtonParam("lutRefreshUi");
    refreshPb->setLabels("Refresh", "Refresh", "Re-read Source / project size and update Max LUT size + export clamp.");
    refreshPb->setParent(*mainG);

    OFX::ChoiceParamDescriptor* lutSize = p_Desc.defineChoiceParam("lutExportSize");
    lutSize->setLabels("Export LUT size", "Export LUT size", "Analyze only: .cube dimension. Must divide max n for this frame. Use Refresh after changing timeline resolution.");
    lutSize->appendOption("16x16x16");
    lutSize->appendOption("32x32x32");
    lutSize->appendOption("64x64x64");
    lutSize->appendOption("128x128x128");
    lutSize->setDefault(1);
    lutSize->setAnimates(false);
    lutSize->setParent(*mainG);

    OFX::PushButtonParamDescriptor* exportPb = p_Desc.definePushButtonParam("exportLut");
    exportPb->setLabels("Export LUT", "Export LUT", "Analyze: build LUT at max capture resolution, then downsample if needed; save .cube.");
    exportPb->setParent(*mainG);

    OFX::GroupParamDescriptor* supportG = addGroup(p_Desc, "lutGenSupportGroup", "SUPPORT", "Info and log.", false);
    describeLutGeneratorSupportParams(p_Desc, *supportG);

    page->addChild(*mainG);
    page->addChild(*supportG);
}
