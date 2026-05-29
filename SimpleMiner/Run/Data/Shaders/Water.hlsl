	
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
		float2 Padding;

	};

	struct vs_input_t
	{
		float3 modelSpacePosition : POSITION;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
	};
	
	struct v2p_t
	{
		float4 position : SV_Position; 
		float4 clipSpacePosition : TEXCOORD0;
		float4 color : COLOR;
		float2 uv : TEXCOORD1;
		float3 worldPosition : POSITION;
	};

	#define NEAR_PLANE 0.1
	#define FAR_PLANE 1000.0
	#define SPRITE_DIMS_XY 8

	Texture2D diffuseTexture : register(t0);
	Texture2D sceneColorTexture : register(t1);
	Texture2D sceneDepthTexture : register(t2);

	SamplerState diffuseSampler : register(s0);
	SamplerState sceneSampler : register(s1);

	float3 DiminishingAddColor(float3 normalizedColorA, float3 normalizedColorB)
	{
		float3 colorOne = float3(1.0,1.0,1.0);
		return(colorOne - ((colorOne - normalizedColorA) * (colorOne - normalizedColorB)));
	}

	float LinearizeDepth(float depth, float nearZ, float farZ)
	{
		float linearDepth = (farZ * nearZ) / (farZ - depth * (farZ - nearZ));
		return saturate((linearDepth - nearZ) / (farZ - nearZ));
	}
	
	float GetFractionWithinRange(float inValue, float inStart, float inEnd)
	{
		return (inValue - inStart) / (inEnd - inStart);
	}

	float Hash(float n)
	{
		return frac(sin(n) * 43758.5453f);
	}

	// Smooth scalar value noise in 2D, returns [0,1]
	float Noise2D_Scalar(float2 p)
	{
		float2 i = floor(p);
		float2 f = frac(p);

		float a = Hash(dot(i, float2(1.0f, 57.0f)));
		float b = Hash(dot(i + float2(1.0f, 0.0f), float2(1.0f, 57.0f)));
		float c = Hash(dot(i + float2(0.0f, 1.0f), float2(1.0f, 57.0f)));
		float d = Hash(dot(i + float2(1.0f, 1.0f), float2(1.0f, 57.0f)));

		float2 u = f * f * (3.0f - 2.0f * f);

		float v1 = lerp(a, b, u.x);
		float v2 = lerp(c, d, u.x);
		float n = lerp(v1, v2, u.y);

		return n; // [0,1]
	}

	// Build a 2D distortion vector from two noise samples.
	float2 WaterDistortion(float3 worldPos, float time)
	{
		// Use the water plane (x,y) as base UVs
		float2 baseUV = worldPos.xy;

		// Two different frequencies and scroll directions
		float2 uv0 = baseUV * 0.12f + float2(time * 0.08f, time * 0.05f);
		float2 uv1 = baseUV * 0.20f + float2(-time * 0.06f, time * 0.04f);

		float n0 = Noise2D_Scalar(uv0);              // 0..1
		float n1 = Noise2D_Scalar(uv1);              // 0..1

		// Turn n0 into a direction angle
		float angle = n0 * 6.2831853f;               // 2*pi
		float2 dir = float2(cos(angle), sin(angle)); // unit vector

		// Magnitude from n1, remapped to [0,1], then biased
		float mag = (n1 * 2.0f - 1.0f);              // -1..1
		mag = abs(mag);                              // 0..1
		mag = lerp(0.25f, 1.0f, mag);                // 0.25..1

		return dir * mag; // distortion direction and strength in water plane
	}
	
	v2p_t VertexMain(vs_input_t input)
	{
		
		float4 modelSpacePosition = float4(input.modelSpacePosition, 1);
		float4 worldSpacePosition = mul(ModelToWorldTransform,modelSpacePosition);
		float4 cameraSpacePosition = mul(WorldToCameraTransform, worldSpacePosition);
		float4 renderSpacePosition = mul(CameraToRenderTransform, cameraSpacePosition);
		float4 clipSpacePosition = mul(RenderToClipTransform, renderSpacePosition);

		float3 outdoorColor = input.color.r * OutdoorLightColor.rgb * input.color.b;
		float3 indoorColor = input.color.g * IndoorLightColor.rgb * input.color.b;
		float4 vertexColor = float4(DiminishingAddColor(outdoorColor, indoorColor), input.color.a);
	
		v2p_t v2p;
		v2p.position = clipSpacePosition;
		v2p.clipSpacePosition = clipSpacePosition;
		v2p.color = vertexColor;
		v2p.uv = input.uv;
		v2p.worldPosition = worldSpacePosition.xyz;
		return v2p;
		
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		// NDC to screen UV
		float2 ndc = input.clipSpacePosition.xy / input.clipSpacePosition.w;
		float2 screenUV = ndc * 0.5f + 0.5f;
		screenUV.y = 1.0f - screenUV.y;

		// Scene depth
		float sceneDepthNonLinear = sceneDepthTexture.Sample(sceneSampler, screenUV).r;
		float sceneDepth = LinearizeDepth(sceneDepthNonLinear, NEAR_PLANE, FAR_PLANE);

		// Water depth
		float waterDepthNonLinear = input.clipSpacePosition.z / input.clipSpacePosition.w;
		float waterDepth = LinearizeDepth(waterDepthNonLinear, NEAR_PLANE, FAR_PLANE);

		// Thickness of water column in world units along the view ray
		float thickness = max(sceneDepth - waterDepth, 0.0f);

		// Noise-based refraction
		//----------------------------------------------------------------------
		// Distortion vector from noise
		float2 distortion = WaterDistortion(input.worldPosition, c_time * 3.5f);

		// Fresnel to soften harsh distortion
		float3 N = float3(0,0,1); 
		float3 V = normalize(CameraPosition - input.worldPosition);
		float fresnel = pow(1.0f - saturate(dot(N, V)), 3.0f);
		float refractionVisibility = 1.0f - fresnel;

		// Scale the distortion
		float baseRefractAmount = 0.25f;
		float t = 1.0f - exp(-thickness * 0.2f);
		float shallowBase = 0.025f;
		float thicknessFactor = shallowBase + (1.0f - shallowBase) * t;
		float deepCompression = 0.6f;
		thicknessFactor = shallowBase + (thicknessFactor - shallowBase) * deepCompression;

		float refractAmount = baseRefractAmount * thicknessFactor;

		// Apply Fresnel softening
		float2 refractUV = screenUV + distortion * refractAmount * refractionVisibility;
		refractUV = saturate(refractUV);
	
		//Limit refraction on objects near water surface so the "infront of" sections of the scene render texture do not get sampled
		//----------------------------------------------------------------------
		float sceneDepthRefrNL = sceneDepthTexture.Sample(sceneSampler, refractUV).r;
		float sceneDepthRefr = LinearizeDepth(sceneDepthRefrNL, NEAR_PLANE, FAR_PLANE);
		float preventLeak = saturate((sceneDepthRefr - waterDepth) * 5.0f);
		float2 safeRefractUV = lerp(screenUV, refractUV, preventLeak);

		// Sample the refracted scene
		float3 refractedScene = sceneColorTexture.Sample(sceneSampler, safeRefractUV).rgb;

		// Underwater haze (softens bright distorted pixels)
		//----------------------------------------------------------------------
		float haze = saturate(thickness * 0.15f);
		float3 hazeColor = float3(0.1f, 0.25f, 0.3f);
		refractedScene = lerp(refractedScene, hazeColor, haze * 0.65f);

		//Distort ocean surface for waves
		//----------------------------------------------------------------------
		float2 waterUV = input.uv + (distortion * 0.15f);
		float4 waterTex = diffuseTexture.Sample(diffuseSampler, waterUV);

		float3 waterColor = lerp(refractedScene, waterTex.rgb, 0.15f);	

		//add blue tint
		//----------------------------------------------------------------------
		float3 shallowBlue = float3(0.35f, 0.75f, 0.95f);
		float3 deepBlue = float3(0.0f, 0.25f, 0.45f); // tweak this
		float tintStrength = saturate(thickness); // thicker water = more blue
		float3 blueColor = lerp(shallowBlue , deepBlue, thickness);
		waterColor *= blueColor;

		//add lighting
		//----------------------------------------------------------------------
		float3 finalColor = waterColor * input.color.rgb;


		// Distance fog 
		//----------------------------------------------------------------------
		float distToCamera = distance(CameraPosition, input.worldPosition);
		float fogFraction = saturate(GetFractionWithinRange(distToCamera, FogNearDistance, FogFarDistance));

		finalColor = lerp(finalColor, SkyColor.rgb, fogFraction * SkyColor.a);	


		return float4(finalColor, 1.0f);

	}
