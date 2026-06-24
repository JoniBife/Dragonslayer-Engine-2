// Bindless forward VS. Reads per-instance data from a StructuredBuffer indexed by
// SV_StartInstanceLocation (NRI_BASE_INSTANCE). The pipeline layout has
// PipelineLayoutBits::ENABLE_DRAW_PARAMETERS_EMULATION set, and we opt into the
// matching shader-side emulation so we work on SM 6.6/6.7 without needing SM 6.8.
#define NRI_ENABLE_DRAW_PARAMETERS_EMULATION
#include "NRI.hlsl"

struct Input
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct Attributes
{
    float4 Position   : SV_Position;
    float3 Normal     : TEXCOORD0;
    float4 PositionWS : TEXCOORD1;
    float3 PositionLS : TEXCOORD2;
    float4 Color      : TEXCOORD3;
};

#ifndef NRI_DXBC

// Must mirror C++ `Instance` in RendererCore.hpp (160 bytes).
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

NRI_RESOURCE(cbuffer, Global, b, 0, 0)
{
    row_major float4x4 gViewToClip;
    row_major float4x4 gLightViewToClip;
    float3 gCameraPos;
    float gPadding;
};

NRI_RESOURCE(StructuredBuffer<Instance>, gInstances, t, 0, 0);

NRI_ENABLE_DRAW_PARAMETERS;

float3 GetMatrixScale(float4x4 m)
{
    return float3(length(m[0].xyz), length(m[1].xyz), length(m[2].xyz));
}

Attributes main(in Input input, NRI_DECLARE_DRAW_PARAMETERS)
{
    Attributes output;

    // Use NRI_INSTANCE_ID_OFFSET = baseInstance + SV_InstanceID so the shader works equally
    // well for non-instanced draws (one CmdDrawIndexed per instance) and CPU-instanced draws
    // (one CmdDrawIndexed per mesh, instanceCount = N, firstInstance = run start).
    const Instance inst = gInstances[NRI_INSTANCE_ID_OFFSET];

    output.PositionWS = mul(inst.modelMatrix, float4(input.Position, 1));
    output.PositionLS = GetMatrixScale(inst.modelMatrix) * input.Position;
    output.Position = mul(gViewToClip, output.PositionWS);
    output.Normal = normalize(mul(inst.normalMatrix, float4(input.Normal, 1)).xyz);
    output.Color = inst.color;

    return output;
}

#else

// DXBC (SM 5.x) has no structured-buffer indexing via SV_StartInstanceLocation in this layout.
// Build skips the bindless body so .dxbc still compiles; D3D11 path is unused for this renderer.
Attributes main(in Input input)
{
    Attributes output = (Attributes)0;
    return output;
}

#endif
