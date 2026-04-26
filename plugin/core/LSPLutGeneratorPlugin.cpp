// OFX plug-in instance: CPU render (LUT table fill or passthrough), Export .cube from Analyze.
#include "LSPLutGeneratorPlugin.h"
#include "LSPLutGeneratorDescribe.h"
#include "LSPLutGeneratorProcessor.h"
#include "LSPLutGeneratorConstants.h"
#include "LSPLutGeneratorCube.h"
#include "LSPLutGeneratorDialogs.h"
#include "LSPLutGeneratorLog.h"
#include "LSPLutGeneratorPattern.h"
#include "ofxsCore.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace {
/* DaVinci Resolve (and other hosts) often pass hierarchical names, e.g. "lutGenExportGroup/exportLut", in
   kOfxActionInstanceChanged. Plain "exportLut" still appears on some hosts — accept both. */
bool lspLutParamLeafIs(const std::string& pName, const char* pLeaf) {
    const size_t n = std::strlen(pLeaf);
    if (n == 0)
        return false;
    if (pName.size() < n)
        return false;
    if (pName.size() == n)
        return pName == pLeaf;
    if (pName.size() < n + 1u)
        return false;
    const char s = pName[pName.size() - n - 1u];
    if (s != '/' && s != '.')
        return false;
    return pName.compare(pName.size() - n, n, pLeaf) == 0;
}
} // namespace

class LSPLutGeneratorPlugin : public OFX::ImageEffect {
public:
    explicit LSPLutGeneratorPlugin(OfxImageEffectHandle p_Handle);

    void render(const OFX::RenderArguments& p_Args) override;
    void beginSequenceRender(const OFX::BeginSequenceRenderArguments& p_Args) override;
    void beginEdit() override;
    void changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName) override;
    void endChanged(OFX::InstanceChangeReason p_Reason) override;

private:
    void updateModeDependentUi();
    void runAnalyzeExportLut(double p_Time);

    OFX::Clip* m_DstClip;
    OFX::Clip* m_SrcClip;
    OFX::ChoiceParam* m_OperationMode;
    OFX::ChoiceParam* m_LutExportSize;
    OFX::StringParam* m_ExportFolder;
    OFX::StringParam* m_ExportFileBase;
    OFX::PushButtonParam* m_ExportLutSetPath;
    OFX::PushButtonParam* m_ExportLut;
    /** True when OFX Metal render is active (kOfxImagePropData is id<MTLBuffer>, not a CPU pointer). */
    bool m_InstanceMetalPixelData{false};
};

LSPLutGeneratorPlugin::LSPLutGeneratorPlugin(OfxImageEffectHandle p_Handle)
    : OFX::ImageEffect(p_Handle)
    , m_DstClip(nullptr)
    , m_SrcClip(nullptr)
    , m_OperationMode(nullptr)
    , m_LutExportSize(nullptr)
    , m_ExportFolder(nullptr)
    , m_ExportFileBase(nullptr)
    , m_ExportLutSetPath(nullptr)
    , m_ExportLut(nullptr) {
    m_DstClip = fetchClip(kOfxImageEffectOutputClipName);
    m_SrcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
    m_OperationMode = fetchChoiceParam("operationMode");
    m_LutExportSize = fetchChoiceParam("lutExportSize");
    m_ExportFolder = fetchStringParam("lutExportFolder");
    m_ExportFileBase = fetchStringParam("lutExportFileBase");
    m_ExportLutSetPath = fetchPushButtonParam("exportLutSetPath");
    m_ExportLut = fetchPushButtonParam("exportLut");

    OFX::StringParam* creditsLabel = fetchStringParam("lutGenCreditsLabel");
    if (creditsLabel)
        creditsLabel->setEnabled(false);

    {
        const std::string versionStr = PLUGIN_VERSION_STR;
        OFX::ImageEffectHostDescription* host = OFX::getImageEffectHostDescription();
        const std::string hostName = host ? host->hostName : "unknown";
        const std::string hostLabel = host ? host->hostLabel : "";
        std::string hostVersion;
        if (host) {
            if (!host->versionLabel.empty())
                hostVersion = host->versionLabel;
            else if (host->versionMajor != 0 || host->versionMinor != 0 || host->versionMicro != 0)
                hostVersion = std::to_string(host->versionMajor) + "." + std::to_string(host->versionMinor) + "." + std::to_string(host->versionMicro);
        }
        const std::string buildInfo = __DATE__;
        const std::string bundlePath = LSPLutGeneratorLog::getPluginBundleRootPath();
        LSP_LUTGEN_LOG_SESSION_START(kPluginName, versionStr, hostName, hostLabel, hostVersion, buildInfo, bundlePath, "");
    }

    updateModeDependentUi();
}

