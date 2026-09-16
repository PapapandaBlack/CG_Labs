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
    float gPadding;
};

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
};

PSInput VS(VSInput vin)
{
    PSInput vout;
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
    vout.NormalW = normalize(mul(float4(vin.NormalL, 0.0f), gWorld).xyz);
    return vout;
}

float4 PS(PSInput pin) : SV_Target
{
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

    float3 finalColor = gColor.rgb * (ambient + diffuse) + specular.xxx;
    finalColor = saturate(finalColor);

    return float4(finalColor, gColor.a);
}