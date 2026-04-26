// 3D LUT: accumulate graded RGB per pattern cell, resize cube, write Iridas .cube text.
#include "LSPLutGeneratorCube.h"
#include "LSPLutGeneratorPattern.h"
#include "ofxsImageEffect.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>

#if defined(__APPLE__)
#include "LSPLutGeneratorMetalCube.h"
#endif

namespace {
struct AccumCell {
    double r;
    double g;
    double b;
    int count;
};

void sampleCubeRgbTriLinear(const float* p_Cube, int p_N, double p_Fr, double p_Fg, double p_Fb, double* p_OutRgb) {
    const double nm1 = static_cast<double>(p_N - 1);
    const double fr = std::clamp(p_Fr, 0.0, nm1);
    const double fg = std::clamp(p_Fg, 0.0, nm1);
    const double fb = std::clamp(p_Fb, 0.0, nm1);
    const int ir0 = static_cast<int>(std::floor(fr));
    const int ig0 = static_cast<int>(std::floor(fg));
    const int ib0 = static_cast<int>(std::floor(fb));
    const int ir1 = std::min(ir0 + 1, p_N - 1);
    const int ig1 = std::min(ig0 + 1, p_N - 1);
    const int ib1 = std::min(ib0 + 1, p_N - 1);
    const double tr = fr - static_cast<double>(ir0);
    const double tg = fg - static_cast<double>(ig0);
    const double tb = fb - static_cast<double>(ib0);
    auto cell = [p_Cube, p_N](int ir, int ig, int ib) -> const float* {
        const size_t idx = (size_t)ir + (size_t)p_N * ((size_t)ig + (size_t)p_N * (size_t)ib);
        return p_Cube + idx * 4u;
    };
    const float* p000 = cell(ir0, ig0, ib0);
    const float* p100 = cell(ir1, ig0, ib0);
    const float* p010 = cell(ir0, ig1, ib0);
    const float* p110 = cell(ir1, ig1, ib0);
    const float* p001 = cell(ir0, ig0, ib1);
    const float* p101 = cell(ir1, ig0, ib1);
    const float* p011 = cell(ir0, ig1, ib1);
    const float* p111 = cell(ir1, ig1, ib1);
    for (int c = 0; c < 3; ++c) {
        const double c000 = p000[c];
        const double c100 = p100[c];
        const double c010 = p010[c];
        const double c110 = p110[c];
        const double c001 = p001[c];
        const double c101 = p101[c];
        const double c011 = p011[c];
        const double c111 = p111[c];
        const double c00 = c000 + tr * (c100 - c000);
        const double c01 = c001 + tr * (c101 - c001);
        const double c10 = c010 + tr * (c110 - c010);
        const double c11 = c011 + tr * (c111 - c011);
        const double c0 = c00 + tg * (c10 - c00);
        const double c1 = c01 + tg * (c11 - c01);
        p_OutRgb[c] = c0 + tb * (c1 - c0);
    }
}
} // namespace

