/*
	This file is part of the MinSG library extension ThesisPeter.
	Copyright (C) 2014 Peter Stilow

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#define MINSG_EXT_THESISPETER
#ifdef MINSG_EXT_THESISPETER

#include "DebugObjects.h"

#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/Mesh/Mesh.h>
#include <MinSG/SceneManagement/Importer/ImportContext.h>
#include <Util/Macros.h>

namespace MinSG {
namespace ThesisPeter {

DebugObjects::DebugObjects(){
	sceneRootNode = 0;
	nodeAdded = false;
	lineNode = new MinSG::GeometryNode();
}

DebugObjects::~DebugObjects(){;
	if(nodeAdded){
//		sceneRootNode->removeChild(lineNode.get());
		lineNode.get()->removeFromParent();
	}
}

void DebugObjects::setSceneRootNode(MinSG::Node* sceneRootNode){
	this->sceneRootNode = (MinSG::GroupNode*)sceneRootNode;
}

void DebugObjects::addDebugLine(Geometry::Vec3 start, Geometry::Vec3 end, Util::Color4f colStart, Util::Color4f colEnd){
	DebugLine *line = new DebugLine();
	line->start = start;
	line->end = end;
	line->colorStart = colStart;
	line->colorEnd = colEnd;
	linesData.push_back(line);
}

void DebugObjects::clearDebug(){
	for(unsigned int i = 0; i < linesData.size(); i++){
		delete linesData[i];
	}
	linesData.clear();
}

void DebugObjects::buildDebugLineNode(){
	Rendering::VertexDescription vertexDesc;
	vertexDesc.appendPosition3D();
	vertexDesc.appendColorRGBAByte();
	Rendering::MeshUtils::MeshBuilder mb(vertexDesc);

	for(unsigned int i = 0; i < linesData.size(); i++){
        mb.color(linesData[i]->colorStart);
        mb.position(linesData[i]->start);
        mb.addVertex();

        mb.color(linesData[i]->colorEnd);
        mb.position(linesData[i]->end);
        mb.addVertex();
	}

	Rendering::Mesh *mesh = mb.buildMesh();
	mesh->setUseIndexData(false);
	mesh->setDrawMode(Rendering::Mesh::DRAW_LINES);

	((MinSG::GeometryNode*)lineNode.get())->setMesh(mesh);

	if(!nodeAdded){
		nodeAdded = true;
		lineNode.get()->setTempNode(true);
		sceneRootNode->addChild(lineNode);
	}
}

}
}

#endif /* MINSG_EXT_THESISPETER */
