#pragma once
class DefaultShader
{
	friend class Renderer;
	friend class RendererDX11;
	friend class RendererDX12;
protected:
	//Default Shader
	const char* m_defaultShaderSource = R"(
	cbuffer CameraConstants : register(b2)
	{
		float4x4 WorldToCameraTransform;
		float4x4 CameraToRenderTransform;
		float4x4 RenderToClipTransform;
	};
	
	cbuffer ModelConstants : register(b3)
	{
		float4x4 ModelToWorldTransform;
		float4 ModelColor;
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

	Texture2D diffuseTexture : register(t0);

	SamplerState diffuseSampler : register(s0);
	
	v2p_t VertexMain(vs_input_t input)
	{
		float4 modelSpacePosition = float4(input.modelSpacePosition, 1);
		float4 worldSpacePosition = mul(ModelToWorldTransform,modelSpacePosition);
		float4 cameraSpacePosition = mul(WorldToCameraTransform, worldSpacePosition);
		float4 renderSpacePosition = mul(CameraToRenderTransform, cameraSpacePosition);
		float4 clipSpacePosition = mul(RenderToClipTransform, renderSpacePosition);
	
		v2p_t v2p;
		v2p.clipSpacePosition = clipSpacePosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
		float4 vertexColor = input.color;
		float4 color = textureColor * vertexColor * ModelColor;
		clip(color.a - 0.01f);
		return float4(color);
	}


	)";

	const char* m_defaultShaderDX11FullScreenCopySource = R"(
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

	cbuffer PerFrameConstants : register(b1)
	{
		float c_time;
		int c_debugInt;
		float c_debugFloat;
		float PADDING;	
	};

	Texture2D colorTexture : register(t0);
	Texture2D depthTexture : register(t1);

	SamplerState colorSampler : register(s0);

	float LinearizeDepth(float depth, float nearZ, float farZ)
	{
		float linearDepth = (farZ * nearZ) / (farZ - depth * (farZ - nearZ));
		return saturate((linearDepth - nearZ) / (farZ - nearZ));
	}
	
