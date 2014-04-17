/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_HELPER_DATAPATH_H
#define MINSG_HELPER_DATAPATH_H

#include <string>

namespace MinSG {

/**
 * @brief MinSG's data directory 
 * @deprecated Use a Util::FileLocator instead!
 * 
 * MinSG's data directory contains data files required by some parts of MinSG
 * (e.g., shader files).
 */
namespace DataDirectory {

/**
 * Return the path of the data directory. It is first searched with the help of
 * the environment variable MINSG_DATA_DIR, then by using a path relative to 
 * the running executable, and at last by checking the preprocessor define
 * MINSG_DATA_DIR, if set.
 * 
 * @return Absolute path to the data directory
 */
std::string getPath();

}

}

#endif /* MINSG_HELPER_DATAPATH_H */