// Bin pixels by pattern ref at p_N; average source RGB per cell (empty → identity ramp).
// baseRow00 points to float at (B.x1, B.y1); rowBytes as OFX (may be negative for CPU images).
bool lspLutGenBuildAnalyzedCubeFromLinearBase(const OfxRectI& B, int p_N, const float* baseRow00, int rowBytes, std::vector<float>& p_OutRgba) {
    if (!baseRow00 || p_N < 2)
        return false;
    if (B.x2 <= B.x1 || B.y2 <= B.y1)
        return false;

    const size_t n3 = (size_t)p_N * (size_t)p_N * (size_t)p_N;
    std::vector<AccumCell> acc(n3);

    const int nm1 = p_N - 1;
    for (int y = B.y1; y < B.y2; ++y) {
        for (int x = B.x1; x < B.x2; ++x) {
            float ref[4];
            lspLutGenPatternRGBA(x, y, B, p_N, ref);
            const float sr = std::clamp(ref[0], 0.0f, 1.0f);
            const float sg = std::clamp(ref[1], 0.0f, 1.0f);
            const float sb = std::clamp(ref[2], 0.0f, 1.0f);
            int ir = static_cast<int>(std::lround(static_cast<double>(sr) * static_cast<double>(nm1)));
            int ig = static_cast<int>(std::lround(static_cast<double>(sg) * static_cast<double>(nm1)));
            int ib = static_cast<int>(std::lround(static_cast<double>(sb) * static_cast<double>(nm1)));
            ir = std::clamp(ir, 0, p_N - 1);
            ig = std::clamp(ig, 0, p_N - 1);
            ib = std::clamp(ib, 0, p_N - 1);
            const size_t idx = (size_t)ir + (size_t)p_N * ((size_t)ig + (size_t)p_N * (size_t)ib);
            const ptrdiff_t off = (ptrdiff_t)(y - B.y1) * (ptrdiff_t)rowBytes + (ptrdiff_t)(x - B.x1) * 16;
            const float* gpx = reinterpret_cast<const float*>(reinterpret_cast<const char*>(baseRow00) + off);
            AccumCell& a = acc[idx];
            a.r += static_cast<double>(gpx[0]);
            a.g += static_cast<double>(gpx[1]);
            a.b += static_cast<double>(gpx[2]);
            a.count += 1;
        }
    }

    p_OutRgba.resize(n3 * 4u);
    const float inv = 1.0f / static_cast<float>(nm1);
    for (size_t ib = 0; ib < (size_t)p_N; ++ib) {
        for (size_t ig = 0; ig < (size_t)p_N; ++ig) {
            for (size_t ir = 0; ir < (size_t)p_N; ++ir) {
                const size_t idxCell = ir + (size_t)p_N * (ig + (size_t)p_N * ib);
                const size_t o = idxCell * 4u;
                const AccumCell& a = acc[idxCell];
                if (a.count > 0) {
                    const double invc = 1.0 / static_cast<double>(a.count);
                    p_OutRgba[o + 0] = static_cast<float>(a.r * invc);
                    p_OutRgba[o + 1] = static_cast<float>(a.g * invc);
                    p_OutRgba[o + 2] = static_cast<float>(a.b * invc);
                } else {
                    p_OutRgba[o + 0] = static_cast<float>(ir) * inv;
                    p_OutRgba[o + 1] = static_cast<float>(ig) * inv;
                    p_OutRgba[o + 2] = static_cast<float>(ib) * inv;
                }
                p_OutRgba[o + 3] = 1.0f;
            }
        }
    }
    return true;
}

bool lspLutGenBuildAnalyzedCube(OFX::Image* p_GradedStrip, int p_N, std::vector<float>& p_OutRgba, bool p_PixelDataIsMetalBuffer) {
    if (!p_GradedStrip || p_N < 2)
        return false;
    if (p_GradedStrip->getPixelDepth() != OFX::eBitDepthFloat)
        return false;
    if (p_GradedStrip->getPixelComponents() != OFX::ePixelComponentRGBA)
        return false;
    if (!p_GradedStrip->getPixelData())
        return false;

    const OfxRectI B = p_GradedStrip->getBounds();
    if (B.x2 <= B.x1 || B.y2 <= B.y1)
        return false;

    const size_t n3 = (size_t)p_N * (size_t)p_N * (size_t)p_N;

#if defined(__APPLE__)
    {
        int tx = 0;
        int ty = 0;
        lspLutGenPatternGridTxTy(p_N, B.x2 - B.x1, B.y2 - B.y1, &tx, &ty);
        if (lspLutGenMetalTryBuildAnalyzedCube(p_GradedStrip->getPixelData(),
                                               p_PixelDataIsMetalBuffer,
                                               p_GradedStrip->getRowBytes(),
                                               B.x1,
                                               B.y1,
                                               B.x2,
                                               B.y2,
                                               p_N,
                                               tx,
                                               ty,
                                               p_OutRgba))
            return true;
        if (p_PixelDataIsMetalBuffer) {
            if (lspLutGenCpuBuildAnalyzedCubeFromMetalBuffer(p_GradedStrip->getPixelData(),
                                                             p_GradedStrip->getRowBytes(),
                                                             B.x1,
                                                             B.y1,
                                                             B.x2,
                                                             B.y2,
                                                             p_N,
                                                             p_OutRgba))
                return true;
            /* Last render was Metal, but this image can be a CPU map in instance-changed. Fall through. */
        }
    }
#endif

    std::vector<AccumCell> acc(n3);

    const int nm1 = p_N - 1;
    for (int y = B.y1; y < B.y2; ++y) {
        for (int x = B.x1; x < B.x2; ++x) {
            float ref[4];
            lspLutGenPatternRGBA(x, y, B, p_N, ref);
            const float sr = std::clamp(ref[0], 0.0f, 1.0f);
            const float sg = std::clamp(ref[1], 0.0f, 1.0f);
            const float sb = std::clamp(ref[2], 0.0f, 1.0f);
            int ir = static_cast<int>(std::lround(static_cast<double>(sr) * static_cast<double>(nm1)));
            int ig = static_cast<int>(std::lround(static_cast<double>(sg) * static_cast<double>(nm1)));
            int ib = static_cast<int>(std::lround(static_cast<double>(sb) * static_cast<double>(nm1)));
            ir = std::clamp(ir, 0, p_N - 1);
            ig = std::clamp(ig, 0, p_N - 1);
            ib = std::clamp(ib, 0, p_N - 1);
            const size_t idx = (size_t)ir + (size_t)p_N * ((size_t)ig + (size_t)p_N * (size_t)ib);
            const float* gpx = static_cast<const float*>(p_GradedStrip->getPixelAddress(x, y));
            if (!gpx)
                continue;
            AccumCell& a = acc[idx];
            a.r += static_cast<double>(gpx[0]);
            a.g += static_cast<double>(gpx[1]);
            a.b += static_cast<double>(gpx[2]);
            a.count += 1;
        }
    }

    p_OutRgba.resize(n3 * 4u);
    const float inv = 1.0f / static_cast<float>(nm1);
    for (size_t ib = 0; ib < (size_t)p_N; ++ib) {
        for (size_t ig = 0; ig < (size_t)p_N; ++ig) {
            for (size_t ir = 0; ir < (size_t)p_N; ++ir) {
                const size_t idxCell = ir + (size_t)p_N * (ig + (size_t)p_N * ib);
                const size_t o = idxCell * 4u;
                const AccumCell& a = acc[idxCell];
                if (a.count > 0) {
                    const double invc = 1.0 / static_cast<double>(a.count);
                    p_OutRgba[o + 0] = static_cast<float>(a.r * invc);
                    p_OutRgba[o + 1] = static_cast<float>(a.g * invc);
                    p_OutRgba[o + 2] = static_cast<float>(a.b * invc);
                } else {
                    p_OutRgba[o + 0] = static_cast<float>(ir) * inv;
                    p_OutRgba[o + 1] = static_cast<float>(ig) * inv;
                    p_OutRgba[o + 2] = static_cast<float>(ib) * inv;
                }
                p_OutRgba[o + 3] = 1.0f;
            }
        }
    }
    return true;
}

