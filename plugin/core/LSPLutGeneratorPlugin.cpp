#include "LSPLutGeneratorPlugin.h"
#include "LSPLutGeneratorDescribe.h"
#include "LSPLutGeneratorProcessor.h"
#include "LSPLutGeneratorConstants.h"
#include "LSPLutGeneratorCube.h"
#include "LSPLutGeneratorDialogs.h"
#include "LSPLutGeneratorLog.h"
#include "LSPLutGeneratorPattern.h"
#include "version_gen.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace {
int largestValidExportChoiceIndex(int p_NMax) {
    for (int i = kLutGenChoiceCount - 1; i >= 0; --i) {
        const int n = lspLutGenLutSizeFromChoiceIndex(i);
        if (lspLutGenExportSizeValid(p_NMax, n))
            return i;
    }
    return 0;
}

struct LutUiSyncGuard {
    bool& _flag;
    bool _skip;
    explicit LutUiSyncGuard(bool& p_Flag)
        : _flag(p_Flag)
        , _skip(p_Flag) {
        if (!_skip)
            _flag = true;
    }
    ~LutUiSyncGuard() {
        if (!_skip)
            _flag = false;
    }
    bool skip() const { return _skip; }
};
} // namespace

class LSPLutGeneratorPlugin : public OFX::ImageEffect {
public:
    explicit LSPLutGeneratorPlugin(OfxImageEffectHandle p_Handle);

    void render(const OFX::RenderArguments& p_Args) override;
    void beginEdit() override;
    void changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName) override;
    void changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName) override;
    void endChanged(OFX::InstanceChangeReason p_Reason) override;

private:
    bool getFrameDimensionsForLut(double p_Time, int& p_OutW, int& p_OutH) const;
    void syncLutSizeConstraintUi(double p_Time);
    void updateAnalyzeControlsEnabled(double p_Time);

    OFX::Clip* m_DstClip;
    OFX::Clip* m_SrcClip;
    OFX::ChoiceParam* m_OperationMode;
    OFX::StringParam* m_MaxLutPatternInfo;
    OFX::ChoiceParam* m_LutExportSize;
    OFX::PushButtonParam* m_ExportLut;
    bool m_InLutUiSync;
};

LSPLutGeneratorPlugin::LSPLutGeneratorPlugin(OfxImageEffectHandle p_Handle)
    : OFX::ImageEffect(p_Handle)
    , m_DstClip(nullptr)
    , m_SrcClip(nullptr)
    , m_OperationMode(nullptr)
    , m_MaxLutPatternInfo(nullptr)
    , m_LutExportSize(nullptr)
    , m_ExportLut(nullptr)
    , m_InLutUiSync(false) {
    m_DstClip = fetchClip(kOfxImageEffectOutputClipName);
    m_SrcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
    m_OperationMode = fetchChoiceParam("operationMode");
    m_MaxLutPatternInfo = fetchStringParam("maxLutPatternInfo");
    m_LutExportSize = fetchChoiceParam("lutExportSize");
    (void)fetchPushButtonParam("lutRefreshUi");
    m_ExportLut = fetchPushButtonParam("exportLut");
    (void)fetchPushButtonParam("lutGenReportBug");
    (void)fetchPushButtonParam("lutGenHelp");
    (void)fetchPushButtonParam("lutGenOpenLog");

    OFX::StringParam* creditsLabel = fetchStringParam("lutGenCreditsLabel");
    if (creditsLabel)
        creditsLabel->setEnabled(false);

    {
        std::string versionStr = PLUGIN_VERSION_STR;
        OFX::ImageEffectHostDescription* host = OFX::getImageEffectHostDescription();
        std::string hostName = host ? host->hostName : "unknown";
        std::string hostLabel = host ? host->hostLabel : "";
        std::string hostVersion;
        if (host && (host->versionMajor != 0 || host->versionMinor != 0 || host->versionMicro != 0))
            hostVersion = std::to_string(host->versionMajor) + "." + std::to_string(host->versionMinor) + "." + std::to_string(host->versionMicro);
        if (host && !host->versionLabel.empty())
            hostVersion = host->versionLabel;
        const std::string buildInfo = __DATE__;
        const std::string bundlePath = LSPLutGeneratorLog::getPluginBundleRootPath();
        LSP_LUTGEN_LOG_SESSION_START(kPluginName, versionStr, hostName, hostLabel, hostVersion, buildInfo, bundlePath, "");
    }

    const double t0 = timeLineGetTime();
    syncLutSizeConstraintUi(t0);
    updateAnalyzeControlsEnabled(t0);
}

