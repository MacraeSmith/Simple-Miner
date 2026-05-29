//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 modelTangent : TANGENT;
	float3 modelBitangent : BITANGENT;
	float3 modelNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 clipPosition : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 worldTangent : TANGENT;
	float4 worldBitangent : BITANGENT;
	float4 worldNormal : NORMAL;
	float4 worldPosition : POSITION;
};


struct PointLight
{
    float3 Position;
    float Padding;
    float4 Color;
};

#define MAX_POINT_LIGHTS 64

//------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b1)
{
	float3 SunDirection;
	float SunIntensity;
	float AmbientIntensity;
    int NumPointLights;
	float2 Padding0;
	
    PointLight PointLights[ MAX_POINT_LIGHTS];
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 WorldToCameraTransform;	// View transform
	float4x4 CameraToRenderTransform;	// Non-standard transform from game to DirectX conventions
	float4x4 RenderToClipTransform;		// Projection transform
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelToWorldTransform;		// Model transform
	float4 ModelColor;
};

cbuffer ColorAdjustmentConstants : register(b4)
{
	float Saturation;
	float Brightness;
	float Inversion;
	float HueStrength;
	float4 HueColor;
};

float4 AddHue(float4 color)
{
	float3 hueColor = lerp(color.rgb, HueColor.rgb, HueStrength);
	hueColor += Brightness;
	return float4(hueColor.rgb, color.a);
};


float4 InvertColor(float4 color)
{
	float3 invertedColor = float3(1.f - color.rgb);
	return float4(lerp(color.rgb, invertedColor.rgb, clamp(Inversion, 0.f, 1.f)), color.a);
};


float4 DesaturateColor(float4 color)
{
	float grayScale = dot(color.rgb, float3(0.299, 0.587, 0.114));
	float3 grayColor = float3(grayScale, grayScale, grayScale);
	return float4(lerp(grayColor.rgb, color.rgb, clamp(Saturation, 0.f, 1.f)), color.a);
};



//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);

//------------------------------------------------------------------------------------------------
SamplerState samplerState : register(s0);

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 modelPosition = float4(input.modelPosition, 1);
	float4 worldPosition = mul(ModelToWorldTransform, modelPosition);
	float4 cameraPosition = mul(WorldToCameraTransform, worldPosition);
	float4 renderPosition = mul(CameraToRenderTransform, cameraPosition);
	float4 clipPosition = mul(RenderToClipTransform, renderPosition);

	float4 worldTangent = mul(ModelToWorldTransform, float4(input.modelTangent, 0.0f));
	float4 worldBitangent = mul(ModelToWorldTransform, float4(input.modelBitangent, 0.0f));
	float4 worldNormal = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));

	v2p_t v2p;
	v2p.clipPosition = clipPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.worldTangent = worldTangent;
	v2p.worldBitangent = worldBitangent;
	v2p.worldNormal = worldNormal;
	v2p.worldPosition = worldPosition;
	return v2p;
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 textureColor = diffuseTexture.Sample(samplerState, input.uv);
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;
	
    float4 ambientColor =  float4(AmbientIntensity * float3(1.0f, 1.0f, 1.0f), 1.0f);
    float4 directionalColor = SunIntensity * saturate(clamp(dot(normalize(input.worldNormal.xyz), -SunDirection),0.f, 1.f)) * float4(1.0f, 1.0f, 1.0f, 1.0f);
	float4 lightColor = ambientColor + directionalColor;
	
    for (int i = 0; i < NumPointLights; ++i)
    {
        float3 pixelToLight = PointLights[i].Position - input.worldPosition.xyz;
        float distance = length(pixelToLight);
        float attenuation = 1.f / ( .25f + 0.7f * distance + .8f * distance * distance);
		//float attenuation = 1.f / (distance * distance);
        pixelToLight = normalize(pixelToLight);
        lightColor += saturate( PointLights[i].Color * attenuation * clamp(dot(normalize(input.worldNormal.xyz), pixelToLight), 0.f, 1.f));
    }
	
    float4 color = lightColor * textureColor * vertexColor * modelColor;
	color.a = clamp(color.a, 0.f, 1.f);
	
	color = InvertColor(color);
	color = AddHue(color);
	color = DesaturateColor(color);
	clip(color.a - 0.01f);

	return color;
}
