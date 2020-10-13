/*
	This file is part of the MinSG library.
	Copyright (C) 2014 Claudius J�hn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VOXEL_WORLD

#ifndef VOXEL_WORLD_H_
#define VOXEL_WORLD_H_

#include <Util/References.h>
#include <cstdint>

namespace Rendering{
class Mesh;
}

namespace Geometry{
template<typename value_t> class _Box;
template<typename Voxel_t,unsigned int blockSizePow, typename integer_t, typename uinteger_t> class VoxelStorage;
}

namespace MinSG {
//! @ingroup ext
namespace VoxelWorld{

typedef Geometry::VoxelStorage<uint32_t,2,int32_t,uint32_t> simpleVoxelStorage_t;

MINSGAPI Util::Reference<Rendering::Mesh> generateMesh( const simpleVoxelStorage_t&, const Geometry::_Box<int32_t>& boundary); // material lib

}
}

#endif /* VOXEL_WORLD_H_ */

#endif /* MINSG_EXT_VOXEL_WORLD */
