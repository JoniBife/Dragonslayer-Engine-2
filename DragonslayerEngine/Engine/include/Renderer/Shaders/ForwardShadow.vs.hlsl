// Bindless shadow VS — same instance-fetch pattern as Forward.vs but projects with
// gLightViewToClip and discards normal/color outputs.
#define NRI_ENABLE_DRAW_PARAMETERS_EMULATION
#include "NRI.hlsl"

struct Input
{
    float3 Position : POSITION;
};

struct Attributes
{
    float4 Position : SV_Position;
};

#ifndef NRI_DXBC

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

Attributes main(in Input input, NRI_DECLARE_DRAW_PARAMETERS)
{
    Attributes output;
    const Instance inst = gInstances[NRI_INSTANCE_ID_OFFSET];
    output.Position = mul(mul(gLightViewToClip, inst.modelMatrix), float4(input.Position, 1));
    return output;
}

#else

Attributes main(in Input input)
{
    Attributes output = (Attributes)0;
    return output;
}

#endif
