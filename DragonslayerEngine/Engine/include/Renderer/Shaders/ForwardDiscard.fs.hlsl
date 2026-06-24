// © 2021 NVIDIA Corporation

#include "NRI.hlsl"

struct Attributes
{
    float4 Position : SV_Position;
    float3 Normal : TEXCOORD0;
};

float4 main( in Attributes input ) : SV_Target
{
    return float4(1., 0., 0., 1.);
}
