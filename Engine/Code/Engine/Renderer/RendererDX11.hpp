#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Game/EngineBuildPreferences.hpp"
#include <vector>

struct Vertex_PCU;
struct Vertex_PCUTBN;
class Window;
struct IntVec2;
class Texture;
class BitmapFont;
class Image;
class Material;

class Shader;
class VertexBuffer;
class ConstantBuffer;
class IndexBuffer;
struct ID3D11RasterizerState;
struct ID3D11RenderTargetView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11BlendState;
struct ID3D11SamplerState;
struct ID3DUserDefinedAnnotation;
struct ID3D11Texture2D;
struct ID3D11DepthStencilView;
struct ID3D11DepthStencilState;
struct ID3D11ShaderResourceView;

static const int k_defaultDiffuseSlot = 0;
static const int k_defaultNormalsSlot = 1;
static const int k_defaultSpecGlossEmitSlot = 2;


class RendererDX11 : public Renderer
{
public:
	RendererDX11(RendererConfig const& config);
	virtual ~RendererDX11() {};

	virtual void Startup() override;
	virtual void BeginFrame() override;
	virtual void EndFrame() override;
	virtual void Shutdown() override;

	virtual void ClearScreen(const Rgba8& clearColor) override;
	virtual void ClearDepth() override;
	virtual void BeginCamera(const Camera& camera) override;
	virtual void EndCamera(const Camera& camera) override;

	virtual void BeginRendererEvent(char const* eventName) const override;
	virtual void EndRendererEvent() const override;


	//Draw
	void		DrawVertexArray(int numVertexes, const Vertex_PCU* vertexes);
	void		DrawVertexArray(const std::vector<Vertex_PCU> verts);
	void		DrawVertexBuffer(VertexBuffer* vbo, unsigned int vertexCount);

	void		DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexedCount);
	void			DrawFullScreenQuad(Texture const* colorTexture, Texture const* depthTexture, Shader const* shader, DepthMode depthMode = DepthMode::DISABLED);


	//Creation
	virtual BitmapFont* CreateOrGetBitMapFontFromFile(char const* bitmapFontFilePathWithNoExtension) override;
	Texture*		CreateOrGetTextureFromFile(char const* imageFilePath);
	Texture*        CreateOrGetMipMapTextureFromFile(char const* imageFilePath, unsigned int numMipLevels);
	Shader*			CreateOrGetShaderFromFile(char const* shaderName, VertexType vertexType = VertexType::VERTEX_PCU);
	VertexBuffer*	CreateVertexBuffer(const unsigned int size, unsigned int stride);
	ConstantBuffer* CreateConstantBuffer(const unsigned int size);
	IndexBuffer*	CreateIndexBuffer(const unsigned int size);

	RenderTarget GetCopyOfCurrentRenderTarget();

	bool		CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name,
					char const* source, char const* entryPoint, char const* target);

	void		CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vbo);
	void		CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* cbo);
	void		CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* ibo);

	//Binds
	void			BindTexture(Texture const* texture, int slot = 0);
	void			BindShader(Shader const* shader);
	void			BindVertexBuffer(VertexBuffer* vbo);
	void			BindConstantBuffer(int slot, ConstantBuffer* cbo);
	void			BindIndexBuffer(IndexBuffer* ibo);

	virtual void	SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::WHITE) override;
	virtual void	SetLightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity, Rgba8 const& sunColor = Rgba8::WHITE) override;
	virtual void	SetLightConstants(LightConstants const& lightConstants) override;
	virtual void	SetColorAdjustmentConstants(ColorAdjustmentConstants const& colorAdjustmentConstants) override;
	virtual void	SetPerFrameConstants(PerFrameConstants const& perFrameConstants) override;
	Material*		GetDefaultMaterialByBlendMode(BlendMode blendMode);

	ID3D11Device*	GetDevice() const {return m_device;}
	ID3D11DeviceContext* GetDeviceContext() const {return m_deviceContext;}