// Box average over s×s×s source voxels per output cell; requires p_NSrc % p_NDst == 0.
bool lspLutGenDownsampleCubeRgba(const float* p_SrcRgba, int p_NSrc, int p_NDst, std::vector<float>& p_OutRgba) {
    if (!p_SrcRgba || p_NSrc < 2 || p_NDst < 2 || (p_NSrc % p_NDst) != 0)
        return false;

#if defined(__APPLE__)
    if (lspLutGenMetalTryDownsampleCube(p_SrcRgba, p_NSrc, p_NDst, p_OutRgba))
        return true;
#endif

    const int s = p_NSrc / p_NDst;
    const double invVol = 1.0 / static_cast<double>(s * s * s);
    const size_t nOut3 = (size_t)p_NDst * (size_t)p_NDst * (size_t)p_NDst;
    p_OutRgba.resize(nOut3 * 4u);
    for (int bo = 0; bo < p_NDst; ++bo) {
        for (int go = 0; go < p_NDst; ++go) {
            for (int ro = 0; ro < p_NDst; ++ro) {
                double ar = 0.0;
                double ag = 0.0;
                double ab = 0.0;
                for (int dz = 0; dz < s; ++dz) {
                    for (int dy = 0; dy < s; ++dy) {
                        for (int dx = 0; dx < s; ++dx) {
                            const int ir = ro * s + dx;
                            const int ig = go * s + dy;
                            const int ib = bo * s + dz;
                            const size_t idx = (size_t)ir + (size_t)p_NSrc * ((size_t)ig + (size_t)p_NSrc * (size_t)ib);
                            ar += static_cast<double>(p_SrcRgba[idx * 4u + 0u]);
                            ag += static_cast<double>(p_SrcRgba[idx * 4u + 1u]);
                            ab += static_cast<double>(p_SrcRgba[idx * 4u + 2u]);
                        }
                    }
                }
                const size_t oidx = (size_t)ro + (size_t)p_NDst * ((size_t)go + (size_t)p_NDst * (size_t)bo);
                p_OutRgba[oidx * 4u + 0u] = static_cast<float>(ar * invVol);
                p_OutRgba[oidx * 4u + 1u] = static_cast<float>(ag * invVol);
                p_OutRgba[oidx * 4u + 2u] = static_cast<float>(ab * invVol);
                p_OutRgba[oidx * 4u + 3u] = 1.0f;
            }
        }
    }
    return true;
}

