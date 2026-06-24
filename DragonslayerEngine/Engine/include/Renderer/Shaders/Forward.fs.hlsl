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

NRI_RESOURCE(SamplerState, gLinearClamp, s, 0, 1);
NRI_RESOURCE(Texture2D<float>, gShadowMap, t, 0, 1);

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

#define USE_PCF

float SampleShadowMap(float4 positionWS, float3 normal) {

    float4 positionLS = mul(gLightViewToClip, positionWS);

    float3 shadowMapUV = positionLS.xyz;
    shadowMapUV.y = -positionLS.y*.5 + .5;
    shadowMapUV.x =  positionLS.x*.5 + .5;

    float currentPixelDepth = shadowMapUV.z;

    if (currentPixelDepth > 1.) {
        return 0.;
    }

    float3 lightDir = float3(-.6, -1., -.6);

    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    bias = 0.001f;

#ifndef USE_PCF

    float depthInShadowMap = gShadowMap.Sample(gLinearClamp, shadowMapUV.xy).r;
    if (currentPixelDepth - bias > depthInShadowMap) {
        return 1.;
    }

    return 0.;

#else
    float shadow = 0.0;
    float texelSize = 1.0 / 4096;

    [unroll(9)]
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = gShadowMap.SampleLevel(gLinearClamp, shadowMapUV.xy + float2(x, y) * texelSize, 0).r;
            shadow += currentPixelDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.;

    return shadow;
#endif
}

[earlydepthstencil]
float4 main(in Attributes input) : SV_Target
{
    float3 lightDir = float3(-.6, -1., -.6);

    float diff = dot(input.Normal, normalize(-lightDir)) * .5;

    float shadow = SampleShadowMap(input.PositionWS, input.Normal);

    float3 color = (input.Color * diff + input.Color * .5).xyz * (1. - shadow * .5);

    color = lerp(color, float3(0.0, 0.0, 0.0), LayeredWorldGridMask(input.PositionLS, input.Normal));

    // Gamma correction
    color = pow(abs(color), .4545);

    return float4(color, 1.);
}
