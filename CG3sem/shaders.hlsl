cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4 gColor;
};

struct LightData
{
    float4 PositionRange;
    float4 ColorIntensity;
    float4 DirectionType;
    float4 SpotAngles;
};

cbuffer cbPass : register(b1)
{
    float4x4 gViewProj;
    float4 gLightDir;
    float4 gEyePosW;

    float gAmbientStrength;
    float gSpecularStrength;
    float gSpecularPower;
    float gTime;

    uint gLightCount;
    uint3 gLightPadding;

    LightData gLights[16];
};

cbuffer cbMaterial : register(b2)
{
    float4 gMaterialDiffuse;
};

Texture2D gDiffuseTexture : register(t0);
SamplerState gDiffuseSampler : register(s0);

struct VSInput
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct PSInput
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION0;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD0;
};

PSInput VS(VSInput vin)
{
    PSInput vout;

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);

    vout.NormalW = normalize(mul(float4(vin.NormalL, 0.0f), gWorld).xyz);

    vout.TexC = vin.TexC;

    return vout;
}

struct GBufferOutput
{
    float4 AlbedoSpecular : SV_Target0;
    float4 WorldPosition : SV_Target1;
    float4 Normal : SV_Target2;
};

GBufferOutput PS(PSInput pin)
{
    const float amplitude = 0.1f;
    const float period = 2.0f;
    float offsetY = amplitude * sin(gTime * 6.2831853f / period);
    float2 animatedUV = pin.TexC + float2(0.0f, offsetY);
    float4 textureColor = gDiffuseTexture.Sample(gDiffuseSampler, animatedUV);
    float4 surfaceColor = textureColor * gMaterialDiffuse * gColor;
    GBufferOutput output;

    output.AlbedoSpecular = float4(surfaceColor.rgb, saturate(gSpecularStrength));
    output.WorldPosition = float4(pin.PosW, 1.0f);
    output.Normal = float4(normalize(pin.NormalW), 0.0f);

    return output;
}