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
		float Padding;

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
		float3 worldPosition : POSITION;
		int isTouchingWater : IS_TOUCHING_WATER;
	};


	Texture2D diffuseTexture : register(t0);

	SamplerState diffuseSampler : register(s0);

	float3 DiminishingAddColor(float3 normalizedColorA, float3 normalizedColorB)
	{
		float3 colorOne = float3(1.0,1.0,1.0);
		return(colorOne - ((colorOne - normalizedColorA) * (colorOne - normalizedColorB)));
	}

	float GetFractionWithinRange(float inValue, float inStart, float inEnd)
	{
		return (inValue - inStart) / (inEnd - inStart);
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
		float4 vertexColor = float4(DiminishingAddColor(outdoorColor, indoorColor), 1.0f);
	
		int isTouchingWater = 0;
		if(input.color.a > 0.f)
		{	
			isTouchingWater = 1;
		}
		
	
		v2p_t v2p;
		v2p.clipSpacePosition = clipSpacePosition;
		v2p.color = vertexColor;
		v2p.uv = input.uv;
		v2p.worldPosition = worldSpacePosition.xyz;
		v2p.isTouchingWater = isTouchingWater;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
		if(textureColor.a < 0.5) discard;
		float4 vertexColor = input.color;
		float4 color = textureColor * vertexColor * ModelColor;

		float distToCamera = distance(CameraPosition, input.worldPosition);
		float fogFraction = saturate(GetFractionWithinRange(distToCamera, FogNearDistance, FogFarDistance));
		float3 fogColor = SkyColor.rgb;
		if(UnderWater == 1 && input.isTouchingWater == 1)
		{ 
			float3 shallowBlue = float3(0.35f, 0.75f, 0.95f);
			float3 deepBlue = float3(0.0f, 0.25f, 0.45f);
			color.rgb *= lerp(shallowBlue, deepBlue, fogFraction);
		}
		color.rgb = lerp(color.rgb, fogColor.rgb, fogFraction * SkyColor.a);
	
		clip(color.a - 0.01f);
		return float4(color);
	}
