/*
	This file is part of the MinSG library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VOXEL_WORLD
#include "VoxelWorld.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Geometry/VoxelStorage.h>
#include <iostream>

namespace MinSG {

Util::Reference<Rendering::Mesh> VoxelWorld::generateMesh( const simpleVoxelStorage_t& voxelStorage, const Geometry::_Box<int32_t>& boundary){
	const auto data=voxelStorage.serialize(boundary);

//	std::cout << "Boundary:"<<boundary<<std::endl;
	
	Rendering::MeshUtils::MeshBuilder mb;
	for(const auto & area : data.first){
//		std::vector<std::tuple<Vec3_t,uinteger_t,Voxel_t>>
		const auto sidelength = std::get<1>(area);
		Rendering::MeshUtils::MeshBuilder::addBox(mb,Geometry::Box(	Geometry::Vec3(std::get<0>(area)), 
																	Geometry::Vec3(sidelength,sidelength,sidelength)));
	}
	const Geometry::Vec3 unit(1,1,1);
//	 std::get<0>
	for(const auto & voxels : data.second){

//		std::vector<std::tuple<Vec3_t,uinteger_t,Voxel_t>>
//std::vector<std::tuple<Vec3_t,block_t>
		const Geometry::Vec3 origin( std::get<0>(voxels) );
		const auto block = std::get<1>(voxels);
//		std::cout << "block:"<<origin<<std::endl;
		for(uint32_t i=0;i<simpleVoxelStorage_t::blockSize;++i){
			const uint32_t value = block[i];
//			std::cout << " "<<value;

			if(value!=0){
				const auto pos = origin + Geometry::Vec3(	i%simpleVoxelStorage_t::blockSideLength,
															(i/simpleVoxelStorage_t::blockSideLength)%simpleVoxelStorage_t::blockSideLength,
															(i/(simpleVoxelStorage_t::blockSideLength*simpleVoxelStorage_t::blockSideLength))%simpleVoxelStorage_t::blockSideLength);
				Rendering::MeshUtils::MeshBuilder::addBox(mb,Geometry::Box(Geometry::Box(pos,pos+unit)));
			}
		}
			
//		const auto block = voxels.get<1>();
		
	}
	
//			std::cout << " "<<std::endl;

	
	return mb.buildMesh();

}


}
#endif /* MINSG_EXT_VOXEL_WORLD */
