#include "Engine/Core/BufferUtilities.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane2D.hpp"
#include "Engine/Math/Plane3D.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/DevConsole.hpp"

BufferParser::BufferParser(unsigned char const* bufferToParse, size_t sizeInBytes, EndianMode endianMode)
	:m_bufferStart(bufferToParse)
	, m_currentReadPosition(0)
	, m_dataSize(sizeInBytes)
{
	SetEndianMode(endianMode);
}

BufferParser::BufferParser(std::vector<uint8_t> const& bufferToParse, EndianMode endianMode)
	:m_bufferStart(bufferToParse.data())
	,m_dataSize(bufferToParse.size())
	,m_currentReadPosition(0)
{
	SetEndianMode(endianMode);
}

size_t BufferParser::GetSizeInBytes() const
{
	return m_dataSize;
}

size_t BufferParser::GetCurrentOffset() const
{
	return m_currentReadPosition;
}

size_t BufferParser::GetRemainingByteCount() const
{
	return static_cast<size_t>(m_dataSize - m_currentReadPosition);
}

bool BufferParser::JumpToPosition(size_t newByteOffsetFromScanStartPos)
{
	if (newByteOffsetFromScanStartPos > m_dataSize)
	{
		if (g_devConsole)
		{
			g_devConsole->AddLine(DevConsole::ERROR, "BufferParser: JumpToPosition() beyond buffer size", 1.f, true);
			g_devConsole->SetMode(FULL);
		}
		return false;
	}

	m_currentReadPosition = newByteOffsetFromScanStartPos;
	return true;
}

bool BufferParser::AdvanceBytes(size_t byteCount)
{
	if (!CanReadBytes(byteCount))
	{
		if (g_devConsole)
		{
			g_devConsole->AddLine(DevConsole::ERROR, "BufferParser: AdvanceBytes past end of buffer", 1.f, true);
			g_devConsole->SetMode(FULL);
		}
		return false;
	}

	m_currentReadPosition += byteCount;
	return true;
}


bool BufferParser::CanReadBytes(size_t byteCount) const
{
	if (m_bufferStart == nullptr)
	{
		return false;
	}

	if (m_currentReadPosition > m_dataSize)
	{
		return false;
	}

	size_t const remainingByteCount = GetRemainingByteCount();
	return (byteCount <= remainingByteCount);
}

void BufferParser::ReadBytes(void* outDestination, size_t byteCount)
{
    if (outDestination == nullptr)
    {
		if (g_devConsole)
		{
			g_devConsole->AddLine(DevConsole::ERROR, "Buffer Parser: attempting to read bytes with a null outDestination", 1.f, true);
			g_devConsole->SetMode(FULL);
		}
        return;
    }

    if (!CanReadBytes(byteCount))
    {
		if (g_devConsole)
		{
			g_devConsole->AddLine(DevConsole::ERROR, "Buffer Parser: attempting to read past end of buffer", 1.f, true);
			g_devConsole->SetMode(FULL);
		}
		return;
    }

	else
	{
		unsigned char const* sourceByteAddress = (m_bufferStart + m_currentReadPosition);
		memcpy(outDestination, sourceByteAddress, byteCount);
		m_currentReadPosition += byteCount;
	}
}


void BufferParser::SetEndianMode(EndianMode endianMode)
{
	EndianMode const nativeMode = GetPlatformNativeEndianMode();

	if (endianMode == EndianMode::NATIVE)
	{
		m_endianMode = nativeMode;
	}
	else
	{
		m_endianMode = endianMode;
	}

	m_isEndianModeOppositeNative = (m_endianMode != nativeMode);
}


uint8_t BufferParser::ParseByte()
{
	uint8_t value = 0;
    ReadBytes(&value, sizeof(uint8_t));
    return value;
}

char BufferParser::ParseChar()
{
    char value = 0;
    ReadBytes(&value, sizeof(char));
    return value;
}

uint16_t BufferParser::ParseUnsignedShort()
{
	uint16_t value = 0;
	ReadBytes(&value, sizeof(uint16_t));
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap16(&value);
	}
	return value;
}

int16_t BufferParser::ParseShort()
{
    int16_t value = 0;
	ReadBytes(&value, sizeof(int16_t));
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap16(&value);
	}
	return value;
}

uint32_t BufferParser::ParseUnsignedInt32()
{
	uint32_t value = 0;
	ReadBytes(&value, sizeof(uint32_t));
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap32(&value);
	}
	return value;
}

int BufferParser::ParseInt()
{
	int value = 0;
	ReadBytes(&value, sizeof(int));
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap32(&value);
	}
	return value;
}

float BufferParser::ParseFloat()
{
	float value = 0;
	ReadBytes(&value, sizeof(float));
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap32(&value);
	}
	return value;
}

