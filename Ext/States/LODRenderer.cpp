/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "LODRenderer.h"

#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../../Rendering/RenderingContext/RenderingContext.h"
#include "../../../Rendering/Mesh/Mesh.h"
#include "../../../Geometry/Rect.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Util/StringIdentifier.h>
#include <Util/GenericAttribute.h>
#include <Util/Graphics/ColorLibrary.h>

#include <Rendering/Draw.h>
#include <Rendering/MeshUtils/Simplification.h>

#include <cassert>
#include <array>

namespace MinSG{

LODRenderer::LODRenderer() : NodeRendererState(FrameContext::DEFAULT_CHANNEL), minComplexity(pow(2,11)), maxComplexity(pow(2,24)), relComplexity(4.0){
}

static const Util::StringIdentifier idMeshes("lodMeshes");

static Util::GenericAttributeList* getLODs(GeometryNode * geo){
	if(geo->isInstance())
		return dynamic_cast<Util::GenericAttributeList*>(geo->getPrototype()->getAttribute(idMeshes));
	else
		return dynamic_cast<Util::GenericAttributeList*>(geo->getAttribute(idMeshes));
}

NodeRendererResult LODRenderer::displayNode(FrameContext & context, Node * node, const RenderParam & rp){
		
	if(rp.getFlag(RenderFlags::NO_GEOMETRY))
		return NodeRendererResult::PASS_ON;
	
	auto geo = dynamic_cast<MinSG::GeometryNode*>(node);
	
	if(		!geo
		||	!geo->getMesh()
		||	geo->getMesh()->getDrawMode() != Rendering::Mesh::DRAW_TRIANGLES
		||	geo->getMesh()->getPrimitiveCount() <= getMinComplexity()
	){
		return NodeRendererResult::PASS_ON;
	}
	
	auto lodMeshes = getLODs(geo);
	
	if(lodMeshes == nullptr)
		return NodeRendererResult::PASS_ON;
	
	auto projSize = context.getProjectedRect(geo).getArea();
	auto targetSize = static_cast<uint32_t>(projSize * getRelComplexity());
		
	Rendering::Mesh * usedMesh = nullptr;
	for(const auto & up : *lodMeshes){
		auto gaMesh = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(up.get());
		assert(gaMesh != nullptr);
		auto testMesh = gaMesh->get();
		
		if(testMesh->getPrimitiveCount() > getMaxComplexity())
			continue;
		
		if(		(!usedMesh)
			||	( usedMesh->getPrimitiveCount() < targetSize && testMesh->getPrimitiveCount() > usedMesh->getPrimitiveCount() )
			||	( usedMesh->getPrimitiveCount() > targetSize && testMesh->getPrimitiveCount() < usedMesh->getPrimitiveCount() && testMesh->getPrimitiveCount() >= targetSize)
		){
		usedMesh = testMesh;
		}
	}
	if(!usedMesh)
		return NodeRendererResult::PASS_ON;
	
	Util::Reference<Rendering::Mesh> tmp = geo->getMesh();
	geo->setMesh(usedMesh);
	geo->display(context, rp);
	geo->setMesh(tmp);
	
	return NodeRendererResult::NODE_HANDLED;
}

void LODRenderer::generateLODsRecursiv(Node* node)
{
	auto geos = collectNodes<GeometryNode>(node);
	for(auto geo : geos){
		
		if(geo->getMesh()->getPrimitiveCount() < getMinComplexity() * 2)
			continue;
		
		auto lodMeshes = getLODs(geo);
		if(!lodMeshes){
			lodMeshes = new Util::GenericAttributeList();
			if(geo->isInstance())
				geo->getPrototype()->setAttribute(idMeshes, lodMeshes);
			else
				geo->setAttribute(idMeshes, lodMeshes);
		}
		
		Util::Reference<Rendering::Mesh> mesh = geo->getMesh();
		for(const auto & up : *lodMeshes){
			auto gaMesh = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(up.get());
			assert(gaMesh != nullptr);
			auto m = gaMesh->get();
			if(m->getPrimitiveCount() < mesh->getPrimitiveCount())
				mesh = m;
		}
		
		std::array<float,5> weights;
		weights.fill(50);
		while(mesh.isNotNull() && mesh->getPrimitiveCount() > getMinComplexity() * 2){
			std::cerr << "Simplifying Mesh: " << mesh->getPrimitiveCount() << " --> " << mesh->getPrimitiveCount()/2 << " Triangles\n";
			mesh = Rendering::MeshUtils::Simplification::simplifyMesh(
				mesh.get(),
				mesh->getPrimitiveCount()/2,
				0,
				true,
				0.1,
				weights
			);
			lodMeshes->push_back(new Util::ReferenceAttribute<Rendering::Mesh>(mesh.get()));
		}
		
	}
}

}
