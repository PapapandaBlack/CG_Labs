cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4 gColor;
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

float4 PS(PSInput pin) : SV_Target
{
    const float amplitude = 0.1f;
    const float period = 2.0f;
    float offsetY = amplitude * sin(gTime * 6.2831853f / period);
    float2 animatedUV = pin.TexC + float2(0.0f, offsetY);

    float4 textureColor = gDiffuseTexture.Sample(gDiffuseSampler, animatedUV);

    float4 surfaceColor = textureColor * gMaterialDiffuse * gColor;

    float3 N = normalize(pin.NormalW);
    float3 L = normalize(-gLightDir.xyz);
    float3 V = normalize(gEyePosW.xyz - pin.PosW);
    float3 R = reflect(-L, N);

    float ambient = gAmbientStrength;
    float diffuse = max(dot(N, L), 0.0f);

    float specular = 0.0f;

    if (diffuse > 0.0f)
    {
        specular = pow(max(dot(R, V), 0.0f), gSpecularPower) * gSpecularStrength;
    }

    float3 finalColor = surfaceColor.rgb * (ambient + diffuse) + specular.xxx;

    finalColor = saturate(finalColor);

    return float4(finalColor, surfaceColor.a);
}