bool LSPLutGeneratorPlugin::getFrameDimensionsForLut(double p_Time, int& p_OutW, int& p_OutH) const {
    p_OutW = 0;
    p_OutH = 0;
    if (m_SrcClip->isConnected()) {
        const OfxRectD rod = m_SrcClip->getRegionOfDefinition(p_Time);
        p_OutW = static_cast<int>(std::ceil(rod.x2 - rod.x1));
        p_OutH = static_cast<int>(std::ceil(rod.y2 - rod.y1));
    }
    if (p_OutW < 1 || p_OutH < 1) {
        const OfxPointD ps = getProjectSize();
        p_OutW = static_cast<int>(std::lround(ps.x));
        p_OutH = static_cast<int>(std::lround(ps.y));
    }
    return p_OutW >= 1 && p_OutH >= 1;
}

void LSPLutGeneratorPlugin::updateAnalyzeControlsEnabled(double p_Time) {
    int mode = 0;
    m_OperationMode->getValueAtTime(p_Time, mode);
    const bool analyze = (mode == kOperationModeAnalyze);
    m_LutExportSize->setEnabled(analyze);
    m_ExportLut->setEnabled(analyze);
}

void LSPLutGeneratorPlugin::syncLutSizeConstraintUi(double p_Time) {
    LutUiSyncGuard guard(m_InLutUiSync);
    if (guard.skip())
        return;

    int fw = 1;
    int fh = 1;
    if (!getFrameDimensionsForLut(p_Time, fw, fh))
        return;

    const float minU = lspLutGenMinPixelsPerLatticeUnit();
    const int nMax = lspLutGenMaxFeasibleN(fw, fh, minU);

    char info[64];
    std::snprintf(info, sizeof(info), "%dx%dx%d", nMax, nMax, nMax);
    m_MaxLutPatternInfo->setValueAtTime(p_Time, std::string(info));

    int idx = 0;
    m_LutExportSize->getValueAtTime(p_Time, idx);
    if (idx < 0)
        idx = 0;
    if (idx >= kLutGenChoiceCount)
        idx = kLutGenChoiceCount - 1;
    const int currentN = lspLutGenLutSizeFromChoiceIndex(idx);
    if (!lspLutGenExportSizeValid(nMax, currentN)) {
        const int newIdx = largestValidExportChoiceIndex(nMax);
        if (newIdx != idx)
            m_LutExportSize->setValueAtTime(p_Time, newIdx);
    }
}

