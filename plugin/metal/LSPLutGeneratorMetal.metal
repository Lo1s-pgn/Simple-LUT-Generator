#include <metal_stdlib>
#include <metal_atomic>
using namespace metal;

// Must match LspLutGenMetalUniforms in LSPLutGeneratorMetal.mm (80 bytes).
struct LspLutGenMetalUniforms {
    int dst_x1;
    int dst_y1;
    int dst_x2;
    int dst_y2;
    int src_x1;
    int src_y1;
    int src_x2;
    int src_y2;
    int rw_x1;
    int rw_y1;
    int rw_x2;
    int rw_y2;
    int dst_row_bytes;
    int src_row_bytes;
    int mode;
    int n;
    int Tx;
    int Ty;
    int _pad0;
    int _pad1;
};

kernel void lutgen_generate(constant LspLutGenMetalUniforms& u [[buffer(0)]],
                            device uchar* dst [[buffer(1)]],
                            uint2 gid [[thread_position_in_grid]]) {
    const int x = u.rw_x1 + int(gid.x);
    const int y = u.rw_y1 + int(gid.y);
    if (x >= u.rw_x2 || y >= u.rw_y2)
        return;

    const int bw = u.dst_x2 - u.dst_x1;
    const int bh = u.dst_y2 - u.dst_y1;
    const int n = u.n;
    const int Tx = u.Tx;
    const int Ty = u.Ty;

    float4 rgba;
    if (n <= 1 || bw <= 0 || bh <= 0) {
        rgba = float4(0.0f, 0.0f, 0.0f, 1.0f);
    } else {
        const double uu = clamp((double(x - u.dst_x1) + 0.5) / double(bw), 0.0, 1.0);
        const double vv = clamp((double(y - u.dst_y1) + 0.5) / double(bh), 0.0, 1.0);
        int ix = int(floor(uu * double(Tx)));
        int iy = int(floor(vv * double(Ty)));
        ix = clamp(ix, 0, Tx - 1);
        iy = clamp(iy, 0, Ty - 1);
        const long long tll = long long(iy) * long long(Tx) + long long(ix);
        const long long n3 = long long(n) * long long(n) * long long(n);
        long long t = tll;
        if (t < 0)
            t = 0;
        if (t >= n3)
            t = n3 - 1;
        long long tq = t;
        const int ri = int(tq % long long(n));
        tq /= long long(n);
        const int gi = int(tq % long long(n));
        tq /= long long(n);
        const int bi = int(tq);
        const float denom = float(n - 1);
        rgba = float4(float(ri) / denom, float(gi) / denom, float(bi) / denom, 1.0f);
    }

    const ulong doff = ulong(y - u.dst_y1) * ulong(u.dst_row_bytes) + ulong(x - u.dst_x1) * 16ul;
    device float4* dstPx = (device float4*)(dst + doff);
    *dstPx = rgba;
}

kernel void lutgen_copy(constant LspLutGenMetalUniforms& u [[buffer(0)]],
                        device uchar* dst [[buffer(1)]],
                        device const uchar* src [[buffer(2)]],
                        uint2 gid [[thread_position_in_grid]]) {
    const int x = u.rw_x1 + int(gid.x);
    const int y = u.rw_y1 + int(gid.y);
    if (x >= u.rw_x2 || y >= u.rw_y2)
        return;

    const ulong doff = ulong(y - u.dst_y1) * ulong(u.dst_row_bytes) + ulong(x - u.dst_x1) * 16ul;
    const ulong soff = ulong(y - u.src_y1) * ulong(u.src_row_bytes) + ulong(x - u.src_x1) * 16ul;
    device float4* dstPx = (device float4*)(dst + doff);
    device const float4* srcPx = (device const float4*)(src + soff);
    *dstPx = *srcPx;
}

// Fixed-point scale for atomic sums (must match LSPLutGeneratorMetalCube.mm).
constant uint kLutGenAtomicScale = 1048576u;

struct LspBinUniforms {
    int bx1;
    int by1;
    int bx2;
    int by2;
    int row_bytes;
    int n;
    int Tx;
    int Ty;
    int pad0;
    int pad1;
    int pad2;
    int pad3;
};

