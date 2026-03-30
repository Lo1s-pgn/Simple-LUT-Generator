#include "LSPLutGeneratorPlugin.h"
#include "LSPLutGeneratorDescribe.h"
#include "LSPLutGeneratorProcessor.h"
#include "LSPLutGeneratorConstants.h"
#include "LSPLutGeneratorCube.h"
#include "LSPLutGeneratorDialogs.h"
#include "LSPLutGeneratorLog.h"
#include "LSPLutGeneratorPattern.h"
#include "version_gen.h"
#include "ofxsCore.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

class LSPLutGeneratorPlugin : public OFX::ImageEffect {
public:
    explicit LSPLutGeneratorPlugin(OfxImageEffectHandle p_Handle);

    void render(const OFX::RenderArguments& p_Args) override;
    void beginEdit() override;
    void changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName) override;
    void endChanged(OFX::InstanceChangeReason p_Reason) override;

private:
    void updateModeDependentUi();

    OFX::Clip* m_DstClip;
    OFX::Clip* m_SrcClip;
    OFX::ChoiceParam* m_OperationMode;
    OFX::ChoiceParam* m_LutExportSize;
    OFX::PushButtonParam* m_ExportLut;
};

LSPLutGeneratorPlugin::LSPLutGeneratorPlugin(OfxImageEffectHandle p_Handle)
    : OFX::ImageEffect(p_Handle)
    , m_DstClip(nullptr)
    , m_SrcClip(nullptr)
    , m_OperationMode(nullptr)
    , m_LutExportSize(nullptr)
    , m_ExportLut(nullptr) {
    m_DstClip = fetchClip(kOfxImageEffectOutputClipName);
    m_SrcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
    m_OperationMode = fetchChoiceParam("operationMode");
    m_LutExportSize = fetchChoiceParam("lutExportSize");
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
    m_LutExportSize->setEnabled(analyze);
    m_ExportLut->setEnabled(analyze);
}

void LSPLutGeneratorPlugin::changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName) {
    (void)p_Args;
    if (p_ParamName == "lutGenHelp") {
#ifdef __APPLE__
        const char* kRepoUrl = "https://github.com/Lo1s-pgn/LSP_LUT-Generator-OFX";
        std::string cmd = std::string("open \"") + kRepoUrl + "\"";
        if (std::system(cmd.c_str()) != 0)
            LSP_LUTGEN_LOG_ERROR("open_help_url_failed");
#endif
        return;
    }
    if (p_ParamName == "lutGenOpenLog") {
#ifdef __APPLE__
        std::string path = LSPLutGeneratorLog::getLogPath();
        LSPLutGeneratorLog::ensureLogDirectoryExists(path);
        std::string cmd = "open \"" + path + "\"";
        if (std::system(cmd.c_str()) != 0)
            LSP_LUTGEN_LOG_ERROR("open_log_failed");
#endif
        return;
    }

    if (p_ParamName == "operationMode")
        updateModeDependentUi();

    if (p_ParamName != "exportLut")
        return;
    const double t = timeLineGetTime();
    int mode = 0;
    m_OperationMode->getValue(mode);
    if (mode != kOperationModeAnalyze)
        return;

    std::unique_ptr<OFX::Image> src(m_SrcClip->fetchImage(t));
    if (!src.get() || !src->getPixelData())
        return;
    if (src->getPixelDepth() != OFX::eBitDepthFloat)
        return;
    if (src->getPixelComponents() != OFX::ePixelComponentRGBA)
        return;

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
    if (!lspLutGenBuildAnalyzedCube(src.get(), nSolve, cubeFull))
        return;

    const float* cubeWrite = cubeFull.data();
    std::vector<float> cubeExport;
    if (nExport != nSolve) {
        if ((nSolve % nExport) == 0) {
            if (!lspLutGenDownsampleCubeRgba(cubeFull.data(), nSolve, nExport, cubeExport))
                return;
        } else {
            if (!lspLutGenResampleCubeRgbaTrilinear(cubeFull.data(), nSolve, nExport, cubeExport))
                return;
        }
        cubeWrite = cubeExport.data();
    }

    const std::string path = LSPLutGenShowSaveLUTDialog(nullptr);
    if (path.empty())
        return;
    if (!lspLutGenWriteCubeFile(path, nExport, cubeWrite))
        LSP_LUTGEN_LOG_ERROR("write_cube_failed");
}

void LSPLutGeneratorPlugin::beginEdit() {
    OFX::ImageEffect::beginEdit();
    updateModeDependentUi();
}

void LSPLutGeneratorPlugin::endChanged(OFX::InstanceChangeReason p_Reason) {
    updateModeDependentUi();
    OFX::ImageEffect::endChanged(p_Reason);
}

void LSPLutGeneratorPlugin::render(const OFX::RenderArguments& p_Args) {
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
    proc.setDstFullBounds(dst->getBounds());
    proc.setGenerateLutN(nUse);
    proc.setRenderWindow(p_Args.renderWindow);
    proc.process();
}

LSPLutGeneratorPluginFactory::LSPLutGeneratorPluginFactory()
    : OFX::PluginFactoryHelper<LSPLutGeneratorPluginFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor) {}

void LSPLutGeneratorPluginFactory::describe(OFX::ImageEffectDescriptor& p_Desc) {
    p_Desc.setLabels(kPluginName, kPluginName, kPluginName);
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
    p_Desc.setSupportsMetalRender(false);
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