void LSPLutGeneratorPlugin::updateModeDependentUi() {
    int mode = 0;
    m_OperationMode->getValue(mode);
    const bool analyze = (mode == kOperationModeAnalyze);
    /* Do not setEnabled on the export GroupParam: some hosts (incl. Resolve) leave push buttons
       non-interactive when both the nested group and its children are toggled. Grey-out is from leaves. */
    m_LutExportSize->setEnabled(analyze);
    m_ExportFolder->setEnabled(analyze);
    m_ExportFileBase->setEnabled(analyze);
    m_ExportLutSetPath->setEnabled(analyze);
    m_ExportLut->setEnabled(analyze);
}

void LSPLutGeneratorPlugin::changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName) {
    if (lspLutParamLeafIs(p_ParamName, "lutGenHelp")) {
#ifdef __APPLE__
        std::string cmd = std::string("open \"") + kLutGenRepoUrl + "\"";
        if (std::system(cmd.c_str()) != 0)
            LSP_LUTGEN_LOG_ERROR("open_help_url_failed");
#endif
        return;
    }
    if (lspLutParamLeafIs(p_ParamName, "lutGenReportBug")) {
#ifdef __APPLE__
        std::string cmd = std::string("open \"") + kLutGenIssuesUrl + "\"";
        if (std::system(cmd.c_str()) != 0)
            LSP_LUTGEN_LOG_ERROR("open_report_bug_url_failed");
#endif
        return;
    }
    if (lspLutParamLeafIs(p_ParamName, "lutGenOpenLog")) {
#ifdef __APPLE__
        std::string path = LSPLutGeneratorLog::getLogPath();
        LSPLutGeneratorLog::ensureLogDirectoryExists(path);
        std::string cmd = "open \"" + path + "\"";
        if (std::system(cmd.c_str()) != 0)
            LSP_LUTGEN_LOG_ERROR("open_log_failed");
#endif
        return;
    }

    if (lspLutParamLeafIs(p_ParamName, "operationMode"))
        updateModeDependentUi();

    if (lspLutParamLeafIs(p_ParamName, "exportLutSetPath")) {
#ifdef __APPLE__
        std::string cur;
        m_ExportFolder->getValue(cur);
        const char* def = cur.empty() ? nullptr : cur.c_str();
        const std::string pick = LSPLutGenShowChooseFolderDialog(def);
        if (!pick.empty())
            m_ExportFolder->setValue(pick);
#endif
        return;
    }

    if (!lspLutParamLeafIs(p_ParamName, "exportLut"))
        return;
    runAnalyzeExportLut(p_Args.time);
}

