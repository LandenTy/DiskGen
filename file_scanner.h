#ifndef FILE_SCANNER_H
#define FILE_SCANNER_H

#include <string>
#include <vector>
#include <filesystem>

struct FileEntry {
	std::string path;       // Relative path in image (e.g., "A/B/C.TXT")
	std::string realPath;   // Actual file path on disk
	bool isDirectory;
};

void ScanFiles(const std::filesystem::path& dirPath, std::vector<FileEntry>& files, const std::string& basePath);

#endif
