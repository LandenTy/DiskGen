#ifndef FLOPPY_CONFIG_H
#define FLOPPY_CONFIG_H

#include <string>

struct FloppyConfig {
	int totalSectors;
	int fatSizeSectors;
	int rootDirEntries;
	int bytesPerSector;
	int sectorsPerCluster;
	int reservedSectors;
	int numFATs;
	int sectorsPerTrack;
	int numHeads;
	unsigned char mediaDescriptor;
};

FloppyConfig GetFloppyConfig(const std::string& type);

#endif // FLOPPY_CONFIG_H
