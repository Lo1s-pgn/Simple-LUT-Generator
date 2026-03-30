// 2D LUT-table layout (n^3 samples in frame) and feasibility for 17/32/64/128 edge sizes.
#include "LSPLutGeneratorPattern.h"
#include <algorithm>
#include <cmath>

int lspLutGenLutSizeFromChoiceIndex(int p_Index) {
    static const int kN[] = { 17, 32, 64, 128 };
    if (p_Index < 0)
        p_Index = 0;
    if (p_Index >= kLutGenChoiceCount)
        p_Index = kLutGenChoiceCount - 1;
    return kN[p_Index];
}

float lspLutGenMinPixelsPerLatticeUnit(void) {
    return 1.0f;
}

namespace {
// Factors Tx=n*a, Ty=n*b with a*b=n so Tx*Ty=n^3; pick (a,b) closest to frame aspect.
void chooseFactorsAB(int p_N, int p_FrameW, int p_FrameH, int* p_OutA, int* p_OutB) {
    *p_OutA = 1;
    *p_OutB = p_N;
    double best = 1e100;
    const double target = (p_FrameH > 0) ? static_cast<double>(p_FrameW) / static_cast<double>(p_FrameH) : 1.0;
    for (int a = 1; a <= p_N; ++a) {
        if (p_N % a != 0)
            continue;
        const int b = p_N / a;
        const double gw = static_cast<double>(p_N * a);
        const double gh = static_cast<double>(p_N * b);
        const double score = std::abs(std::log(gw / gh) - std::log(target));
        if (score < best) {
            best = score;
            *p_OutA = a;
            *p_OutB = b;
        }
    }
}

void gridDimensions(int p_N, int p_FrameW, int p_FrameH, int* p_OutTx, int* p_OutTy) {
    if (!p_OutTx || !p_OutTy || p_N < 2) {
        if (p_OutTx)
            *p_OutTx = 0;
        if (p_OutTy)
            *p_OutTy = 0;
        return;
    }
    int a = 1;
    int b = p_N;
    chooseFactorsAB(p_N, std::max(1, p_FrameW), std::max(1, p_FrameH), &a, &b);
    *p_OutTx = p_N * a;
    *p_OutTy = p_N * b;
}

bool feasibleNImpl(int p_N, int p_FrameW, int p_FrameH, float p_MinPixelsPerUnit) {
    if (p_N < 2 || p_FrameW < 1 || p_FrameH < 1)
        return false;
    int tx = 0;
    int ty = 0;
    gridDimensions(p_N, p_FrameW, p_FrameH, &tx, &ty);
    if (tx < 1 || ty < 1)
        return false;
    const float pxW = static_cast<float>(p_FrameW) / static_cast<float>(tx);
    const float pxH = static_cast<float>(p_FrameH) / static_cast<float>(ty);
    return pxW >= p_MinPixelsPerUnit && pxH >= p_MinPixelsPerUnit;
}
} // namespace

bool lspLutGenFeasibleN(int p_N, int p_FrameW, int p_FrameH, float p_MinPixelsPerUnit) {
    return feasibleNImpl(p_N, p_FrameW, p_FrameH, p_MinPixelsPerUnit);
}

int lspLutGenMaxFeasibleN(int p_FrameW, int p_FrameH, float p_MinPixelsPerUnit) {
    static const int kOrder[] = { 128, 64, 32, 17 };
    for (int n : kOrder) {
        if (feasibleNImpl(n, p_FrameW, p_FrameH, p_MinPixelsPerUnit))
            return n;
    }
    return 17;
}

// Pixel (px,py) → ref RGB for its cell; must match Generate and Analyze binning for same n.
void lspLutGenPatternRGBA(int p_Px, int p_Py, const OfxRectI& p_B, int n, float p_Out[4]) {
    const int bw = p_B.x2 - p_B.x1;
    const int bh = p_B.y2 - p_B.y1;
    if (n <= 1 || bw <= 0 || bh <= 0) {
        p_Out[0] = p_Out[1] = p_Out[2] = 0.0f;
        p_Out[3] = 1.0f;
        return;
    }
    int a = 1;
    int b = n;
    chooseFactorsAB(n, bw, bh, &a, &b);
    const int Tx = n * a;
    const int Ty = n * b;
    const double u = (static_cast<double>(p_Px - p_B.x1) + 0.5) / static_cast<double>(bw);
    const double v = (static_cast<double>(p_Py - p_B.y1) + 0.5) / static_cast<double>(bh);
    const double uCl = std::clamp(u, 0.0, 1.0);
    const double vCl = std::clamp(v, 0.0, 1.0);
    int ix = static_cast<int>(std::floor(uCl * static_cast<double>(Tx)));
    int iy = static_cast<int>(std::floor(vCl * static_cast<double>(Ty)));
    ix = std::clamp(ix, 0, Tx - 1);
    iy = std::clamp(iy, 0, Ty - 1);
    const long long tll = static_cast<long long>(iy) * static_cast<long long>(Tx) + static_cast<long long>(ix);
    const long long n3 = static_cast<long long>(n) * static_cast<long long>(n) * static_cast<long long>(n);
    long long t = tll;
    if (t < 0)
        t = 0;
    if (t >= n3)
        t = n3 - 1;
    long long tq = t;
    const int ri = static_cast<int>(tq % n);
    tq /= n;
    const int gi = static_cast<int>(tq % n);
    tq /= n;
    const int bi = static_cast<int>(tq);
    const float denom = static_cast<float>(n - 1);
    p_Out[0] = static_cast<float>(ri) / denom;
    p_Out[1] = static_cast<float>(gi) / denom;
    p_Out[2] = static_cast<float>(bi) / denom;
    p_Out[3] = 1.0f;
}
