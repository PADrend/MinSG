/*
	This file is part of the MinSG library.
	Copyright (C) 2014 Claudius J�hn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VOXEL_WORLD

#ifndef VOXEL_MATERIAL_LIB_H_
#define VOXEL_MATERIAL_LIB_H_

#include "Material.h"
#include <Util/StringIdentifier.h>
#include <unordered_map>
#include <memory>


namespace MinSG {
namespace VoxelWorld{


class MaterialLib{

public:
	typedef uint32_t materialId_t;
	
	MaterialLib() = default;

	Material& access(const Util::StringIdentifier& name){
		const auto idFound = getMaterialId(name);
		return access(idFound.second ? idFound.second : createMaterialId(name));
	}
		
	Material& access(materialId_t id){
		const auto it = m.find(id);
		if( it == m.end() ){
			auto * mat = new Material;
			m.insert( std::make_pair(id,std::unique_ptr<Material>(mat)));
			return *mat;
		}else {
			return *it->second.get();
		}
	}
	Material* get(materialId_t id)const{
		const auto it = m.find(id);
		return it == m.end() ? nullptr : it->second.get();
	}
	
	std::pair<materialId_t,bool> getMaterialId(const Util::StringIdentifier& name)const{
		const auto it = materialNames.find(name);
		return it == materialNames.end() ? std::make_pair(static_cast<materialId_t>(0),false) : std::make_pair(it->second,true);
	}
	materialId_t createMaterialId(const Util::StringIdentifier& name){
		const auto formerId = getMaterialId(name);
		if(formerId.second)
			return formerId.first;
		materialId_t newId = static_cast<materialId_t>(materialNames.size()+1);
		materialNames[name] = newId;
		return newId;
	}
	const std::unordered_map<Util::StringIdentifier,materialId_t>& getMaterials()const{
		return materialNames;
	}
private:
	std::unordered_map<Util::StringIdentifier,materialId_t> materialNames;	// materialName --> mId
	std::unordered_map<materialId_t,std::unique_ptr<Material>> m;			// mId --> material

};

}
}

#endif /* VOXEL_MATERIAL_LIB_H_ */

#endif /* MINSG_EXT_VOXEL_WORLD */
