	
	cbuffer PerFrameConstants : register(b1)
	{
		float c_time;
		int c_debugInt;
		float c_debugFloat;
		float PADDING;	
	};

	cbuffer CameraConstants : register(b2)
	{
		float4x4 WorldToCameraTransform;
		float4x4 CameraToRenderTransform;
		float4x4 RenderToClipTransform;
		float3 CameraPosition;
	};
	
	cbuffer ModelConstants : register(b3)
	{
		float4x4 ModelToWorldTransform;
		float4 ModelColor;
	};

	cbuffer WorldConstants : register(b4)
	{
		float4 IndoorLightColor;
		float4 OutdoorLightColor;
		float4 SkyColor;
		float FogNearDistance;
		float FogFarDistance;
		int UnderWater;
		float DayTime;

	};

	struct vs_input_t
	{
		float3 modelSpacePosition : POSITION;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
	};
	
	struct v2p_t
	{
		float4 clipSpacePosition : SV_Position;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
	};

	#define NEAR_PLANE 0.1
	#define FAR_PLANE 1000.0
	#define SPRITE_DIMS_XY 8
	#define UNDERWATER_FOG_DEPTH 5


	Texture2D colorTexture : register(t0);
	Texture2D depthTexture : register(t1);
	Texture2D waterDepthTexture : register(t2);

	SamplerState colorSampler : register(s0);

	float LinearizeDepth(float depth, float nearZ, float farZ)
	{
		float linearDepth = (farZ * nearZ) / (farZ - depth * (farZ - nearZ));
		return saturate((linearDepth - nearZ) / (farZ - nearZ));
	}

	
	v2p_t VertexMain(vs_input_t input)
	{
		float4 clipSpacePosition = float4(input.modelSpacePosition.xy, 0.0, 1);
	
		v2p_t v2p;
		v2p.clipSpacePosition = clipSpacePosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{

		float2 uv = input.uv;
		
		float rawTerrainDepth = depthTexture.Sample(colorSampler, uv).r;
		float terrainDepth = LinearizeDepth(rawTerrainDepth, NEAR_PLANE, FAR_PLANE);

		float rawWaterDepth = waterDepthTexture.Sample(colorSampler, uv).r;
		float waterDepth = LinearizeDepth(rawWaterDepth, NEAR_PLANE, FAR_PLANE);
		

		//----------------------------------------------------
		// Fog from thickness
		//----------------------------------------------------
		float fogStart = 0.0f;     // near surface = less fog
		float fogEnd   = 0.2f;     // deeper = full fog

		float depthToUse = min(terrainDepth, waterDepth);

		float fogFactor = saturate((depthToUse - fogStart) / (fogEnd - fogStart));
		fogFactor *= fogFactor; 

		//----------------------------------------------------
		// Fog color with time-of-day tint
		//----------------------------------------------------
		float3 baseWaterColor = float3(0.0, 0.28, 0.42);
		float outdoorBrightness = dot(OutdoorLightColor.rgb, float3(0.299, 0.587, 0.114));
		float darkness = 1.0f - outdoorBrightness;
		float3 fogColor = baseWaterColor * lerp(1.0f, 0.35f, darkness);

		float3 sceneColor = colorTexture.Sample(colorSampler, uv).rgb;

		//----------------------------------------------------
		// Final fog mix
		//----------------------------------------------------
		float3 finalColor = lerp(sceneColor, fogColor, fogFactor);
		return float4(finalColor, 1.0);
	}
