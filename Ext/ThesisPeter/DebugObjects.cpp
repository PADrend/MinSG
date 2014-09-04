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
#include <MinSG/Core/States/TransparencyRenderer.h>

namespace MinSG {
namespace ThesisPeter {

DebugObjects::DebugObjects(){
	sceneRootNode = 0;
	nodeAddedLines = false;
	nodeAddedFaces = false;
	lineNode = new MinSG::GeometryNode();
	faceNode = new MinSG::GeometryNode();
}

DebugObjects::~DebugObjects(){;
	if(nodeAddedLines){
//		sceneRootNode->removeChild(lineNode.get());
		lineNode.get()->removeFromParent();
	}
	if(nodeAddedFaces){
		faceNode.get()->removeFromParent();
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

void DebugObjects::addDebugFace(Geometry::Vec3 vert1, Geometry::Vec3 vert2, Geometry::Vec3 vert3, Geometry::Vec3 vert4, Util::Color4f col1, Util::Color4f col2, Util::Color4f col3, Util::Color4f col4){
	DebugFace *face = new DebugFace();
	face->vert1 = vert1;
	face->vert2 = vert2;
	face->vert3 = vert3;
	face->vert4 = vert4;
	face->col1 = col1;
	face->col2 = col2;
	face->col3 = col3;
	face->col4 = col4;
	facesData.push_back(face);
}

void DebugObjects::addDebugBoxMinMax(Geometry::Vec3 min, Geometry::Vec3 max, Util::Color4f col1, Util::Color4f col2){
	Geometry::Vec3 midPos = (min + max) * 0.5f;
	Geometry::Vec3 size = max - min;
	if(size.x() < 0 || size.y() < 0 || size.z() < 0){
		std::cout << "Box size is less 0! Taking abs!" << std::endl;
		size.setX(std::abs(size.x()));
		size.setY(std::abs(size.y()));
		size.setZ(std::abs(size.z()));
	}

	addDebugBox(midPos, size, col1, col2);
}

void DebugObjects::addDebugBox(Geometry::Vec3 midPos, Geometry::Vec3 size, Util::Color4f col1, Util::Color4f col2){
	Util::Color4f cH1(col1.r() * 0.6f + col2.r() * 0.4f, col1.g() * 0.6f + col2.g() * 0.4f, col1.b() * 0.6f + col2.b() * 0.4f, col1.a() * 0.6f + col2.a() * 0.4f);
	Util::Color4f cH2(col1.r() * 0.4f + col2.r() * 0.6f, col1.g() * 0.4f + col2.g() * 0.6f, col1.b() * 0.4f + col2.b() * 0.6f, col1.a() * 0.4f + col2.a() * 0.6f);
	Geometry::Vec3 sH = size * 0.5f;
	Geometry::Vec3 v0 = midPos - sH;
	Geometry::Vec3 v1(midPos.x() - sH.x(), midPos.y() - sH.y(), midPos.z() + sH.z());
	Geometry::Vec3 v2(midPos.x() - sH.x(), midPos.y() + sH.y(), midPos.z() - sH.z());
	Geometry::Vec3 v3(midPos.x() - sH.x(), midPos.y() + sH.y(), midPos.z() + sH.z());
	Geometry::Vec3 v4(midPos.x() + sH.x(), midPos.y() - sH.y(), midPos.z() - sH.z());
	Geometry::Vec3 v5(midPos.x() + sH.x(), midPos.y() - sH.y(), midPos.z() + sH.z());
	Geometry::Vec3 v6(midPos.x() + sH.x(), midPos.y() + sH.y(), midPos.z() - sH.z());
	Geometry::Vec3 v7 = midPos + sH;
	addDebugFace(v0, v2, v6, v4, col1, cH1, cH2, cH1);
	addDebugFace(v1, v3, v2, v0, cH1, cH2, cH1, col1);
	addDebugFace(v1, v0, v4, v5, cH1, col1, cH1, cH2);
	addDebugFace(v2, v3, v7, v6, cH1, cH2, col2, cH2);
	addDebugFace(v6, v7, v5, v4, cH2, col2, cH2, cH1);
	addDebugFace(v7, v3, v1, v5, col2, cH2, cH1, cH2);
}

void DebugObjects::addDebugBox(Geometry::Vec3 midPos, float size, Util::Color4f col1, Util::Color4f col2){
	addDebugBox(midPos, Geometry::Vec3(size, size, size), col1, col2);
}

void DebugObjects::addDebugBoxLinesMinMax(Geometry::Vec3 min, Geometry::Vec3 max, Util::Color4f col1, Util::Color4f col2){
	Geometry::Vec3 midPos = (min + max) * 0.5f;
	Geometry::Vec3 size = max - min;
	if(size.x() < 0 || size.y() < 0 || size.z() < 0){
		std::cout << "Box size is less 0! Taking abs!" << std::endl;
		size.setX(std::abs(size.x()));
		size.setY(std::abs(size.y()));
		size.setZ(std::abs(size.z()));
	}

	addDebugBoxLines(midPos, size, col1, col2);
}

void DebugObjects::addDebugBoxLines(Geometry::Vec3 midPos, Geometry::Vec3 size, Util::Color4f col1, Util::Color4f col2){
	Util::Color4f cH1(col1.r() * 0.6f + col2.r() * 0.4f, col1.g() * 0.6f + col2.g() * 0.4f, col1.b() * 0.6f + col2.b() * 0.4f, col1.a() * 0.6f + col2.a() * 0.4f);
	Util::Color4f cH2(col1.r() * 0.4f + col2.r() * 0.6f, col1.g() * 0.4f + col2.g() * 0.6f, col1.b() * 0.4f + col2.b() * 0.6f, col1.a() * 0.4f + col2.a() * 0.6f);
	Geometry::Vec3 sH = size * 0.5f;
	Geometry::Vec3 v0 = midPos - sH;
	Geometry::Vec3 v1(midPos.x() - sH.x(), midPos.y() - sH.y(), midPos.z() + sH.z());
	Geometry::Vec3 v2(midPos.x() - sH.x(), midPos.y() + sH.y(), midPos.z() - sH.z());
	Geometry::Vec3 v3(midPos.x() - sH.x(), midPos.y() + sH.y(), midPos.z() + sH.z());
	Geometry::Vec3 v4(midPos.x() + sH.x(), midPos.y() - sH.y(), midPos.z() - sH.z());
	Geometry::Vec3 v5(midPos.x() + sH.x(), midPos.y() - sH.y(), midPos.z() + sH.z());
	Geometry::Vec3 v6(midPos.x() + sH.x(), midPos.y() + sH.y(), midPos.z() - sH.z());
	Geometry::Vec3 v7 = midPos + sH;
	addDebugLine(v0, v1, col1, cH1);
	addDebugLine(v0, v2, col1, cH1);
	addDebugLine(v0, v4, col1, cH1);
	addDebugLine(v5, v1, cH2, cH1);
	addDebugLine(v5, v4, cH2, cH1);
	addDebugLine(v5, v7, cH2, col2);
	addDebugLine(v3, v1, cH2, cH1);
	addDebugLine(v3, v2, cH2, cH1);
	addDebugLine(v3, v7, cH2, col2);
	addDebugLine(v6, v2, cH2, cH1);
	addDebugLine(v6, v4, cH2, cH1);
	addDebugLine(v6, v7, cH2, col2);
}

void DebugObjects::addDebugBoxLines(Geometry::Vec3 midPos, float size, Util::Color4f col1, Util::Color4f col2){
	addDebugBoxLines(midPos, Geometry::Vec3(size, size, size), col1, col2);
}

void DebugObjects::clearDebug(){
	for(unsigned int i = 0; i < linesData.size(); i++){
		delete linesData[i];
	}
	linesData.clear();

	for(unsigned int i = 0; i < facesData.size(); i++){
		delete facesData[i];
	}
	facesData.clear();
}

void DebugObjects::buildDebugLineNode(){
	if(linesData.size() <= 0) return;

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

	if(!nodeAddedLines){
		nodeAddedLines = true;
		lineNode.get()->setTempNode(true);
		sceneRootNode->addChild(lineNode);
	}
}

void DebugObjects::buildDebugFaceNode(){
	if(facesData.size() <= 0) return;

	Rendering::VertexDescription vertexDesc;
	vertexDesc.appendPosition3D();
	vertexDesc.appendColorRGBAByte();
	Rendering::MeshUtils::MeshBuilder mb(vertexDesc);

	if(facesData.size() <= 0) return;

	for(unsigned int i = 0; i < facesData.size(); i++){
		mb.color(facesData[i]->col1);
		mb.position(facesData[i]->vert1);
		mb.addVertex();

		mb.color(facesData[i]->col2);
		mb.position(facesData[i]->vert2);
		mb.addVertex();

		mb.color(facesData[i]->col3);
		mb.position(facesData[i]->vert3);
		mb.addVertex();

		mb.color(facesData[i]->col3);
		mb.position(facesData[i]->vert3);
		mb.addVertex();

		mb.color(facesData[i]->col4);
		mb.position(facesData[i]->vert4);
		mb.addVertex();

		mb.color(facesData[i]->col1);
		mb.position(facesData[i]->vert1);
		mb.addVertex();
	}

	Rendering::Mesh *mesh = mb.buildMesh();
	mesh->setUseIndexData(false);
	mesh->setDrawMode(Rendering::Mesh::DRAW_TRIANGLES);

	((MinSG::GeometryNode*)faceNode.get())->setMesh(mesh);

	if(!nodeAddedFaces){
		nodeAddedFaces = true;
		faceNode.get()->setTempNode(true);
		sceneRootNode->addChild(faceNode);
	}
}

}
}

#endif /* MINSG_EXT_THESISPETER */