double BufferParser::ParseDouble()
{
	double value = 0.0;
	ReadBytes(&value, sizeof(double));

	if (m_isEndianModeOppositeNative)
	{
		ByteSwap64(&value);
	}

	return value;
}

std::string BufferParser::ParseZeroTerminatedString()
{
	if (m_currentReadPosition >= m_dataSize)
	{
		if (g_devConsole)
		{
			g_devConsole->AddLine(DevConsole::ERROR, "BufferParser: zero-terminated string past end of buffer", 1.f, true);
			g_devConsole->SetMode(FULL);
		}
		return std::string();
	}

	size_t const stringStartOffset = m_currentReadPosition;

	while (m_currentReadPosition < m_dataSize)
	{
		unsigned char const currentByte = m_bufferStart[m_currentReadPosition];

		if (currentByte == 0)
		{
			size_t const stringLength = (m_currentReadPosition - stringStartOffset);

			std::string result(
				reinterpret_cast<char const*>(m_bufferStart + stringStartOffset),
				stringLength
			);

			m_currentReadPosition += 1; // skip null terminator
			return result;
		}

		m_currentReadPosition += 1;
	}

	if (g_devConsole)
	{
		g_devConsole->AddLine(DevConsole::ERROR, "BufferParser: zero-terminated string not terminated before end of buffer", 1.f, true);
		g_devConsole->SetMode(FULL);
	}
	return std::string();
}

std::string BufferParser::ParseLengthPrecededString()
{
	uint32_t const stringLength = ParseUnsignedInt32();

	if (!CanReadBytes(static_cast<size_t>(stringLength)))
	{
		if (g_devConsole)
		{
			g_devConsole->AddLine(DevConsole::ERROR, "BufferParser: length-preceded string exceeds buffer", 1.f, true);
			g_devConsole->SetMode(FULL);
		}

		return std::string();
	}

	std::string result;
	result.resize(static_cast<size_t>(stringLength));

	ReadBytes(result.data(), static_cast<size_t>(stringLength));

	return result;
}

bool BufferParser::ParseBool()
{
	uint8_t value = 0;
	ReadBytes(&value, sizeof(uint8_t));

	return (value != 0);
}

Vec2 BufferParser::ParseVec2()
{
	Vec2 newVec;
	newVec.x = ParseFloat();
	newVec.y = ParseFloat();
	return newVec;
}

Vec3 BufferParser::ParseVec3()
{
	Vec3 newVec;
	newVec.x = ParseFloat();
	newVec.y = ParseFloat();
	newVec.z = ParseFloat();
	return newVec;
}

Vec4 BufferParser::ParseVec4()
{
	Vec4 newVec;
	newVec.x = ParseFloat();
	newVec.y = ParseFloat();
	newVec.z = ParseFloat();
	newVec.w = ParseFloat();

	return newVec;
}

IntVec2 BufferParser::ParseIntVec2()
{
	IntVec2 newIntVec;
	newIntVec.x = ParseInt();
	newIntVec.y = ParseInt();

	return newIntVec;
}

IntVec3 BufferParser::ParseIntVec3()
{
	IntVec3 newIntVec;
	newIntVec.x = ParseInt();
	newIntVec.y = ParseInt();
	newIntVec.z = ParseInt();

	return newIntVec;
}

Rgba8 BufferParser::ParseRgba()
{
	Rgba8 result;
	result.r = ParseByte();
	result.g = ParseByte();
	result.b = ParseByte();
	result.a = ParseByte();
	return result;
}

Rgba8 BufferParser::ParseRgb()
{
	Rgba8 result;
	result.r = ParseByte();
	result.g = ParseByte();
	result.b = ParseByte();
	result.a = 255;
	return result;
}

AABB2 BufferParser::ParseAABB2()
{
	Vec2 mins = ParseVec2();
	Vec2 maxs = ParseVec2();
	return AABB2(mins, maxs);
}


AABB3 BufferParser::ParseAABB3()
{
	Vec3 mins = ParseVec3();
	Vec3 maxs = ParseVec3();
	return AABB3(mins, maxs);
}

OBB2 BufferParser::ParseOBB2()
{
	Vec2 m_center = ParseVec2();
	Vec2 m_iBasisNormal = ParseVec2();
	Vec2 m_halfDimensionsIJ = ParseVec2();
	return OBB2(m_center, m_iBasisNormal, m_halfDimensionsIJ);
}

OBB3 BufferParser::ParseOBB3()
{
	Vec3 m_center = ParseVec3();
	Vec3 m_halfDimensionsIJK = ParseVec3();
	Vec3 m_iBasis = ParseVec3();
	Vec3 m_jBasis = ParseVec3();
	Vec3 m_kBasis = ParseVec3();
	return OBB3(m_center, m_halfDimensionsIJK, m_iBasis, m_jBasis, m_kBasis);
}

