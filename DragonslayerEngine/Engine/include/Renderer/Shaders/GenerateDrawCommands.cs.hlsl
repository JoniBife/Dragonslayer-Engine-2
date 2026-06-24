// GPU draw-generation compute pass.
// One thread per instance. Filters by shader bucket + flag mask, then writes a
// DrawIndexedDesc into the indirect-args buffer at the appropriate slot.
// Dispatched once per pass (shadow / opaque / transparent) with different filter constants.

#define NRI_USE_BYTE_ADDRESS_BUFFER
// Must match the graphics pipeline layout's ENABLE_DRAW_PARAMETERS_EMULATION flag — with this
// define active, NRI_FILL_DRAW_INDEXED_DESC writes a DrawIndexedBaseDesc (7 uints / 28 bytes)
// instead of DrawIndexedDesc (5 uints / 20 bytes), routing baseInstance through a per-draw
// root-constant slot the runtime emulates.
#define NRI_ENABLE_DRAW_PARAMETERS_EMULATION
#include "NRI.hlsl"

// Must mirror the C++ MeshDesc in RendererCore.hpp (16 bytes).
struct MeshDesc
{
    uint firstIndex;
    uint indexCount;
    int  baseVertex;
    uint padding;
};

// Must mirror the C++ Instance in RendererCore.hpp (160 bytes).
struct Instance
{
    row_major float4x4 modelMatrix;
    row_major float4x4 normalMatrix;
    float4 color;
    uint meshID;
    uint materialID;
    uint shaderID;
    uint flags;
};

struct BuildDrawsConstants
{
    uint instanceCount;       // size of gInstances this frame
    uint passShaderID;        // bucket to populate (Opaque/Transparent/Shadow)
    uint passFlagMask;        // optional flag filter (0 = accept any)
    uint maxDrawsPerBucket;   // upper bound on draws written into one bucket
};
NRI_ROOT_CONSTANTS(BuildDrawsConstants, gConsts, 0, 0);

NRI_RESOURCE(StructuredBuffer<Instance>, gInstances, t, 0, 0);
NRI_RESOURCE(StructuredBuffer<MeshDesc>, gMeshDescs, t, 1, 0);
NRI_RESOURCE(RWByteAddressBuffer, gIndirectArgs, u, 0, 0);
NRI_RESOURCE(RWStructuredBuffer<uint>, gDrawCount, u, 1, 0);

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    const uint gid = dtid.x;
    if (gid >= gConsts.instanceCount)
        return;

    Instance inst = gInstances[gid];

    // Bucket filter: each dispatch handles exactly one shader bucket.
    if (inst.shaderID != gConsts.passShaderID)
    {
        // For the shadow pass we want to draw opaque + transparent that cast shadows, regardless
        // of the instance's "main pass" bucket. Handle that case via the flag mask: if it's set,
        // ignore the shaderID filter and rely on the flag check instead.
        if (gConsts.passFlagMask == 0)
            return;
    }

    if (gConsts.passFlagMask != 0 && (inst.flags & gConsts.passFlagMask) == 0)
        return;

    MeshDesc mesh = gMeshDescs[inst.meshID];

    uint slot;
    InterlockedAdd(gDrawCount[gConsts.passShaderID], 1u, slot);
    if (slot >= gConsts.maxDrawsPerBucket)
        return; // overflow — silently drop. CPU-side MAX_DRAWS_PER_PSO should be sized large enough.

    // Linear command index within the shared indirect-args buffer.
    const uint cmdIndex = gConsts.passShaderID * gConsts.maxDrawsPerBucket + slot;

    // DrawIndexedDesc layout (5 uints) — matches nri::DrawIndexedDesc.
    // baseInstance = gid carries the instance index to the VS via SV_StartInstanceLocation.
    NRI_FILL_DRAW_INDEXED_DESC(gIndirectArgs, cmdIndex,
        mesh.indexCount,
        1u,
        mesh.firstIndex,
        mesh.baseVertex,
        gid);
}