void LSPLutGeneratorPlugin::changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName) {
    if (p_ParamName == "lutRefreshUi") {
        syncLutSizeConstraintUi(p_Args.time);
        updateAnalyzeControlsEnabled(p_Args.time);
        return;
    }
    if (p_ParamName == "lutGenReportBug" || p_ParamName == "lutGenHelp")
        return;
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

    syncLutSizeConstraintUi(p_Args.time);
    if (p_ParamName == "operationMode")
        updateAnalyzeControlsEnabled(p_Args.time);

    if (p_ParamName != "exportLut")
        return;
    int mode = 0;
    m_OperationMode->getValueAtTime(p_Args.time, mode);
    if (mode != kOperationModeAnalyze)
        return;

    int fw = 0;
    int fh = 0;
    if (!getFrameDimensionsForLut(p_Args.time, fw, fh))
        return;
    const int nMax = lspLutGenMaxFeasibleN(fw, fh, lspLutGenMinPixelsPerLatticeUnit());

    int lutEdgeIdx = 0;
    m_LutExportSize->getValueAtTime(p_Args.time, lutEdgeIdx);
    const int nExport = lspLutGenLutSizeFromChoiceIndex(lutEdgeIdx);
    if (!lspLutGenExportSizeValid(nMax, nExport))
        return;

    std::unique_ptr<OFX::Image> src(m_SrcClip->fetchImage(p_Args.time));
    if (!src.get() || !src->getPixelData())
        return;
    if (src->getPixelDepth() != OFX::eBitDepthFloat)
        return;
    if (src->getPixelComponents() != OFX::ePixelComponentRGBA)
        return;

    std::vector<float> cubeMax;
    if (!lspLutGenBuildAnalyzedCube(src.get(), nMax, cubeMax))
        return;

    if (nExport == nMax) {
        const std::string path = LSPLutGenShowSaveLUTDialog(nullptr);
        if (path.empty())
            return;
        if (!lspLutGenWriteCubeFile(path, nMax, cubeMax.data()))
            LSP_LUTGEN_LOG_ERROR("write_cube_failed");
        return;
    }

    std::vector<float> cubeOut;
    if (!lspLutGenDownsampleCubeRgba(cubeMax.data(), nMax, nExport, cubeOut))
        return;
    const std::string path = LSPLutGenShowSaveLUTDialog(nullptr);
    if (path.empty())
        return;
    if (!lspLutGenWriteCubeFile(path, nExport, cubeOut.data()))
        LSP_LUTGEN_LOG_ERROR("write_cube_failed");
}

void LSPLutGeneratorPlugin::beginEdit() {
    OFX::ImageEffect::beginEdit();
    const double t = timeLineGetTime();
    syncLutSizeConstraintUi(t);
    updateAnalyzeControlsEnabled(t);
}

void LSPLutGeneratorPlugin::changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName) {
    OFX::ImageEffect::changedClip(p_Args, p_ClipName);
    if (p_ClipName == std::string(kOfxImageEffectSimpleSourceClipName))
        syncLutSizeConstraintUi(p_Args.time);
}

void LSPLutGeneratorPlugin::endChanged(OFX::InstanceChangeReason p_Reason) {
    if (m_InLutUiSync) {
        OFX::ImageEffect::endChanged(p_Reason);
        return;
    }
    const double t = timeLineGetTime();
    syncLutSizeConstraintUi(t);
    updateAnalyzeControlsEnabled(t);
    OFX::ImageEffect::endChanged(p_Reason);
}

void LSPLutGeneratorPlugin::render(const OFX::RenderArguments& p_Args) {
    // Do not call isConnected() on the output clip — hosts (including Resolve) may not define it like inputs.
    std::unique_ptr<OFX::Image> dst(m_DstClip->fetchImage(p_Args.time));
    std::unique_ptr<OFX::Image> src(m_SrcClip->fetchImage(p_Args.time));
    if (!dst.get() || !src.get() || !dst->getPixelData() || !src->getPixelData()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }
    if (dst->getRenderScale().x != p_Args.renderScale.x || dst->getRenderScale().y != p_Args.renderScale.y ||
        src->getRenderScale().x != p_Args.renderScale.x || src->getRenderScale().y != p_Args.renderScale.y) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }
    if (src->getPixelDepth() != dst->getPixelDepth() || src->getPixelComponents() != dst->getPixelComponents()) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }
    if (dst->getPixelDepth() != OFX::eBitDepthFloat || dst->getPixelComponents() != OFX::ePixelComponentRGBA) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    int mode = 0;
    m_OperationMode->getValueAtTime(p_Args.time, mode);

    const OfxRectI db = dst->getBounds();
    const int fw = db.x2 - db.x1;
    const int fh = db.y2 - db.y1;
    const int nGen = lspLutGenMaxFeasibleN(fw, fh, lspLutGenMinPixelsPerLatticeUnit());

    LSPLutGeneratorProcessor proc(*this);
    proc.setDstImg(dst.get());
    proc.setSrcImg(src.get());
    proc.setOperationMode(mode);
    proc.setDstFullBounds(dst->getBounds());
    proc.setGenerateLutN(nGen);
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
