/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_DEFINITIONS_H_
#define OUTOFCORE_DEFINITIONS_H_

// #define MINSG_EXT_OUTOFCORE_DEBUG

#include <cstdint>

namespace MinSG {
namespace OutOfCore {

//! Identifier of a cache level inside the cache hierarchy
typedef uint8_t cacheLevelId_t;

//! Compile time maximum number of cache levels
const cacheLevelId_t maxNumCacheLevels = 8;

//! Possible types of cache levels
enum class CacheLevelType : uint32_t {
	FILE_SYSTEM = 1,		//!< @see CacheLevelFileSystem
	FILES = 2,				//!< @see CacheLevelFiles
	MAIN_MEMORY = 3,		//!< @see CacheLevelMainMemory
	GRAPHICS_MEMORY = 4		//!< @see CacheLevelGraphicsMemory
};

}
}

#endif /* OUTOFCORE_DEFINITIONS_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
