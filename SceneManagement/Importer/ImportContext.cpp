/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ImportContext.h"
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Util/References.h>
#include <functional>
#include <utility>

namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace SceneManagement {

void ImportContext::executeFinalizingActions(){
	for(auto & action : finalizeActions) {
		action(*this);
	}
	finalizeActions.clear();
}

// ----------- registered Meshes

void ImportContext::registerMesh( const std::string & meshFileName, Rendering::Mesh * t){
	registeredMeshes[meshFileName]=t;
}

void ImportContext::unregisterMesh( const std::string & meshFileName){
	registeredMeshes.erase(meshFileName);
}

Rendering::Mesh * ImportContext::getRegisteredMesh( const std::string & meshFileName)const{
	auto it = registeredMeshes.find(meshFileName);
	return (it == registeredMeshes.end()) ? nullptr :  it->second.get();
}


Rendering::Mesh * ImportContext::getRegisteredMesh(const uint32_t hash , Rendering::Mesh * m)const{

	auto it = registeredHashedMeshes.find(hash);
	if(it==registeredHashedMeshes.end())
		return nullptr;

	const meshHashingRegistryBucket_t & bucket = it->second;
	for(const auto & it2 : bucket) {
		if(Rendering::MeshUtils::compareMeshes(it2.get(), m)) {
			return it2.get();
		}
	}
	return nullptr;
}

}
}
