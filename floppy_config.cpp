#include "floppy_config.h"
#include <iostream>

FloppyConfig GetFloppyConfig(const std::string& type) {
	if (type == "1.44") {
		return FloppyConfig{ 2880, 9, 224, 512, 1, 1, 2, 18, 2, 0xF0 };
	}
	else if (type == "1.2") {
		return FloppyConfig{ 2400, 7, 224, 512, 1, 1, 2, 15, 2, 0xF9 };
	}
	else if (type == "360") {
		return FloppyConfig{ 720, 2, 224, 512, 1, 1, 2, 9, 2, 0xF9 };
	}
	else {
		std::cerr << "Unknown floppy type, defaulting to 1.44MB" << std::endl;
		return FloppyConfig{ 2880, 9, 224, 512, 1, 1, 2, 18, 2, 0xF0 };
	}
}
