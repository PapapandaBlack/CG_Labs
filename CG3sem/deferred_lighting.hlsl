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

    LightData
    gLights[16];
};

Texture2D<float4> gAlbedoSpecular : register(t0);
Texture2D<float4> gWorldPosition : register(t1);
Texture2D<float4> gNormal : register(t2);

struct ScreenInput
{
    float4 PosH : SV_POSITION;
};

float3 SafeNormalize(float3 value)
{
    return value * rsqrt(max(dot(value, value), 0.000001f));
}

ScreenInput VS(uint vertexID : SV_VertexID)
{
    float2 positions[3] = {float2(-1.0f, -1.0f), float2(-1.0f, 3.0f), float2(3.0f, -1.0f)};

    ScreenInput output;
    output.PosH = float4(positions[vertexID], 0.0f, 1.0f);

    return output;
}

float3 EvaluateSurfaceLighting(float3 albedo, float specularStrength, float3 normal, float3 viewDirection, float3 lightDirection, float3 lightColor)
{
    float diffuseFactor = saturate(dot(normal, lightDirection));
    float specular = 0.0f;

    if (diffuseFactor > 0.0f)
    {
        float3 reflected = reflect(-lightDirection, normal);
        specular = pow(saturate(dot(reflected, viewDirection)), max(gSpecularPower, 1.0f)) * specularStrength;
    }

    return (albedo * diffuseFactor + specular.xxx) * lightColor;
}

float3 EvaluateLight(LightData light,float3 worldPosition, float3 albedo, float specularStrength, float3 normal, float3 viewDirection)
{
    uint lightType = (uint)light.DirectionType.w;
    float intensity = max(light.ColorIntensity.w, 0.0f);

    if (intensity <= 0.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 lightColor = light.ColorIntensity.rgb * intensity;
    float3 lightDirection;
    float attenuation = 1.0f;

    if (lightType == 0)
    {
        lightDirection = SafeNormalize(-light.DirectionType.xyz);
    }

    else if (lightType == 1 || lightType == 2)
    {
        float radius = light.PositionRange.w;

        if (radius <= 0.0f)
        {
            return float3(0.0f, 0.0f, 0.0f);
        }

        float3 toLight = light.PositionRange.xyz - worldPosition;
        float distanceSquared = dot(toLight, toLight);

        if (distanceSquared >= radius * radius)
        {
            return float3(0.0f, 0.0f, 0.0f);
        }

        float distanceToLight = sqrt(distanceSquared);
        lightDirection = SafeNormalize(toLight);

        attenuation = saturate(1.0f - distanceToLight / radius);
        attenuation *= attenuation;

        if (lightType == 2)
        {
            float3 spotDirection = SafeNormalize(light.DirectionType.xyz);
            
            float coneCos = dot(spotDirection, -lightDirection);
            float innerCos = light.SpotAngles.x;
            float outerCos = light.SpotAngles.y;
            
            float coneFactor = saturate((coneCos - outerCos) / max(innerCos - outerCos, 0.00001f));
            coneFactor = coneFactor * coneFactor * (3.0f - 2.0f * coneFactor);
            
            attenuation *= coneFactor;
        }
    }
    else
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    return EvaluateSurfaceLighting(albedo, specularStrength, normal, viewDirection, lightDirection, lightColor * attenuation);
}

float4 PS(ScreenInput pin) : SV_Target
{
    int2 pixel = int2(pin.PosH.xy);

    float4 worldPosition = gWorldPosition.Load(int3(pixel, 0));

    if (worldPosition.w < 0.5f)
    {
        return float4(0.15f, 0.18f, 0.22f, 1.0f);
    }

    float4 albedoSpecular = gAlbedoSpecular.Load(int3(pixel, 0));
    float3 albedo = albedoSpecular.rgb;
    float specularStrength = albedoSpecular.a;
    float3 normal = SafeNormalize(gNormal.Load(int3(pixel, 0)).xyz);
    float3 viewVector = gEyePosW.xyz - worldPosition.xyz;
    float3 viewDirection = SafeNormalize(viewVector);
    float3 finalColor = albedo * gAmbientStrength;

    uint lightCount = min(gLightCount, 16u);

    [loop]
    for (uint i = 0; i < lightCount; ++i)
    {
        finalColor += EvaluateLight(gLights[i], worldPosition.xyz, albedo, specularStrength, normal, viewDirection);
    }

    return float4(saturate(finalColor), 1.0f);
}