Plane2D BufferParser::ParsePlane2D()
{
	Plane2D newPlane;
	newPlane.m_normal = ParseVec2();
	newPlane.m_distanceAlongNormal = ParseFloat();
	return newPlane;
}

Plane3D BufferParser::ParsePlane3D()
{
	Plane3D newPlane;
	newPlane.m_normal = ParseVec3();
	newPlane.m_distanceAlongNormal = ParseFloat();
	return newPlane;
}

Vertex_PCU BufferParser::ParseVertexPCU()
{
	Vec3 pos = ParseVec3();
	Rgba8 color = ParseRgba();
	Vec2 uvs = ParseVec2();
	return Vertex_PCU(pos, color, uvs);
}

Vertex_PCUTBN BufferParser::ParseVertexPCUTBN()
{
	Vec3 pos = ParseVec3();
	Rgba8 color = ParseRgba();
	Vec2 uvs = ParseVec2();
	Vec3 tangent = ParseVec3();
	Vec3 biTangent = ParseVec3();
	Vec3 normal = ParseVec3();

	return Vertex_PCUTBN(pos, color, tangent, biTangent, normal, uvs);
}



EndianMode GetPlatformNativeEndianMode()
{
	uint16_t const testValue = 0x0001;
	uint8_t const* testBytes = reinterpret_cast<uint8_t const*>(&testValue);

	if (testBytes[0] == 0x01)
	{
		return EndianMode::LITTLE;
	}
	else
	{
		return EndianMode::BIG;
	}
}

void ByteSwap16(void* value)
{
	uint16_t* valueAsUint16 = reinterpret_cast<uint16_t*>(value);

	uint16_t originalValue = *valueAsUint16;

	uint16_t swappedValue = static_cast<uint16_t>(
		((originalValue & 0x00FFu) << 8) |
		((originalValue & 0xFF00u) >> 8)
		);

	*valueAsUint16 = swappedValue;
}

void ByteSwap32(void* value)
{
	uint32_t* valueAsUint32 = reinterpret_cast<uint32_t*>(value);

	uint32_t originalValue = *valueAsUint32;

	uint32_t swappedValue =
		((originalValue & 0x000000FFu) << 24) |
		((originalValue & 0x0000FF00u) << 8) |
		((originalValue & 0x00FF0000u) >> 8) |
		((originalValue & 0xFF000000u) >> 24);

	*valueAsUint32 = swappedValue;
}

void ByteSwap64(void* value)
{
	uint64_t* valueAsUint64 = reinterpret_cast<uint64_t*>(value);

	uint64_t originalValue = *valueAsUint64;

	uint64_t swappedValue =
		((originalValue & 0x00000000000000FFull) << 56) |
		((originalValue & 0x000000000000FF00ull) << 40) |
		((originalValue & 0x0000000000FF0000ull) << 24) |
		((originalValue & 0x00000000FF000000ull) << 8) |
		((originalValue & 0x000000FF00000000ull) >> 8) |
		((originalValue & 0x0000FF0000000000ull) >> 24) |
		((originalValue & 0x00FF000000000000ull) >> 40) |
		((originalValue & 0xFF00000000000000ull) >> 56);

	*valueAsUint64 = swappedValue;
}

BufferWriter::BufferWriter(std::vector<uint8_t>& buffer, EndianMode endianMode)
	:m_buffer(buffer)
{
	SetEndianMode(endianMode);
}

void BufferWriter::SetEndianMode(EndianMode endianMode)
{
	EndianMode const nativeMode = GetPlatformNativeEndianMode();

	if (endianMode == EndianMode::NATIVE)
	{
		m_endianMode = nativeMode;
	}
	else
	{
		m_endianMode = endianMode;
	}

	m_isEndianModeOppositeNative = (m_endianMode != nativeMode);
}

void BufferWriter::AppendByte(uint8_t value)
{
	AppendBytes(&value, sizeof(uint8_t));
}

void BufferWriter::AppendChar(char value)
{
	AppendBytes(&value, sizeof(char));
}

void BufferWriter::AppendUnsignedShort(uint16_t value)
{
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap16(&value);
	}

	AppendBytes(&value, sizeof(uint16_t));
}

void BufferWriter::AppendShort(int16_t value)
{
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap16(&value);
	}

	AppendBytes(&value, sizeof(int16_t));
}

void BufferWriter::AppendUnsignedInt32(uint32_t value)
{
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap32(&value);
	}

	AppendBytes(&value, sizeof(uint32_t));
}

void BufferWriter::AppendInt(int value)
{
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap32(&value);
	}

	AppendBytes(&value, sizeof(int));
}

void BufferWriter::AppendUInt64(uint64_t value)
{
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap64(&value);
	}

	AppendBytes(&value, sizeof(uint64_t));
}

