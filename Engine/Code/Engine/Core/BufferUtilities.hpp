#pragma once
#include <vector>
#include <cstdint>
#include <string>

struct Vec2;
struct Vec3;
struct Vec4;
struct IntVec2;
struct IntVec3;
struct Rgba8;
struct AABB2;
struct AABB3;
struct OBB2;
struct OBB3;
struct Plane2D;
struct Plane3D;
struct Vertex_PCU;
struct Vertex_PCUTBN;

enum class EndianMode : int
{
	NATIVE,
	LITTLE,
	BIG
};

static EndianMode GetPlatformNativeEndianMode();
void ByteSwap16(void* value);
void ByteSwap32(void* value);
void ByteSwap64(void* value);

class BufferParser
{
public:
	BufferParser(unsigned char const* bufferToParse, size_t sizeInBytes, EndianMode endianMode = EndianMode::NATIVE);
	BufferParser(std::vector<uint8_t> const& bufferToParse, EndianMode endianMode = EndianMode::NATIVE);
	~BufferParser() {};

	size_t			GetSizeInBytes() const;
	size_t			GetCurrentOffset() const;
	size_t			GetRemainingByteCount() const;

	bool			JumpToPosition(size_t newByteOffsetFromScanStartPos);
	bool			AdvanceBytes(size_t byteCount);
	bool			CanReadBytes(size_t byteCount) const;

	void			SetEndianMode(EndianMode endianMode);
	EndianMode		GetEndianMode() const {return m_endianMode;}


	uint8_t			ParseByte();
	char			ParseChar();
	uint16_t		ParseUnsignedShort();
	int16_t			ParseShort();
	uint32_t		ParseUnsignedInt32();
	int				ParseInt();
	float			ParseFloat();
	double			ParseDouble();
	std::string		ParseZeroTerminatedString();
	std::string		ParseLengthPrecededString();
	bool			ParseBool();

	Vec2			ParseVec2();
	Vec3			ParseVec3();
	Vec4			ParseVec4();
	IntVec2			ParseIntVec2();
	IntVec3			ParseIntVec3();
	Rgba8			ParseRgba();
	Rgba8			ParseRgb();
	AABB2			ParseAABB2();
	AABB3			ParseAABB3();
	OBB2			ParseOBB2();
	OBB3			ParseOBB3();
	Plane2D			ParsePlane2D();
	Plane3D			ParsePlane3D();
	Vertex_PCU		ParseVertexPCU();
	Vertex_PCUTBN	ParseVertexPCUTBN();

private:
	void			ReadBytes(void* outDestination, size_t byteCount);


public:

protected:
	unsigned char const* m_bufferStart = nullptr;
	size_t m_dataSize = 0;
	size_t m_currentReadPosition = 0;

	EndianMode m_endianMode = EndianMode::NATIVE;
	bool m_isEndianModeOppositeNative = false;
	
};

class BufferWriter
{
public:
	BufferWriter(std::vector<uint8_t>& buffer, EndianMode endianMode = EndianMode::NATIVE);
	~BufferWriter(){};
	void		SetEndianMode(EndianMode endianMode);
	EndianMode	GetEndianMode() const {return m_endianMode;}
	size_t		GetBufferSize() const {return m_buffer.size();}
	std::vector<uint8_t> GetCopyOfBuffer() const {return m_buffer;}
	
	void		AppendByte(uint8_t value);
	void		AppendChar(char value);
	void		AppendUnsignedShort(uint16_t value);
	void		AppendShort(int16_t value);
	void		AppendUnsignedInt32(uint32_t value);
	void		AppendInt(int value);
	void		AppendUInt64(uint64_t value);
	void		AppendInt64(int64_t value);
	void		AppendFloat(float value);
	void		AppendDouble(double value);
	void		AppendZeroTerminatedString(std::string const& value);
	void		AppendLengthPrecededString(std::string const& value);
	void		AppendBool(bool value);

	void		AppendVec2(Vec2 const& value);
	void		AppendVec3(Vec3 const& value);
	void		AppendVec4(Vec4 const& value);
	void		AppendIntVec2(IntVec2 const& value);
	void		AppendIntVec3(IntVec3 const& value);
	void		AppendRgba(Rgba8 const& value);
	void		AppendRgb(Rgba8 const& value);
	void		AppendAABB2(AABB2 const& value);
	void		AppendAABB3(AABB3 const& value);
	void		AppendOBB2(OBB2 const& value);
	void		AppendOBB3(OBB3 const& value);
	void		AppendPlane2D(Plane2D const& value);
	void		AppendPlane3D(Plane3D const& value);
	void		AppendVertexPCU(Vertex_PCU const& value);
	void		AppendVertexPCUTBN(Vertex_PCUTBN const& value);

	void		OverwriteUInt32AtOffset(size_t offset, uint32_t value);
private:
	void		AppendBytes(void const* data, size_t byteCount);

private:
	std::vector<uint8_t>& m_buffer;
	EndianMode m_endianMode = EndianMode::NATIVE;
	bool m_isEndianModeOppositeNative = false;
};