// Analyze export: accumulate graded RGB per lattice cell (matches CPU lspLutGenBuildAnalyzedCube).
kernel void lutgen_bin_pixels(constant LspBinUniforms& u [[buffer(0)]],
                              device const uchar* src [[buffer(1)]],
                              device atomic_ulong* sum_r [[buffer(2)]],
                              device atomic_ulong* sum_g [[buffer(3)]],
                              device atomic_ulong* sum_b [[buffer(4)]],
                              device atomic_uint* cnt [[buffer(5)]],
                              uint2 gid [[thread_position_in_grid]]) {
    const int x = u.bx1 + int(gid.x);
    const int y = u.by1 + int(gid.y);
    if (x >= u.bx2 || y >= u.by2)
        return;

    const int bw = u.bx2 - u.bx1;
    const int bh = u.by2 - u.by1;
    const int n = u.n;
    const int Tx = u.Tx;
    const int Ty = u.Ty;

    if (n <= 1 || bw <= 0 || bh <= 0)
        return;

    const double uu = clamp((double(x - u.bx1) + 0.5) / double(bw), 0.0, 1.0);
    const double vv = clamp((double(y - u.by1) + 0.5) / double(bh), 0.0, 1.0);
    int ix = int(floor(uu * double(Tx)));
    int iy = int(floor(vv * double(Ty)));
    ix = clamp(ix, 0, Tx - 1);
    iy = clamp(iy, 0, Ty - 1);
    const long long tll = long long(iy) * long long(Tx) + long long(ix);
    const long long n3 = long long(n) * long long(n) * long long(n);
    long long t = tll;
    if (t < 0)
        t = 0;
    if (t >= n3)
        t = n3 - 1;
    long long tq = t;
    const int ri = int(tq % long long(n));
    tq /= long long(n);
    const int gi = int(tq % long long(n));
    tq /= long long(n);
    const int bi = int(tq);
    const float denom = float(n - 1);
    const float sr = clamp(float(ri) / denom, 0.0f, 1.0f);
    const float sg = clamp(float(gi) / denom, 0.0f, 1.0f);
    const float sb = clamp(float(bi) / denom, 0.0f, 1.0f);

    const int nm1 = n - 1;
    int ir = int(round(double(sr) * double(nm1)));
    int ig = int(round(double(sg) * double(nm1)));
    int ib = int(round(double(sb) * double(nm1)));
    ir = clamp(ir, 0, n - 1);
    ig = clamp(ig, 0, n - 1);
    ib = clamp(ib, 0, n - 1);
    const uint idx = uint(ir + n * (ig + n * ib));

    const ulong off = ulong(y - u.by1) * ulong(u.row_bytes) + ulong(x - u.bx1) * 16ul;
    device const float4* px = (device const float4*)(src + off);
    const float4 g = *px;

    const ulong sc_r = ulong(clamp(g.x, 0.0f, 1.0f) * float(kLutGenAtomicScale));
    const ulong sc_g = ulong(clamp(g.y, 0.0f, 1.0f) * float(kLutGenAtomicScale));
    const ulong sc_b = ulong(clamp(g.z, 0.0f, 1.0f) * float(kLutGenAtomicScale));

    atomic_fetch_add_explicit(sum_r + idx, sc_r, memory_order_relaxed);
    atomic_fetch_add_explicit(sum_g + idx, sc_g, memory_order_relaxed);
    atomic_fetch_add_explicit(sum_b + idx, sc_b, memory_order_relaxed);
    atomic_fetch_add_explicit(cnt + idx, 1u, memory_order_relaxed);
}

struct LspCubeBoxUniforms {
    int n_src;
    int n_dst;
    int s;
    int pad;
};

kernel void lutgen_downsample_cube(constant LspCubeBoxUniforms& u [[buffer(0)]],
                                   device const float* src [[buffer(1)]],
                                   device float* dst [[buffer(2)]],
                                   uint3 gid [[thread_position_in_grid]]) {
    const int nDst = u.n_dst;
    const int ro = int(gid.x);
    const int go = int(gid.y);
    const int bo = int(gid.z);
    if (ro >= nDst || go >= nDst || bo >= nDst)
        return;

    const int s = u.s;
    const int nSrc = u.n_src;
    double ar = 0.0;
    double ag = 0.0;
    double ab = 0.0;
    for (int dz = 0; dz < s; ++dz) {
        for (int dy = 0; dy < s; ++dy) {
            for (int dx = 0; dx < s; ++dx) {
                const int ir = ro * s + dx;
                const int ig = go * s + dy;
                const int ib = bo * s + dz;
                const ulong idx = ulong(ir) + ulong(nSrc) * (ulong(ig) + ulong(nSrc) * ulong(ib));
                ar += double(src[idx * 4u + 0u]);
                ag += double(src[idx * 4u + 1u]);
                ab += double(src[idx * 4u + 2u]);
            }
        }
    }
    const double invVol = 1.0 / double(s * s * s);
    const ulong oidx = ulong(ro) + ulong(nDst) * (ulong(go) + ulong(nDst) * ulong(bo));
    dst[oidx * 4u + 0u] = float(ar * invVol);
    dst[oidx * 4u + 1u] = float(ag * invVol);
    dst[oidx * 4u + 2u] = float(ab * invVol);
    dst[oidx * 4u + 3u] = 1.0f;
}

