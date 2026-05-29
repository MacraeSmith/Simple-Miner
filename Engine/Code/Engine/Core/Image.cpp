#include "Engine/Core/Image.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/Vec4.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"


Image::Image(char const* imageFilePath)
	:m_imageFilePath(imageFilePath)
{
	int bytesPerTexel = 0;

	stbi_set_flip_vertically_on_load(1); //sets uv coords (0,0) to bottom left
	unsigned char* texelData = stbi_load(imageFilePath, &m_dimensions.x, &m_dimensions.y, &bytesPerTexel, 0);

	GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath));
	//GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", imageFilePath));
	GUARANTEE_OR_DIE(m_dimensions.x > 0 && m_dimensions.y > 0, Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", imageFilePath, m_dimensions.x, m_dimensions.y));
	GUARANTEE_OR_DIE(bytesPerTexel <= 4, Stringf("Image: \"%s\" had \"%i\" bytes per texel", imageFilePath, bytesPerTexel));

	int numTexels = m_dimensions.x * m_dimensions.y;
	m_rgbaTexels.resize(numTexels);

	int byteIndex = 0;
	for (int texelNum = 0; texelNum < numTexels; ++texelNum)
	{

		if (bytesPerTexel == 1) // grayscale
		{
			unsigned char gray = texelData[byteIndex++];
			m_rgbaTexels[texelNum].r = gray;
			m_rgbaTexels[texelNum].g = gray;
			m_rgbaTexels[texelNum].b = gray;
			m_rgbaTexels[texelNum].a = 255;
		}
		else if (bytesPerTexel == 3) // RGB
		{
			m_rgbaTexels[texelNum].r = texelData[byteIndex++];
			m_rgbaTexels[texelNum].g = texelData[byteIndex++];
			m_rgbaTexels[texelNum].b = texelData[byteIndex++];
			m_rgbaTexels[texelNum].a = 255; // opaque
		}
		else if (bytesPerTexel == 4) // RGBA
		{
			m_rgbaTexels[texelNum].r = texelData[byteIndex++];
			m_rgbaTexels[texelNum].g = texelData[byteIndex++];
			m_rgbaTexels[texelNum].b = texelData[byteIndex++];
			m_rgbaTexels[texelNum].a = texelData[byteIndex++];
		}
		else
		{
			GUARANTEE_OR_DIE(false, Stringf("Unsupported channel count %i in image \"%s\"", bytesPerTexel, imageFilePath));
		}

	}
	
	stbi_image_free(texelData);
}

Image::Image(IntVec2 size, Rgba8 color, char const* name)
	:m_dimensions(size)
	,m_imageFilePath(name)
{
	int numTexels = m_dimensions.x * m_dimensions.y;
	m_rgbaTexels.resize(numTexels);

	for (int texelNum = 0; texelNum < numTexels; ++texelNum)
	{
		m_rgbaTexels[texelNum].r = color.r;
		m_rgbaTexels[texelNum].g = color.g;
		m_rgbaTexels[texelNum].b = color.b;
		m_rgbaTexels[texelNum].a = color.a;
	}
}

Image::~Image()
{
}

std::string const& Image::GetImageFilePath() const
{
	return m_imageFilePath;
}

const void* Image::GetRawData() const
{
	return m_rgbaTexels.data();
}

IntVec2 Image::GetDimensions() const
{
	return m_dimensions;
}

Rgba8 Image::GetTexelColor(IntVec2 const& texelCoords) const
{
	int index = (texelCoords.y * m_dimensions.x) + texelCoords.x;
	return m_rgbaTexels[index];
}

void Image::SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor)
{
	int index = (texelCoords.y * m_dimensions.x) + texelCoords.x;
	m_rgbaTexels[index] = newColor;
}
