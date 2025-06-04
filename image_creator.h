#ifndef IMAGE_CREATOR_H
#define IMAGE_CREATOR_H

#include <string>
#include <vector>

#include "floppy_config.h"
#include "file_scanner.h"

void CreateImageFromFiles(const FloppyConfig& cfg,
	const std::string& oemName,
	const std::string& volLabel,
	const std::vector<FileEntry>& files);

const unsigned char ATTR_DIRECTORY = 0x10;

#endif // IMAGE_CREATOR_H