	v2p_t VertexMain(vs_input_t input)
	{
		float4 clipSpacePosition = float4(input.modelSpacePosition, 1);
	
		v2p_t v2p;
		v2p.clipSpacePosition = clipSpacePosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		float4 textureColor = colorTexture.Sample(colorSampler, input.uv);
		float4 vertexColor = input.color;
		float4 color = textureColor * vertexColor;

		
		if(c_debugInt == 1)
		{
			float depth =  depthTexture.Sample(colorSampler, input.uv).r;
			depth = 1.0 - LinearizeDepth(depth, 0.01, 500.0);
			color = float4(depth,depth,depth,1.0);
		}

		clip(color.a - 0.01f);
		return float4(color);
	}


	)";

	//Default Shader DX12 (Bindless texturing)
	const char* m_defaultShaderDX12Source = R"(
	cbuffer CameraConstants : register(b2)
	{
		float4x4 WorldToCameraTransform;
		float4x4 CameraToRenderTransform;
		float4x4 RenderToClipTransform;
	};
	
	cbuffer ModelConstants : register(b3)
	{
		float4x4 ModelToWorldTransform;
		float4 ModelColor;
	};
		
	#define NUM_TEXTURE_SLOTS 16
	#define MAX_NUM_BINDLESS_TEXTURES 4096

	cbuffer TextureConstants : register(b5)
	{
		uint4 textureIndexes[NUM_TEXTURE_SLOTS / 4];
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

	Texture2D textures[MAX_NUM_BINDLESS_TEXTURES] : register(t0, space0);

	SamplerState diffuseSampler : register(s0);

	void GetTextureGroupAndOffset(in uint textureIndex, out uint out_group, out uint out_offset)
	{	
		out_group = textureIndex / 4;
		out_offset = textureIndex % 4;
	}
	
	v2p_t VertexMain(vs_input_t input)
	{
		float4 modelSpacePosition = float4(input.modelSpacePosition, 1);
		float4 worldSpacePosition = mul(ModelToWorldTransform,modelSpacePosition);
		float4 cameraSpacePosition = mul(WorldToCameraTransform, worldSpacePosition);
		float4 renderSpacePosition = mul(CameraToRenderTransform, cameraSpacePosition);
		float4 clipSpacePosition = mul(RenderToClipTransform, renderSpacePosition);
	
		v2p_t v2p;
		v2p.clipSpacePosition = clipSpacePosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		uint group;
		uint offset;
		uint textureIndex = 0;
		GetTextureGroupAndOffset(textureIndex, group, offset);

		float4 textureColor = textures[textureIndexes[group][offset]].Sample(diffuseSampler, input.uv);
		float4 vertexColor = input.color;
		float4 color = textureColor * ModelColor * vertexColor;
		clip(color.a - 0.01f);
		return float4(color);
	}


	)";

	//Default Shader DX12 (Bindless texturing)
	const char* m_defaultShaderDX12FullScreenCopySource = R"(
	
	#define NUM_TEXTURE_SLOTS 16
	#define MAX_NUM_BINDLESS_TEXTURES 4096

	cbuffer TextureConstants : register(b5)
	{
		uint4 textureIndexes[NUM_TEXTURE_SLOTS / 4];
	};

	cbuffer PerFrameConstants : register(b1)
	{
		float c_time;
		int c_debugInt;
		float c_debugFloat;
		float PADDING;	
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

	Texture2D textures[MAX_NUM_BINDLESS_TEXTURES] : register(t0, space0);

	SamplerState diffuseSampler : register(s0);

	void GetTextureGroupAndOffset(in uint textureIndex, out uint out_group, out uint out_offset)
	{	
		out_group = textureIndex / 4;
		out_offset = textureIndex % 4;
	}

	float LinearizeDepth(float depth, float nearZ, float farZ)
	{
		float linearDepth = (farZ * nearZ) / (farZ - depth * (farZ - nearZ));
		return saturate((linearDepth - nearZ) / (farZ - nearZ));
	}
	
	v2p_t VertexMain(vs_input_t input)
	{
		float4 clipSpacePosition = float4(input.modelSpacePosition, 1);
	
		v2p_t v2p;
		v2p.clipSpacePosition = clipSpacePosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		uint group;
		uint offset;
		uint textureIndex = 0;
		GetTextureGroupAndOffset(textureIndex, group, offset);

		float4 textureColor = textures[textureIndexes[group][offset]].Sample(diffuseSampler, input.uv);

		float4 vertexColor = input.color;
		float4 color = textureColor;// * vertexColor;
		clip(color.a - 0.01f);
		return float4(color);
	}

	)";

//Default Shader DX12 (Bindless texturing)
const char* m_defaultMipComputeShaderDX12 = R"(

cbuffer MipGenConstants : register(b0)
{
    uint destinationMipWidth;
    uint destinationMipHeight;
    uint sourceMipWidth;
    uint sourceMipHeight;
};

Texture2D<float4> SourceMip : register(t0);
RWTexture2D<float4> DestinationMip : register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 destinationPixel = dispatchThreadId.xy;

    if ((destinationPixel.x >= destinationMipWidth) || (destinationPixel.y >= destinationMipHeight))
    {
        return;
    }

    uint2 sourcePixel00 = (destinationPixel * 2);

    uint sourceX0 = sourcePixel00.x;
    uint sourceY0 = sourcePixel00.y;

    uint sourceX1 = min(sourceX0 + 1, sourceMipWidth - 1);
    uint sourceY1 = min(sourceY0 + 1, sourceMipHeight - 1);

    float4 sample00 = SourceMip.Load(int3(sourceX0, sourceY0, 0));
    float4 sample10 = SourceMip.Load(int3(sourceX1, sourceY0, 0));
    float4 sample01 = SourceMip.Load(int3(sourceX0, sourceY1, 0));
    float4 sample11 = SourceMip.Load(int3(sourceX1, sourceY1, 0));

    float4 averagedColor = (sample00 + sample10 + sample01 + sample11) * 0.25f;
    DestinationMip[destinationPixel] = averagedColor;
}
)";
	
};

