#pragma once
#include <cstdint>
constexpr int NUM_BLOCKS_IN_INVENTORY = 9;
class PlayerInventory
{
public:
	PlayerInventory(){}
	~PlayerInventory(){};

	void Render() const;

	void ChangeSelectedBlock(int blockIndex) {m_selectedBlockIndex = blockIndex;}
	uint8_t GetSelectedBlock() const {return m_blocks[m_selectedBlockIndex];}

public:
	uint8_t m_blocks[NUM_BLOCKS_IN_INVENTORY] = {13, 14, 15, 1, 12, 5, 6, 2, 41};
	int m_selectedBlockIndex = 0;
};

