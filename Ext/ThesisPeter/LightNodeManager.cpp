/*
	This file is part of the MinSG library extension ThesisPeter.
	Copyright (C) 2014 Peter Stilow

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#define MINSG_EXT_THESISPETER
#ifdef MINSG_EXT_THESISPETER

#include "LightNodeManager.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/LightNode.h>
#include <random>
//#include <GL/glew.h>

namespace Rendering {
const std::string Rendering::LightNodeIndexAttributeAccessor::noAttrErrorMsg("No attribute named '");
const std::string Rendering::LightNodeIndexAttributeAccessor::unimplementedFormatMsg("Attribute format not implemented for attribute '");
}

namespace MinSG {
namespace ThesisPeter {

const Util::StringIdentifier NodeCreaterVisitor::staticNodeIdent = Util::StringIdentifier(NodeAttributeModifier::create("staticNode", NodeAttributeModifier::PRIVATE_ATTRIBUTE));
const Util::StringIdentifier LightNodeManager::lightNodeIDIdent = Util::StringIdentifier(NodeAttributeModifier::create("lightNodeID", NodeAttributeModifier::PRIVATE_ATTRIBUTE));
unsigned int NodeCreaterVisitor::nodeIndex = 0;

const float LightNodeManager::MAX_EDGE_LENGTH = 10.0f;

NodeCreaterVisitor::NodeCreaterVisitor(){
	nodeIndex = 0;
}

NodeCreaterVisitor::status NodeCreaterVisitor::leave(MinSG::Node* node){
	//travel only geometry nodes
	if(typeid(node) == typeid(MinSG::GeometryNode)){
		//create a lightNodeMap to save the mapping of parameters to the traversed node
		LightNodeMap* lightNodeMap = new LightNodeMap();
		lightNodeMap->geometryNode = static_cast<MinSG::GeometryNode*>(node);
		lightNodeMaps->push_back(lightNodeMap);

		//create the light nodes
		LightNodeManager::createLightNodes(lightNodeMap->geometryNode, &lightNodeMap->lightNodes);

		//update the nodes attributes (for inside the shaders)
		LightInfoAttribute* infoAttrib = new LightInfoAttribute();
		infoAttrib->staticNode = true;
		infoAttrib->lightNodeID = nodeIndex;
		nodeIndex += lightNodeMaps->size();
		node->setAttribute(staticNodeIdent, infoAttrib);

		//map the light nodes to the objects
		LightNodeManager::mapLightNodesToObject(lightNodeMap->geometryNode, &lightNodeMap->lightNodes);
	} else if(typeid(node) == typeid(MinSG::LightNode)){
		//TODO: implement light nodes
	}

	return CONTINUE_TRAVERSAL;
}

LightNodeManager::LightNodeManager(){

}

void LightNodeManager::setSceneRootNode(MinSG::Node *sceneRootNode){
	this->sceneRootNode = sceneRootNode;
}

void LightNodeManager::createLightNodes(MinSG::Node *rootNode){
	NodeCreaterVisitor createNodes;
	createNodes.nodeIndex = 0;
    createNodes.lightNodeMaps = &lightNodeMaps;
    setLightRootNode(rootNode);
    rootNode->traverse(createNodes);
}

void LightNodeManager::createLightNodes(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes){
	createLightNodesPerVertexRandom(node, lightNodes, 0.1f);
}

void LightNodeManager::mapLightNodesToObject(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes){
	mapLightNodesToObjectClosest(node, lightNodes);
}

void LightNodeManager::createLightEdges(){
	//TODO: fasten up (e.g. with octree)
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
		//create (possible) internal edges
		for(unsigned int j = 0; j < lightNodeMaps[i]->lightNodes.size(); j++){
            for(unsigned int k = j + 1; k < lightNodeMaps[i]->lightNodes.size(); k++){
            	//take maximum edge length into account
            	LightNode* source = lightNodeMaps[i]->lightNodes[j];
            	LightNode* target = lightNodeMaps[i]->lightNodes[k];
				float distance = source->position.distance(target->position);
				if(distance <= MAX_EDGE_LENGTH){
					if(isVisible(source, target)){
						LightEdge* lightEdge = new LightEdge();
						lightEdge->source = source;
						lightEdge->target = target;
						lightEdge->weight = 1.0f / (distance * distance);	//quadratic decrease of light
						lightNodeMaps[i]->internalLightEdges.push_back(lightEdge);
					}
				}
            }
            filterIncorrectEdges(lightNodeMaps[i]->internalLightEdges);
		}
	}
}

void LightNodeManager::setLightRootNode(MinSG::Node *rootNode){
	lightRootNode = rootNode;
}

void LightNodeManager::createLightNodesPerVertexRandom(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes, float randomVal){
	Rendering::Mesh* mesh = node->getMesh();

	static std::default_random_engine engine;
	std::uniform_real_distribution<float> distribution(0, 1);

	Util::Reference<Rendering::PositionAttributeAccessor> posAcc = Rendering::PositionAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::POSITION);
	Util::Reference<Rendering::NormalAttributeAccessor> norAcc = Rendering::NormalAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::NORMAL);
	Util::Reference<Rendering::ColorAttributeAccessor> colAcc = Rendering::ColorAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::COLOR);

	for(unsigned int i = 0; i < mesh->getVertexCount(); ++i){
		const float rndVal = distribution(engine);
		if(rndVal <= randomVal){
			LightNode* lightNode = new LightNode();
			lightNode->position = posAcc->getPosition(i);
			lightNode->normal = norAcc->getNormal(i);
			lightNode->color = colAcc->getColor4f(i);
			lightNodes->push_back(lightNode);
		}
	}

	if(lightNodes->empty() && mesh->getVertexCount() != 0){
		LightNode* lightNode = new LightNode();
		lightNode->position = posAcc->getPosition(0);
		lightNode->normal = norAcc->getNormal(0);
		lightNode->color = colAcc->getColor4f(0);
		lightNodes->push_back(lightNode);
	}
}

void LightNodeManager::mapLightNodesToObjectClosest(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes){
	Rendering::Mesh* mesh = node->getMesh();

	//add new vertex attribute (if not existent)
	Rendering::VertexDescription description = mesh->getVertexDescription();
	if(!description.hasAttribute(lightNodeIDIdent)){
//		description.appendAttribute(lightNodeIDIdent, 1, GL_UNSIGNED_INT);
		description.appendUnsignedIntAttribute(lightNodeIDIdent, 1);
		Rendering::MeshVertexData& oldData = mesh->openVertexData();
		std::unique_ptr<Rendering::MeshVertexData> newData(Rendering::MeshUtils::convertVertices(oldData, description));
		oldData.swap(*newData);
	}

	Util::Reference<Rendering::PositionAttributeAccessor> posAcc = Rendering::PositionAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::POSITION);
	Util::Reference<Rendering::LightNodeIndexAttributeAccessor> lniAcc = Rendering::LightNodeIndexAttributeAccessor::create(mesh->openVertexData(), lightNodeIDIdent);
	for(unsigned int i = 0; i < mesh->getVertexCount(); ++i){
		unsigned int bestIndex = 0;
		Geometry::Vec3 pos = posAcc->getPosition(i);
		float bestDistance = pos.distanceSquared((*lightNodes)[0]->position);
		for(unsigned int j = 1; j < lightNodes->size(); j++){
			float newDist = pos.distanceSquared((*lightNodes)[j]->position);
			if(bestDistance > newDist){
				bestDistance = newDist;
				bestIndex = j;
			}
		}
		//set lightNodeIndex to vertex
		lniAcc->setLightNodeIndex(i, bestIndex);
	}
}

bool LightNodeManager::isVisible(LightNode* source, LightNode* target){
	Geometry::Vec3 direction = target->position - source->position;
	//quick test, if the angles could fit
	if(target->position.dot(direction) < 0 || source->position.dot(direction) > 0){
		return false;
	} else {
		//TODO: do a correct ray cast or filter later all edges
		return true;
	}
}

void LightNodeManager::filterIncorrectEdges(std::vector<LightEdge*> edges){

}

}
}

#endif /* MINSG_EXT_THESISPETER */