private:

	//Textures
	Texture*		GetTextureForFileName(char const* imageFilePath) const;
	Texture*		CreateTextureFromFile(char const* imageFilePath);
	Texture*		CreateTextureFromImage(Image const& image);
	Texture*		CreateMipMappedCopy(Texture* texture, int numMipLevels);
	Texture*		CreateTextureWithMipMaps(Image const& image, int numMipLevels);
	Shader*			CreateShader(char const* shaderName, char const* shaderSource, VertexType vertexType = VertexType::VERTEX_PCU);
	Shader*			CreateShader(char const* shaderName, VertexType vertexType = VertexType::VERTEX_PCU);


	void			SetStatesIfChanged();

	//Start up Process
	void			CreateDeviceAndSwapChain();
	void			GetBackBufferTextureAndCreateRenderTargetView();
	RenderTarget	CreateRenderTarget(IntVec2 const& dimensions);
	Texture*		CreateRenderTexture(IntVec2 const& dimensions);
	Texture*		CreateDepthTexture(IntVec2 const& dimensions);
	RenderTarget    CreateCopyRenderTarget(IntVec2 const& dimensions);
	Texture*		CreateRenderTargetCopyColorTexture(IntVec2 const& dimensions);
	Texture*		CreateRenderTargetCopyDepthTexture(IntVec2 const& dimensions);
	
	void			CreateRasterizerStates();
	void			CreateBlendStates();
	void			CreateSamplerModes();
	void			CreateDepthStencil();
	void			CreateDefaultMaterials();

private:
	std::vector<Texture*> m_loadedTextures;
	Material* m_defaultMats[(int)BlendMode::COUNT] = {};

protected:
	//DX objects
	ID3D11RenderTargetView* m_backBufferRTV = nullptr;
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;
	IDXGISwapChain* m_swapChain = nullptr;

	//Shaders
	std::vector<Shader*> m_loadedShaders;
	Shader const* m_defaultShader = nullptr;
	Shader const* m_defaultScreenCopyShader = nullptr;
	Shader const* m_desiredShader = nullptr;
	Shader const* m_currentShader = nullptr;

	//Textures
	Texture const* m_defaultTexturesBySlot[NUM_TEXTURE_SLOTS] = {};
	Texture const* m_desiredTexturesBySlot[NUM_TEXTURE_SLOTS] = {};
	Texture const* m_currentTexturesBySlot[NUM_TEXTURE_SLOTS] = {};

	//Buffers
	VertexBuffer* m_immediateVBO = nullptr;
	ConstantBuffer* m_perFrameCBO = nullptr;
	ConstantBuffer* m_cameraCBO = nullptr;
	ConstantBuffer* m_modelCBO = nullptr;
	ConstantBuffer* m_lightCBO = nullptr;
	ConstantBuffer* m_colorAdjustmentCBO = nullptr;

	//BlendStates
	ID3D11BlendState* m_blendState = nullptr;
	ID3D11BlendState* m_blendStates[(int)BlendMode::COUNT] = {};

	//SamplerStates
	ID3D11SamplerState* m_samplerStateBySlot[NUM_TEXTURE_SLOTS] = {};
	ID3D11SamplerState* m_samplerStates[(int)SamplerMode::COUNT] = {};

	//RasterizerStates
	ID3D11RasterizerState* m_rasterizerState = nullptr;
	ID3D11RasterizerState* m_rasterizerStates[(int)RasterizerMode::COUNT] = {};

	//Renderer Events Annotations
	ID3DUserDefinedAnnotation* m_userDefinedAnnotations = nullptr;

	//DepthStates
	ID3D11Texture2D* m_depthStencilTexture = nullptr;
	ID3D11ShaderResourceView* m_depthStencilSRV = nullptr;
	ID3D11DepthStencilView* m_backBufferDSV = nullptr;
	ID3D11DepthStencilState* m_depthStencilState = nullptr;
	ID3D11DepthStencilState* m_depthStencilStates[(int)DepthMode::COUNT] = {};

};