// Domain-aligned trilinear sample (e.g. 128 → 17 when box downsample does not apply).
bool lspLutGenResampleCubeRgbaTrilinear(const float* p_SrcRgba, int p_NSrc, int p_NDst, std::vector<float>& p_OutRgba) {
    if (!p_SrcRgba || p_NSrc < 2 || p_NDst < 2)
        return false;

#if defined(__APPLE__)
    if (lspLutGenMetalTryResampleCubeTrilinear(p_SrcRgba, p_NSrc, p_NDst, p_OutRgba))
        return true;
#endif

    const double srcNm1 = static_cast<double>(p_NSrc - 1);
    const double dstNm1 = static_cast<double>(p_NDst - 1);
    const size_t nOut3 = (size_t)p_NDst * (size_t)p_NDst * (size_t)p_NDst;
    p_OutRgba.resize(nOut3 * 4u);
    double rgb[3];
    for (int bo = 0; bo < p_NDst; ++bo) {
        for (int go = 0; go < p_NDst; ++go) {
            for (int ro = 0; ro < p_NDst; ++ro) {
                const double ur = (dstNm1 > 0.0) ? static_cast<double>(ro) / dstNm1 : 0.0;
                const double ug = (dstNm1 > 0.0) ? static_cast<double>(go) / dstNm1 : 0.0;
                const double ub = (dstNm1 > 0.0) ? static_cast<double>(bo) / dstNm1 : 0.0;
                const double fr = ur * srcNm1;
                const double fg = ug * srcNm1;
                const double fb = ub * srcNm1;
                sampleCubeRgbTriLinear(p_SrcRgba, p_NSrc, fr, fg, fb, rgb);
                const size_t oidx = (size_t)ro + (size_t)p_NDst * ((size_t)go + (size_t)p_NDst * (size_t)bo);
                p_OutRgba[oidx * 4u + 0u] = static_cast<float>(rgb[0]);
                p_OutRgba[oidx * 4u + 1u] = static_cast<float>(rgb[1]);
                p_OutRgba[oidx * 4u + 2u] = static_cast<float>(rgb[2]);
                p_OutRgba[oidx * 4u + 3u] = 1.0f;
            }
        }
    }
    return true;
}

static std::string lspLutGenTrimCopyForExportPath(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\n' || s[a] == '\r'))
        ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\n' || s[b - 1] == '\r'))
        --b;
    return s.substr(a, b - a);
}

static std::string lspLutGenSanitizeExportBaseName(const std::string& p_Raw) {
    std::string t = lspLutGenTrimCopyForExportPath(p_Raw);
    if (t.size() >= 5) {
        bool endsCube = true;
        const char* suf = ".cube";
        for (size_t i = 0; i < 5; ++i) {
            if (std::tolower((unsigned char)t[t.size() - 5 + i]) != (unsigned char)suf[i]) {
                endsCube = false;
                break;
            }
        }
        if (endsCube)
            t = t.substr(0, t.size() - 5);
    }
    for (char& c : t) {
        if (c == '/' || c == '\\')
            c = '_';
    }
    t = lspLutGenTrimCopyForExportPath(t);
    if (t.empty())
        t = "LUT";
    return t;
}

std::string lspLutGenMakeUniqueNumberedCubePath(const std::string& p_Directory, const std::string& p_RawFileBase) {
    namespace fs = std::filesystem;
    const std::string dirTrim = lspLutGenTrimCopyForExportPath(p_Directory);
    if (dirTrim.empty())
        return "";
    const std::string stem = lspLutGenSanitizeExportBaseName(p_RawFileBase);
    try {
        const fs::path dir = fs::path(dirTrim);
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec))
            return "";
        for (int n = 1; n < 100000; ++n) {
            char sfx[32];
            std::snprintf(sfx, sizeof(sfx), "_%03d", n);
            const fs::path out = dir / (stem + std::string(sfx) + ".cube");
            if (!fs::exists(out, ec))
                return out.string();
        }
    } catch (...) {
        return "";
    }
    return "";
}

// Iridas .cube text: DOMAIN 0–1, LUT_3D_SIZE, then r-major RGB rows.
bool lspLutGenWriteCubeFile(const std::string& p_Path, int p_N, const float* p_BufferRgba) {
    if (!p_BufferRgba || p_N < 2)
        return false;
    std::ofstream out(p_Path);
    if (!out)
        return false;
    const long long n = (long long)p_N * p_N * p_N;
    out << "# LSP - Simple LUT Generator analyzed LUT " << p_N << "^3\n";
    out << "DOMAIN_MIN 0.0 0.0 0.0\n";
    out << "DOMAIN_MAX 1.0 1.0 1.0\n";
    out << "LUT_3D_SIZE " << p_N << "\n";
    for (long long i = 0; i < n; ++i) {
        const float r = p_BufferRgba[(size_t)i * 4u + 0u];
        const float g = p_BufferRgba[(size_t)i * 4u + 1u];
        const float b = p_BufferRgba[(size_t)i * 4u + 2u];
        out << r << " " << g << " " << b << "\n";
    }
    return out.good();
}