void LSPLutGeneratorPlugin::runAnalyzeExportLut(double t) {
    // Build at nSolve (same lattice as Generate); shrink to export size if needed (box or trilinear).
    int mode = 0;
    m_OperationMode->getValue(mode);
    if (mode != kOperationModeAnalyze)
        return;

    std::string exportFolder;
    m_ExportFolder->getValue(exportFolder);
    std::string fileBase;
    m_ExportFileBase->getValue(fileBase);
    if (exportFolder.empty()) {
        const std::string msg = "Set an export folder in Export path (or use Set path file).";
        try {
            sendMessage(OFX::Message::eMessageWarning, "lspLutGenExportNoPath", msg);
        } catch (const OFX::Exception::Suite&) {
            LSP_LUTGEN_LOG_ERROR("export_no_folder_no_message_suite");
        }
        return;
    }
    const std::string outPath = lspLutGenMakeUniqueNumberedCubePath(exportFolder, fileBase);
    if (outPath.empty()) {
        const std::string msg = "The export path is not a valid, existing folder:\n" + exportFolder
            + "\n\nCheck the path, or use Set path file to choose a folder.";
        try {
            sendMessage(OFX::Message::eMessageError, "lspLutGenExportBadFolder", msg);
        } catch (const OFX::Exception::Suite&) {
            LSP_LUTGEN_LOG_ERROR("export_bad_folder_no_message_suite");
        }
        return;
    }

    std::unique_ptr<OFX::Image> src(m_SrcClip->fetchImage(t));
    if (!src.get() || !src->getPixelData()) {
        const double tFallback = timeLineGetTime();
        if (tFallback != t)
            src.reset(m_SrcClip->fetchImage(tFallback));
    }
    if (!src.get() || !src->getPixelData()) {
        const char* msg = "Export LUT: could not read the Source image at the current time.\n"
            "Scrub the playhead over the clip, then try again.";
        try {
            sendMessage(OFX::Message::eMessageWarning, "lspLutGenExportNoSource", std::string(msg));
        } catch (const OFX::Exception::Suite&) {
            LSP_LUTGEN_LOG_ERROR("export_no_source_no_message_suite");
        }
        return;
    }
    if (src->getPixelDepth() != OFX::eBitDepthFloat) {
        try {
            sendMessage(OFX::Message::eMessageWarning, "lspLutGenExportBitDepth", std::string("Source must be 32-bit float (RGB)."));
        } catch (const OFX::Exception::Suite&) {
            LSP_LUTGEN_LOG_ERROR("export_bit_depth_no_message_suite");
        }
        return;
    }
    if (src->getPixelComponents() != OFX::ePixelComponentRGBA) {
        try {
            sendMessage(OFX::Message::eMessageWarning, "lspLutGenExportComponents", std::string("Source must be RGBA for export."));
        } catch (const OFX::Exception::Suite&) {
            LSP_LUTGEN_LOG_ERROR("export_components_no_message_suite");
        }
        return;
    }

    const OfxRectI sb = src->getBounds();
    const int fw = sb.x2 - sb.x1;
    const int fh = sb.y2 - sb.y1;
    if (fw < 1 || fh < 1)
        return;

    const float minU = lspLutGenMinPixelsPerLatticeUnit();

    int lutEdgeIdx = 0;
    m_LutExportSize->getValue(lutEdgeIdx);
    if (lutEdgeIdx < 0)
        lutEdgeIdx = 0;
    const int nOpt = m_LutExportSize->getNOptions();
    if (nOpt > 0 && lutEdgeIdx >= nOpt)
        lutEdgeIdx = nOpt - 1;
    else if (lutEdgeIdx >= kLutGenChoiceCount)
        lutEdgeIdx = kLutGenChoiceCount - 1;
    const int nExport = lspLutGenLutSizeFromChoiceIndex(lutEdgeIdx);

    if (!lspLutGenFeasibleN(nExport, fw, fh, minU)) {
        char detail[256];
        std::snprintf(detail, sizeof(detail),
            "The output frame (%d x %d pixels) is too small for a %dx%dx%d LUT.\n"
            "Raise timeline resolution or pick a smaller LUT size.",
            fw, fh, nExport, nExport, nExport);
        try {
            sendMessage(OFX::Message::eMessageWarning, "lspLutGenExportResolution", std::string(detail));
        } catch (const OFX::Exception::Suite&) {
            LSP_LUTGEN_LOG_ERROR("export_resolution_too_low_no_message_suite");
        }
        return;
    }

    const int nCap = lspLutGenMaxFeasibleN(fw, fh, minU);
    const int nSolve = (nCap >= 2) ? nCap : 2;

    std::vector<float> cubeFull;
    if (!lspLutGenBuildAnalyzedCube(src.get(), nSolve, cubeFull, m_InstanceMetalPixelData)) {
        try {
            sendMessage(OFX::Message::eMessageError, "lspLutGenBuildCubeFailed", std::string("Could not build the analyzed 3D LUT from the frame."));
        } catch (const OFX::Exception::Suite&) {
            LSP_LUTGEN_LOG_ERROR("export_build_cube_no_message_suite");
        }
        LSP_LUTGEN_LOG_ERROR("export_build_cube_failed");
        return;
    }

    const float* cubeWrite = cubeFull.data();
    std::vector<float> cubeExport;
    if (nExport != nSolve) {
        if ((nSolve % nExport) == 0) {
            if (!lspLutGenDownsampleCubeRgba(cubeFull.data(), nSolve, nExport, cubeExport)) {
                try {
                    sendMessage(OFX::Message::eMessageError, "lspLutGenDownsampleFailed", std::string("Could not resize the analyzed LUT to the chosen export size."));
                } catch (const OFX::Exception::Suite&) {
                    LSP_LUTGEN_LOG_ERROR("export_downsample_no_message_suite");
                }
                return;
            }
        } else {
            if (!lspLutGenResampleCubeRgbaTrilinear(cubeFull.data(), nSolve, nExport, cubeExport)) {
                try {
                    sendMessage(OFX::Message::eMessageError, "lspLutGenResampleFailed", std::string("Could not resample the analyzed LUT to the chosen export size."));
                } catch (const OFX::Exception::Suite&) {
                    LSP_LUTGEN_LOG_ERROR("export_resample_no_message_suite");
                }
                return;
            }
        }
        cubeWrite = cubeExport.data();
    }

    if (!lspLutGenWriteCubeFile(outPath, nExport, cubeWrite)) {
        const std::string detail = std::string("Could not save the LUT file:\n") + outPath
            + "\n\nCheck folder permissions, disk space, and that the path is writable.";
        try {
            sendMessage(OFX::Message::eMessageError, "lspLutGenWriteCubeFailed", detail);
        } catch (const OFX::Exception::Suite&) {
            LSP_LUTGEN_LOG_ERROR("write_cube_failed_no_message_suite");
        }
        LSP_LUTGEN_LOG_ERROR("write_cube_failed");
    }
}

