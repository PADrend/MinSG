/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#if defined(_WIN32) || defined(_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "DataDirectory.h"
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/Macros.h>
#include <Util/Utils.h>
#include <cstdlib>
#include <string>

namespace MinSG {
namespace DataDirectory {

std::string getPath() {
	static std::string dataDirPath;
	// Check if the path is already cached.
	if(!dataDirPath.empty()) {
		return dataDirPath;
	}

	std::string testFile;

	// First check for a environment variable
	if(std::getenv("MINSG_DATA_DIR")) {
		dataDirPath = std::getenv("MINSG_DATA_DIR");
		testFile = dataDirPath + "/shader/Phong.vs";
	}
	if(testFile.empty() || !Util::FileUtils::isFile(Util::FileName(testFile))) {
		// Check exe/../share/MinSG
		dataDirPath = Util::FileName(Util::Utils::getExecutablePath()).getDir() 
															+ "/../share/MinSG";
		testFile = dataDirPath + "/shader/Phong.vs";
	}
#ifdef MINSG_DATA_DIR
	if(testFile.empty() || !Util::FileUtils::isFile(Util::FileName(testFile))) {
		// Check preprocessor defined path
		dataDirPath = MINSG_DATA_DIR;
		testFile = dataDirPath + "/shader/Phong.vs";
	}
#endif
	if(testFile.empty() || !Util::FileUtils::isFile(Util::FileName(testFile))) {
		WARN("Could not find MinSG's data directory.");
		dataDirPath.clear();
	}
	return dataDirPath;
}

}
}
