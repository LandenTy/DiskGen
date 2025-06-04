#define _CRT_SECURE_NO_WARNINGS
#include "image_creator.h"
#include "fat_utils.h"

#include <cstdio>
#include <cstring>
#include <cctype>
#include <iostream>
#include <vector>

void CreateImageFromFiles(const FloppyConfig& cfg,
	const std::string& oemName,
	const std::string& volLabel,
	const std::vector<FileEntry>& files)
{
	FILE* f = fopen("output.img", "wb+");
	if (!f) {
		std::cerr << "Failed to create output.img" << std::endl;
		return;
	}

	unsigned char boot[512] = { 0 };

	// Simple boot sector template:
	boot[0] = 0xEB;
	boot[1] = 0x3C;
	boot[2] = 0x90;

	for (int i = 0; i < 8; ++i) {
		if (i < (int)oemName.length())
			boot[3 + i] = toupper(oemName[i]);
		else
			boot[3 + i] = ' ';
	}

	boot[11] = cfg.bytesPerSector & 0xFF;
	boot[12] = (cfg.bytesPerSector >> 8) & 0xFF;
	boot[13] = cfg.sectorsPerCluster;
	boot[14] = cfg.reservedSectors & 0xFF;
	boot[15] = (cfg.reservedSectors >> 8) & 0xFF;
	boot[16] = cfg.numFATs;
	boot[17] = cfg.rootDirEntries & 0xFF;
	boot[18] = (cfg.rootDirEntries >> 8) & 0xFF;
	boot[19] = cfg.totalSectors & 0xFF;
	boot[20] = (cfg.totalSectors >> 8) & 0xFF;
	boot[21] = cfg.mediaDescriptor;
	boot[22] = cfg.fatSizeSectors & 0xFF;
	boot[23] = (cfg.fatSizeSectors >> 8) & 0xFF;
	boot[24] = cfg.sectorsPerTrack & 0xFF;
	boot[25] = (cfg.sectorsPerTrack >> 8) & 0xFF;
	boot[26] = cfg.numHeads & 0xFF;
	boot[27] = (cfg.numHeads >> 8) & 0xFF;

	// Hidden sectors (4 bytes) - zero
	// Large sectors (4 bytes) - zero

	boot[36] = 0x00; // Drive number
	boot[37] = 0x00; // Reserved
	boot[38] = 0x29; // Boot signature

	// Volume ID (4 random bytes)
	boot[39] = 0x55;
	boot[40] = 0xAA;
	boot[41] = 0x66;
	boot[42] = 0x77;

	// Volume label (11 chars)
	for (int i = 0; i < 11; ++i) {
		if (i < (int)volLabel.length())
			boot[43 + i] = toupper(volLabel[i]);
		else
			boot[43 + i] = ' ';
	}

	const char* fsType = "FAT12   ";
	memcpy(boot + 54, fsType, 8);

	boot[510] = 0x55;
	boot[511] = 0xAA;

	fwrite(boot, 1, 512, f);

	// Zero-fill rest of disk image
	unsigned char zero[512] = { 0 };
	for (int i = 1; i < cfg.totalSectors; ++i) {
		fwrite(zero, 1, 512, f);
	}

	// Initialize FAT tables
	unsigned char* fat = new unsigned char[cfg.bytesPerSector * cfg.fatSizeSectors];
	memset(fat, 0, cfg.bytesPerSector * cfg.fatSizeSectors);

	fat[0] = cfg.mediaDescriptor;
	fat[1] = 0xFF;
	fat[2] = 0xFF;

	SetFatEntry(fat, 0, 0xFF0);
	SetFatEntry(fat, 1, 0xFFF);

	int nextFreeCluster = 2;

	// Allocate root directory entries
	unsigned char* rootDir = new unsigned char[32 * cfg.rootDirEntries];
	memset(rootDir, 0, 32 * cfg.rootDirEntries);

	// Volume label root dir entry
	unsigned char* volDirEntry = rootDir;
	for (int i = 0; i < 11; ++i) {
		if (i < (int)volLabel.length())
			volDirEntry[i] = toupper(volLabel[i]);
		else
			volDirEntry[i] = ' ';
	}
	volDirEntry[11] = 0x08; // Volume label attribute

	int dirEntryIndex = 1;

	// Helper lambda to add directory entry
	auto AddDirEntry = [&](const std::string& path, bool isDir, int startCluster, unsigned int fileSize) -> bool
	{
		if (dirEntryIndex >= cfg.rootDirEntries) {
			std::cerr << "Root directory full, cannot add: " << path << std::endl;
			return false;
		}

		unsigned char* dirEntry = rootDir + (dirEntryIndex * 32);
		memset(dirEntry, 0, 32);

		// Extract filename and extension from path (last component)
		size_t lastSlash = path.find_last_of("/\\");
		std::string baseName = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);

		// Separate name and extension
		size_t dotPos = baseName.find_last_of('.');
		std::string namePart = (dotPos == std::string::npos) ? baseName : baseName.substr(0, dotPos);
		std::string extPart = (dotPos == std::string::npos) ? "" : baseName.substr(dotPos + 1);

		char fname[8];
		char fext[3];
		memset(fname, ' ', 8);
		memset(fext, ' ', 3);

		// Copy name part, uppercase, max 8 chars
		for (size_t i = 0; i < namePart.size() && i < 8; ++i) {
			fname[i] = toupper(namePart[i]);
		}

		// Copy extension part, uppercase, max 3 chars
		for (size_t i = 0; i < extPart.size() && i < 3; ++i) {
			fext[i] = toupper(extPart[i]);
		}

		memcpy(dirEntry, fname, 8);
		memcpy(dirEntry + 8, fext, 3);

		if (isDir)
			dirEntry[11] = 0x10; // Directory attribute
		else
			dirEntry[11] = 0x20; // Archive attribute

		dirEntry[26] = startCluster & 0xFF;
		dirEntry[27] = (startCluster >> 8) & 0xFF;

		dirEntry[28] = (unsigned char)(fileSize & 0xFF);
		dirEntry[29] = (unsigned char)((fileSize >> 8) & 0xFF);
		dirEntry[30] = (unsigned char)((fileSize >> 16) & 0xFF);
		dirEntry[31] = (unsigned char)((fileSize >> 24) & 0xFF);

		dirEntryIndex++;
		return true;
	};

	// Write files and directories
	for (size_t i = 0; i < files.size(); ++i) {
		const FileEntry& fe = files[i];

		if (fe.isDirectory) {
			// Directories take cluster 0 (root directory), size 0
			if (!AddDirEntry(fe.path, true, 0, 0)) {
				std::cerr << "Failed to add directory entry for: " << fe.path << std::endl;
			}
			else {
				std::cout << "Added directory entry for: " << fe.path << std::endl;
			}
			continue;
		}

		// File handling
		FILE* infile = fopen(fe.realPath.c_str(), "rb");
		if (!infile) {
			std::cerr << "Failed to open input file: " << fe.realPath << std::endl;
			continue;
		}

		fseek(infile, 0, SEEK_END);
		long fileSize = ftell(infile);
		fseek(infile, 0, SEEK_SET);

		if (fileSize <= 0) {
			fclose(infile);
			continue;
		}

		int clustersNeeded = (fileSize + (cfg.bytesPerSector * cfg.sectorsPerCluster) - 1) / (cfg.bytesPerSector * cfg.sectorsPerCluster);

		int maxClusters = (cfg.totalSectors - (cfg.reservedSectors + cfg.numFATs * cfg.fatSizeSectors + (cfg.rootDirEntries * 32) / cfg.bytesPerSector)) / cfg.sectorsPerCluster + 2;

		if (nextFreeCluster + clustersNeeded > maxClusters) {
			std::cerr << "Not enough space on floppy for all files" << std::endl;
			fclose(infile);
			break;
		}

		int firstCluster = nextFreeCluster;
		int currentCluster = firstCluster;

		long bytesLeft = fileSize;

		for (int c = 0; c < clustersNeeded; ++c) {
			int sector = cfg.reservedSectors + cfg.numFATs * cfg.fatSizeSectors + (cfg.rootDirEntries * 32) / cfg.bytesPerSector + (currentCluster - 2) * cfg.sectorsPerCluster;
			fseek(f, sector * cfg.bytesPerSector, SEEK_SET);

			unsigned char buffer[512] = { 0 };
			int bytesToRead = (bytesLeft < (long)(cfg.bytesPerSector * cfg.sectorsPerCluster)) ? (int)bytesLeft : (cfg.bytesPerSector * cfg.sectorsPerCluster);

			size_t readBytes = fread(buffer, 1, bytesToRead, infile);
			if (readBytes != (size_t)bytesToRead) {
				std::cerr << "Warning: Could not read expected bytes from file: " << fe.realPath << std::endl;
			}

			fwrite(buffer, 1, cfg.bytesPerSector * cfg.sectorsPerCluster, f);

			bytesLeft -= bytesToRead;

			if (c == clustersNeeded - 1) {
				SetFatEntry(fat, currentCluster, 0xFFF);
			}
			else {
				SetFatEntry(fat, currentCluster, currentCluster + 1);
			}

			currentCluster++;
			if (bytesLeft <= 0) break;
		}

		fclose(infile);

		if (!AddDirEntry(fe.path, false, firstCluster, (unsigned int)fileSize)) {
			std::cerr << "Failed to add directory entry for file: " << fe.path << std::endl;
		}
		else {
			std::cout << "Added file entry for: " << fe.path << std::endl;
		}

		nextFreeCluster += clustersNeeded;
	}

	// Write FAT tables
	for (int i = 0; i < cfg.numFATs; ++i) {
		fseek(f, (cfg.reservedSectors + i * cfg.fatSizeSectors) * cfg.bytesPerSector, SEEK_SET);
		fwrite(fat, 1, cfg.fatSizeSectors * cfg.bytesPerSector, f);
	}

	// Write root directory
	fseek(f, (cfg.reservedSectors + cfg.numFATs * cfg.fatSizeSectors) * cfg.bytesPerSector, SEEK_SET);
	fwrite(rootDir, 1, cfg.rootDirEntries * 32, f);

	fclose(f);

	delete[] fat;
	delete[] rootDir;

	std::cout << "Floppy image created as output.img" << std::endl;
}
