/*
	This file is part of the MinSG library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VOXEL_WORLD

#ifndef VOXEL_MATERIAL_LIB_H_
#define VOXEL_MATERIAL_LIB_H_

#include "Material.h"
#include <unordered_map>
#include <memory>


namespace MinSG {
namespace VoxelWorld{


class MaterialLib{
	std::unordered_map<uint32_t,std::unique_ptr<Material>> m;

public:
	MaterialLib() {}

	Material& access(uint32_t id){
		const auto it = m.find(id);
		if( it == m.end() ){
			auto * mat = new Material;
			m.insert( std::make_pair(id,std::unique_ptr<Material>(mat)));
			return *mat;
		}else {
			return *it->second.get();
		}
	}
	Material* get(uint32_t id)const{
		const auto it = m.find(id);
		return it == m.end() ? nullptr : it->second.get();
	}
};

}
}

#endif /* VOXEL_MATERIAL_LIB_H_ */

#endif /* MINSG_EXT_VOXEL_WORLD */