void LSPLutGeneratorPlugin::beginEdit() {
    OFX::ImageEffect::beginEdit();
    updateModeDependentUi();
}

void LSPLutGeneratorPlugin::endChanged(OFX::InstanceChangeReason p_Reason) {
    updateModeDependentUi();
    OFX::ImageEffect::endChanged(p_Reason);
}

void LSPLutGeneratorPlugin::beginSequenceRender(const OFX::BeginSequenceRenderArguments& p_Args) {
    m_InstanceMetalPixelData = p_Args.isEnabledMetalRender;
    OFX::ImageEffect::beginSequenceRender(p_Args);
}

void LSPLutGeneratorPlugin::render(const OFX::RenderArguments& p_Args) {
    m_InstanceMetalPixelData = p_Args.isEnabledMetalRender;
    // Generate: fill with pattern at max feasible N. Analyze: copy source → output.
    std::unique_ptr<OFX::Image> dst(m_DstClip->fetchImage(p_Args.time));
    std::unique_ptr<OFX::Image> src(m_SrcClip->fetchImage(p_Args.time));
    if (!dst.get() || !src.get() || !dst->getPixelData() || !src->getPixelData()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }
    if (dst->getRenderScale().x != p_Args.renderScale.x || dst->getRenderScale().y != p_Args.renderScale.y ||
        src->getRenderScale().x != p_Args.renderScale.x || src->getRenderScale().y != p_Args.renderScale.y) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
    }
    if (src->getPixelDepth() != dst->getPixelDepth() || src->getPixelComponents() != dst->getPixelComponents()) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
    }
    if (dst->getPixelDepth() != OFX::eBitDepthFloat || dst->getPixelComponents() != OFX::ePixelComponentRGBA) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    int mode = 0;
    m_OperationMode->getValueAtTime(p_Args.time, mode);

    const OfxRectI db = dst->getBounds();
    const int fw = db.x2 - db.x1;
    const int fh = db.y2 - db.y1;
    const float minU = lspLutGenMinPixelsPerLatticeUnit();
    const int nCap = lspLutGenMaxFeasibleN(fw, fh, minU);
    const int nUse = (nCap >= 2) ? nCap : 2;

    LSPLutGeneratorProcessor proc(*this);
    proc.setDstImg(dst.get());
    proc.setSrcImg(src.get());
    proc.setOperationMode(mode);
    proc.setDstFullBounds(db);
    proc.setGenerateLutN(nUse);
    proc.setRenderWindow(p_Args.renderWindow);
    proc.setGPURenderArgs(p_Args);
    proc.process();
}

LSPLutGeneratorPluginFactory::LSPLutGeneratorPluginFactory()
    : OFX::PluginFactoryHelper<LSPLutGeneratorPluginFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor) {}

void LSPLutGeneratorPluginFactory::describe(OFX::ImageEffectDescriptor& p_Desc) {
    p_Desc.setLabels(kPluginName, kPluginName, kPluginName);
    p_Desc.setVersion(PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_PATCH, 0, std::string(PLUGIN_VERSION_STR));
    p_Desc.setPluginGrouping(kPluginGrouping);
    p_Desc.setPluginDescription(kPluginDescription);
    p_Desc.addSupportedContext(OFX::eContextFilter);
    p_Desc.addSupportedContext(OFX::eContextGeneral);
    p_Desc.addSupportedBitDepth(OFX::eBitDepthFloat);
    p_Desc.setSingleInstance(false);
    p_Desc.setHostFrameThreading(false);
    p_Desc.setSupportsMultiResolution(kSupportsMultiResolution);
    p_Desc.setSupportsTiles(kSupportsTiles);
    p_Desc.setTemporalClipAccess(false);
    p_Desc.setRenderTwiceAlways(false);
    p_Desc.setSupportsMultipleClipPARs(kSupportsMultipleClipPARs);
    p_Desc.setSupportsMetalRender(true);
}

void LSPLutGeneratorPluginFactory::describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum p_Context) {
    describeLutGeneratorInContext(p_Desc, p_Context);
}

OFX::ImageEffect* LSPLutGeneratorPluginFactory::createInstance(OfxImageEffectHandle p_Handle, OFX::ContextEnum /*p_Context*/) {
    return new LSPLutGeneratorPlugin(p_Handle);
}

void OFX::Plugin::getPluginIDs(OFX::PluginFactoryArray& p_FactoryArray) {
    static LSPLutGeneratorPluginFactory s_factory;
    p_FactoryArray.push_back(&s_factory);
}