struct LspTriUniforms {
    int n_src;
    int n_dst;
    int pad0;
    int pad1;
};

static float lutgen_cell_c(device const float* cube, int N, int ir, int ig, int ib, int c) {
    const ulong idx = ulong(ir) + ulong(N) * (ulong(ig) + ulong(N) * ulong(ib));
    return cube[idx * 4u + ulong(c)];
}

kernel void lutgen_trilinear_cube(constant LspTriUniforms& u [[buffer(0)]],
                                  device const float* src [[buffer(1)]],
                                  device float* dst [[buffer(2)]],
                                  uint3 gid [[thread_position_in_grid]]) {
    const int nDst = u.n_dst;
    const int ro = int(gid.x);
    const int go = int(gid.y);
    const int bo = int(gid.z);
    if (ro >= nDst || go >= nDst || bo >= nDst)
        return;

    const int nSrc = u.n_src;
    const double srcNm1 = double(nSrc - 1);
    const double dstNm1 = double(nDst - 1);
    const double ur = (dstNm1 > 0.0) ? double(ro) / dstNm1 : 0.0;
    const double ug = (dstNm1 > 0.0) ? double(go) / dstNm1 : 0.0;
    const double ub = (dstNm1 > 0.0) ? double(bo) / dstNm1 : 0.0;
    double fr = ur * srcNm1;
    double fg = ug * srcNm1;
    double fb = ub * srcNm1;
    const double nm1 = double(nSrc - 1);
    fr = clamp(fr, 0.0, nm1);
    fg = clamp(fg, 0.0, nm1);
    fb = clamp(fb, 0.0, nm1);
    const int ir0 = int(floor(fr));
    const int ig0 = int(floor(fg));
    const int ib0 = int(floor(fb));
    const int ir1 = min(ir0 + 1, nSrc - 1);
    const int ig1 = min(ig0 + 1, nSrc - 1);
    const int ib1 = min(ib0 + 1, nSrc - 1);
    const double tr = fr - double(ir0);
    const double tg = fg - double(ig0);
    const double tb = fb - double(ib0);
    const ulong oidx = ulong(ro) + ulong(nDst) * (ulong(go) + ulong(nDst) * ulong(bo));
    for (int c = 0; c < 3; ++c) {
        const double c000 = lutgen_cell_c(src, nSrc, ir0, ig0, ib0, c);
        const double c100 = lutgen_cell_c(src, nSrc, ir1, ig0, ib0, c);
        const double c010 = lutgen_cell_c(src, nSrc, ir0, ig1, ib0, c);
        const double c110 = lutgen_cell_c(src, nSrc, ir1, ig1, ib0, c);
        const double c001 = lutgen_cell_c(src, nSrc, ir0, ig0, ib1, c);
        const double c101 = lutgen_cell_c(src, nSrc, ir1, ig0, ib1, c);
        const double c011 = lutgen_cell_c(src, nSrc, ir0, ig1, ib1, c);
        const double c111 = lutgen_cell_c(src, nSrc, ir1, ig1, ib1, c);
        const double c00 = c000 + tr * (c100 - c000);
        const double c01 = c001 + tr * (c101 - c001);
        const double c10 = c010 + tr * (c110 - c010);
        const double c11 = c011 + tr * (c111 - c011);
        const double c0 = c00 + tg * (c10 - c00);
        const double c1 = c01 + tg * (c11 - c01);
        const float v = float(c0 + tb * (c1 - c0));
        dst[oidx * 4u + ulong(c)] = v;
    }
    dst[oidx * 4u + 3u] = 1.0f;
}
