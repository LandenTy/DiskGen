#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <filesystem>

#include "floppy_config.h"
#include "image_creator.h"
#include "file_scanner.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: floppytool <file1_or_dir> [file2_or_dir ...] [-oem OEMNAME] [-vol VOLLABEL] [-type FLOPPYTYPE]" << std::endl;
		std::cout << "FLOPPYTYPE options: 1.44 (default), 1.2, 360" << std::endl;
		return 1;
	}

	std::string oemName = "MSDOS5.0";
	std::string volLabel = "NO NAME    ";
	std::string floppyType = "1.44";

	std::vector<FileEntry> fileEntries;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-oem") == 0 && i + 1 < argc) {
			oemName = argv[++i];
			if (oemName.length() > 8) oemName = oemName.substr(0, 8);
		}
		else if (strcmp(argv[i], "-vol") == 0 && i + 1 < argc) {
			volLabel = argv[++i];
			if (volLabel.length() > 11) volLabel = volLabel.substr(0, 11);
		}
		else if (strcmp(argv[i], "-type") == 0 && i + 1 < argc) {
			floppyType = argv[++i];
		}
		else if (argv[i][0] == '-') {
			std::cerr << "Unknown option: " << argv[i] << std::endl;
			return 1;
		}
		else {
			fs::path p = argv[i];

			if (!fs::exists(p)) {
				std::cerr << "Path does not exist: " << p.string() << std::endl;
				return 1;
			}

			if (fs::is_directory(p)) {
				// Add directory itself as a directory entry
				std::string dirName = p.filename().string();
				fileEntries.push_back({ dirName, p.string(), true });

				// Recursively add its contents with relative paths starting at the directory name
				ScanFiles(p, fileEntries, dirName);
			}
			else if (fs::is_regular_file(p)) {
				// Add single file, relative path is just filename (no folders)
				fileEntries.push_back({ p.filename().string(), p.string(), false });
			}
			else {
				std::cerr << "Unsupported file type: " << p.string() << std::endl;
				return 1;
			}
		}
	}

	if (fileEntries.empty()) {
		std::cerr << "No input files specified." << std::endl;
		return 1;
	}

	FloppyConfig cfg = GetFloppyConfig(floppyType);

	CreateImageFromFiles(cfg, oemName, volLabel, fileEntries);

	return 0;
}