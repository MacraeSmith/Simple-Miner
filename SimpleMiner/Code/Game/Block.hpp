#pragma once
#include <cstdint>

constexpr uint8_t BLOCK_BIT_MASK_IS_SKY = (1 << 0);
constexpr uint8_t BLOCK_BIT_MASK_IS_LIGHT_DIRTY = (1 << 1);
constexpr uint8_t BLOCK_BIT_MASK_IS_FULL_OPAQUE = (1 << 2);
constexpr uint8_t BLOCK_BIT_MASK_IS_SOLID = (1 << 3);
constexpr uint8_t BLOCK_BIT_MASK_IS_VISIBLE = (1 << 4);
constexpr uint8_t BLOCK_BIT_MASK_IS_LIQUID = (1 << 5);
constexpr uint8_t BLOCK_BIT_MASK_IS_LIQUID_DIRTY = (1 << 6);
constexpr uint8_t BLOCK_BIT_MASK_IS_LIQUID_SOURCE = (1 << 7);

struct Block
{
	Block() {};
	explicit Block(uint8_t blockDefIndex);
	uint8_t m_blockType = 0;
	uint8_t m_lightData = 0;
	uint8_t m_flags = 0;
	uint8_t m_liquidData = 0;

	bool IsSky() const;
	void SetIsSky(bool isSky);

	bool IsLightDirty() const;
	void SetIsLightDirty(bool isDirty);

	bool IsFullOpaque() const;
	void SetIsFullOpaque(bool isOpaque);

	bool IsSolid() const;
	void SetIsSolid(bool isSolid);

	bool IsVisible() const;
	void SetIsVisible(bool isVisible);

	bool IsLiquid() const;
	void SetIsLiquid(bool isLiquid);

	bool IsLiquidDirty() const;
	void SetIsLiquidDirty(bool isDirty);

	bool IsLiquidSource() const;
	void SetIsLiquidSource(bool isDirty);

	bool IsAir() const;

	uint8_t GetOutdoorLight() const;
	void SetOutdoorLight(uint8_t lightValue);
	uint8_t GetIndoorLight() const;
	void SetIndoorLight(uint8_t lightValue);

	uint8_t GetFlowValue() const;
	void SetFlowValue(uint8_t flowValue);
};

inline bool Block::IsSky() const { return (m_flags & BLOCK_BIT_MASK_IS_SKY) != 0; }
inline void Block::SetIsSky(bool isSky)
{
	if (isSky) m_flags |= BLOCK_BIT_MASK_IS_SKY;
	else       m_flags &= ~BLOCK_BIT_MASK_IS_SKY;
}

inline bool Block::IsLightDirty() const { return (m_flags & BLOCK_BIT_MASK_IS_LIGHT_DIRTY) != 0; }
inline void Block::SetIsLightDirty(bool isDirty)
{
	if (isDirty) m_flags |= BLOCK_BIT_MASK_IS_LIGHT_DIRTY;
	else         m_flags &= ~BLOCK_BIT_MASK_IS_LIGHT_DIRTY;
}

inline bool Block::IsFullOpaque() const { return (m_flags & BLOCK_BIT_MASK_IS_FULL_OPAQUE) != 0; }
inline void Block::SetIsFullOpaque(bool isOpaque)
{
	if (isOpaque) m_flags |= BLOCK_BIT_MASK_IS_FULL_OPAQUE;
	else          m_flags &= ~BLOCK_BIT_MASK_IS_FULL_OPAQUE;
}

inline bool Block::IsSolid() const { return (m_flags & BLOCK_BIT_MASK_IS_SOLID) != 0; }
inline void Block::SetIsSolid(bool isSolid)
{
	if (isSolid) m_flags |= BLOCK_BIT_MASK_IS_SOLID;
	else         m_flags &= ~BLOCK_BIT_MASK_IS_SOLID;
}

inline bool Block::IsVisible() const { return (m_flags & BLOCK_BIT_MASK_IS_VISIBLE) != 0; }
inline void Block::SetIsVisible(bool isVisible)
{
	if (isVisible) m_flags |= BLOCK_BIT_MASK_IS_VISIBLE;
	else           m_flags &= ~BLOCK_BIT_MASK_IS_VISIBLE;
}

inline uint8_t Block::GetOutdoorLight() const
{
	return (m_lightData >> 4) & 0x0F;
}

inline void Block::SetOutdoorLight(uint8_t val)
{
	val = (val > 15) ? 15 : val;
	m_lightData = (uint8_t)((m_lightData & 0x0F) | (val << 4));
}

inline uint8_t Block::GetIndoorLight() const
{
	return m_lightData & 0x0F;
}

inline void Block::SetIndoorLight(uint8_t val)
{
	val = (val > 15) ? 15 : val;
	m_lightData = (uint8_t)((m_lightData & 0xF0) | (val & 0x0F));
}

inline bool Block::IsLiquid() const { return (m_flags & BLOCK_BIT_MASK_IS_LIQUID) != 0; }
inline void Block::SetIsLiquid(bool isLiquid)
{
	if (isLiquid) m_flags |= BLOCK_BIT_MASK_IS_LIQUID;
	else         m_flags &= ~BLOCK_BIT_MASK_IS_LIQUID;
}

inline uint8_t Block::GetFlowValue() const { return m_liquidData & 0x0F; }

inline void Block::SetFlowValue(uint8_t val)
{
	if (val > 15) val = 15;
	m_liquidData = (uint8_t)((m_liquidData & 0xF0) | val);
}

inline bool Block::IsLiquidDirty() const
{
	return (m_flags & BLOCK_BIT_MASK_IS_LIQUID_DIRTY) != 0;
}

inline void Block::SetIsLiquidDirty(bool isDirty)
{
	if (isDirty)
		m_flags |= BLOCK_BIT_MASK_IS_LIQUID_DIRTY;
	else
		m_flags &= ~BLOCK_BIT_MASK_IS_LIQUID_DIRTY;
}

inline bool Block::IsLiquidSource() const
{
	return (m_flags & BLOCK_BIT_MASK_IS_LIQUID_SOURCE) != 0;
}

inline void Block::SetIsLiquidSource(bool isSource)
{
	if (isSource)
		m_flags |= BLOCK_BIT_MASK_IS_LIQUID_SOURCE;
	else
		m_flags &= ~BLOCK_BIT_MASK_IS_LIQUID_SOURCE;
}

inline bool Block::IsAir() const
{
	return m_blockType == 0;
}