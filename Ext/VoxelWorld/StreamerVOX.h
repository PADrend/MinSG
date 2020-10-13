/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VOXEL_WORLD

#ifndef MINSG_STREAMERVOX_H_
#define MINSG_STREAMERVOX_H_

#include <Geometry/Vec3.h>

#include <array>
#include <cstdint>
#include <iosfwd>
#include <vector>

namespace Util {
class FileName;
}
	
namespace MinSG {
namespace VoxelWorld{


/**

	.vox (Voxel File Format)
	========================

	Fileformat: binary little endian

	VOX-File ::=	Header (char[4] "vox"+chr(13) ),
					uint32 version (currently 0x01),
					DataBlock *,
					EndMarker (uint32 0xFFFFFFFF)

	DataBlock ::=	uint32 dataType,
					uint32 dataSize -- nr of bytes to be jumped to skip the block (not including dataType and blockSize value),
					uint8 data[dataSize]

	DataBlock ::=	AreaBlock

	DataBlock ::=	4x4LeafsBlock

	AreaBlock ::= 	AreaBlock-dataType (uint32 0x56000000),
					uint32 dataSize,
					uint32 numberOfAreas
					[int32 x,int32 y,int32 z, uint32_t sideLength, uint32_t value]*

	4x4x4LeafsBlock ::= AreaBlock-dataType (uint32 0x56000001),
					uint32 dataSize,
					uint32 numberOfLeafBlocks
					[int32 x,int32 y,int32 z, uint8_t nrOfRuns, [uint32_t value,uint8_t RunLength]* ]*
*/

typedef std::pair<	std::vector<std::tuple<Geometry::_Vec3<int32_t>,uint32_t,uint32_t>>,	// uniform areas
					std::vector<std::tuple<Geometry::_Vec3<int32_t>, std::array<uint32_t, 64>>> >// blocks
				serializationData_t;

MINSGAPI serializationData_t loadVoxels(std::istream & input);
MINSGAPI serializationData_t loadVoxels(const Util::FileName & filename);
MINSGAPI void saveVoxels(std::ostream & output, const serializationData_t&);
MINSGAPI void saveVoxels(const Util::FileName & filename, const serializationData_t&);


}
}

#endif /* MINSG_STREAMERVOX_H_ */

#endif /* MINSG_EXT_VOXEL_WORLD */