void BufferWriter::AppendInt64(int64_t value)
{
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap64(&value);
	}

	AppendBytes(&value, sizeof(int64_t));
}

void BufferWriter::AppendFloat(float value)
{
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap32(&value);
	}

	AppendBytes(&value, sizeof(float));
}

void BufferWriter::AppendDouble(double value)
{
	if (m_isEndianModeOppositeNative)
	{
		ByteSwap64(&value);
	}

	AppendBytes(&value, sizeof(double));
}

void BufferWriter::AppendZeroTerminatedString(std::string const& value)
{
	AppendBytes(value.data(), value.size());

	uint8_t zero = 0;
	AppendBytes(&zero, sizeof(uint8_t));
}

void BufferWriter::AppendLengthPrecededString(std::string const& value)
{
	uint32_t length = static_cast<uint32_t>(value.size());

	AppendUnsignedInt32(length);

	AppendBytes(value.data(), value.size());
}

void BufferWriter::AppendBool(bool value)
{
	uint8_t numValue = value ? 1 : 0;
	AppendByte(numValue);
}

void BufferWriter::AppendVec2(Vec2 const& value)
{
	AppendFloat(value.x);
	AppendFloat(value.y);
}

void BufferWriter::AppendVec3(Vec3 const& value)
{
	AppendFloat(value.x);
	AppendFloat(value.y);
	AppendFloat(value.z);
}

void BufferWriter::AppendVec4(Vec4 const& value)
{
	AppendFloat(value.x);
	AppendFloat(value.y);
	AppendFloat(value.z);
	AppendFloat(value.w);
}

void BufferWriter::AppendIntVec2(IntVec2 const& value)
{
	AppendInt(value.x);
	AppendInt(value.y);
}

void BufferWriter::AppendIntVec3(IntVec3 const& value)
{
	AppendInt(value.x);
	AppendInt(value.y);
	AppendInt(value.z);
}

void BufferWriter::AppendRgba(Rgba8 const& value)
{
	AppendByte(value.r);
	AppendByte(value.g);
	AppendByte(value.b);
	AppendByte(value.a);
}

void BufferWriter::AppendRgb(Rgba8 const& value)
{
	AppendByte(value.r);
	AppendByte(value.g);
	AppendByte(value.b);
}

void BufferWriter::AppendAABB2(AABB2 const& value)
{
	AppendVec2(value.m_mins);
	AppendVec2(value.m_maxs);
}

void BufferWriter::AppendAABB3(AABB3 const& value)
{
	AppendVec3(value.m_mins);
	AppendVec3(value.m_maxs);
}

void BufferWriter::AppendOBB2(OBB2 const& value)
{
	AppendVec2(value.m_center);
	AppendVec2(value.m_iBasisNormal);
	AppendVec2(value.m_halfDimensionsIJ);
}

void BufferWriter::AppendOBB3(OBB3 const& value)
{
	AppendVec3(value.m_center);
	AppendVec3(value.m_halfDimensionsIJK);
	AppendVec3(value.m_iBasis);
	AppendVec3(value.m_jBasis);
	AppendVec3(value.m_kBasis);
}

void BufferWriter::AppendPlane2D(Plane2D const& value)
{
	AppendVec2(value.m_normal);
	AppendFloat(value.m_distanceAlongNormal);
}

void BufferWriter::AppendPlane3D(Plane3D const& value)
{
	AppendVec3(value.m_normal);
	AppendFloat(value.m_distanceAlongNormal);
}

void BufferWriter::AppendVertexPCU(Vertex_PCU const& value)
{
	AppendVec3(value.m_position);
	AppendRgba(value.m_color);
	AppendVec2(value.m_uvTexCoords);
}

void BufferWriter::AppendVertexPCUTBN(Vertex_PCUTBN const& value)
{
	AppendVec3(value.m_position);
	AppendRgba(value.m_color);
	AppendVec2(value.m_uvTexCoords);
	AppendVec3(value.m_tangent);
	AppendVec3(value.m_biTangent);
	AppendVec3(value.m_normal);
}

void BufferWriter::OverwriteUInt32AtOffset(size_t offset, uint32_t value)
{
	if ((offset + sizeof(uint32_t)) > m_buffer.size())
	{
		ERROR_RECOVERABLE("BufferWriter: overwrite beyond buffer");
		return;
	}

	if (m_isEndianModeOppositeNative)
	{
		ByteSwap32(&value);
	}

	unsigned char* destination = m_buffer.data() + offset;
	memcpy(destination, &value, sizeof(uint32_t));
}

void BufferWriter::AppendBytes(void const* data, size_t byteCount)
{
	unsigned char const* source = reinterpret_cast<unsigned char const*>(data);

	size_t const originalSize = m_buffer.size();
	m_buffer.resize(originalSize + byteCount);

	memcpy(m_buffer.data() + originalSize, source, byteCount);
}
