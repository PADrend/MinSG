/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "AbstractSurfelSampler.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/MeshUtils/MeshUtils.h>

#include <Util/Timer.h>

#include <memory>

namespace MinSG {
namespace BlueSurfels {
  
AbstractSurfelSampler::AbstractSurfelSampler() {
  static std::random_device rd;
  rng.seed(rd());
}

Rendering::Mesh* AbstractSurfelSampler::finalizeMesh(Rendering::Mesh* source, const std::vector<uint32_t>& indices) {
  // move surfels to new mesh
  auto surfels = new Rendering::Mesh;  
  std::unique_ptr<Rendering::MeshVertexData> vertexData(Rendering::MeshUtils::extractVertices(source, indices));  
  vertexData->updateBoundingBox();
  surfels->openVertexData().swap(*vertexData);
  surfels->setUseIndexData(false);
  surfels->setDrawMode(Rendering::Mesh::DRAW_POINTS);
  
  return surfels;
}

} /* BlueSurfels */
} /* MinSG */
#endif // MINSG_EXT_BLUE_SURFELS