#include "NRI.hlsl"

struct Attributes
{
    float4 Position   : SV_Position;
    float3 Normal     : TEXCOORD0;
    float4 PositionWS : TEXCOORD1;
    float3 PositionLS : TEXCOORD2;
    float4 Color      : TEXCOORD3;
};

NRI_RESOURCE(cbuffer, Global, b, 0, 0)
{
    row_major float4x4 gViewToClip;
    row_major float4x4 gLightViewToClip;
    float3 gCameraPos;
    float gPadding;
};

float WorldGridMask(float3 position, float3 normal, float cellSize, float lineWidth)
{
    float3 coord = position / cellSize;
    float3 d = abs(frac(coord - 0.5) - 0.5);
    float3 fw = max(fwidth(coord) * lineWidth, 1e-5);
    float3 line3 = 1.0 - smoothstep(float3(0,0,0), fw, d);

    float3 w = abs(normalize(normal));
    float gx = max(line3.y, line3.z) * w.x;
    float gy = max(line3.x, line3.z) * w.y;
    float gz = max(line3.x, line3.y) * w.z;
    return saturate(gx + gy + gz);
}

float LayeredWorldGridMask(float3 position, float3 normal)
{
    return max(WorldGridMask(position, normal, 4., 2.) * .3, WorldGridMask(position, normal, 1., 1.5) * .1);
}

float4 main(in Attributes input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    float3 lightDir = float3(-.6, -1., -.6);

    float3 shadingNormal = isFrontFace ? input.Normal : -input.Normal;
    float diff = dot(shadingNormal, normalize(-lightDir)) * .5;

    float3 color = (input.Color * diff + input.Color * .5).xyz;
    color = lerp(color, float3(0.0, 0.0, 0.0), LayeredWorldGridMask(input.PositionLS, input.Normal));

    // Gamma correction
    color = pow(abs(color), .4545);

    return float4(color, input.Color.w);
}
