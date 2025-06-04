#include "file_scanner.h"

namespace fs = std::filesystem;

void ScanFiles(const std::filesystem::path& dirPath, std::vector<FileEntry>& files, const std::string& basePath) {
	for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
		std::string relativePath = basePath + "\\" + entry.path().filename().string();

		if (entry.is_directory()) {
			files.push_back({ relativePath, entry.path().string(), true });
			ScanFiles(entry.path(), files, relativePath);
		}
		else if (entry.is_regular_file()) {
			files.push_back({ relativePath, entry.path().string(), false });
		}
	}
}