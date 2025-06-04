#include "fat_utils.h"

// Sets a FAT12 12-bit entry in the FAT table
void SetFatEntry(unsigned char* fat, int cluster, int value) {
	int offset = (cluster * 3) / 2;
	if (cluster & 1) {
		// Odd cluster number
		fat[offset] = (fat[offset] & 0x0F) | ((value << 4) & 0xF0);
		fat[offset + 1] = (value >> 4) & 0xFF;
	}
	else {
		// Even cluster number
		fat[offset] = value & 0xFF;
		fat[offset + 1] = (fat[offset + 1] & 0xF0) | ((value >> 8) & 0x0F);
	}
}
