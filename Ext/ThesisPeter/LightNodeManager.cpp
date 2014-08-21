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
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Helper.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/LightNode.h>
#include <MinSG/Core/FrameContext.h>
#include <random>
#include <Rendering/Serialization/Serialization.h>
#include <MinSG/Core/Nodes/CameraNode.h>
#include <MinSG/Core/Nodes/ListNode.h>

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
const unsigned int LightNodeManager::VOXEL_OCTREE_DEPTH = 7;
const unsigned int LightNodeManager::VOXEL_OCTREE_TEXTURE_SIZE = 1024;
const unsigned int LightNodeManager::VOXEL_OCTREE_SIZE_PER_NODE = 8;

#define USE_ATOMIC_COUNTER

NodeCreaterVisitor::NodeCreaterVisitor(){
	nodeIndex = 0;
}

NodeCreaterVisitor::status NodeCreaterVisitor::leave(MinSG::Node* node){
//	if(typeid(node) == typeid(MinSG::GeometryNode)){
	if(node->getTypeId() == MinSG::GeometryNode::getClassId()){
		//create a lightNodeMap to save the mapping of parameters to the traversed node
		LightNodeMap* lightNodeMap = new LightNodeMap();
		lightNodeMap->geometryNode = static_cast<MinSG::GeometryNode*>(node);
		lightNodeMap->staticNode = true;
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
	} else if(node->getTypeId() == MinSG::LightNode::getClassId()){
		//TODO: implement light nodes
		std::cout << "Found LightNode!" << std::endl;
	} else {
		if(typeid(MinSG::AbstractCameraNode*) == typeid(node)) std::cout << "AbstractCameraNode: " << std::endl;
		if(typeid(MinSG::CameraNode*) == typeid(node)) std::cout << "CameraNode: " << std::endl;
		if(typeid(MinSG::CameraNodeOrtho*) == typeid(node)) std::cout << "CameraNodeOrtho: " << std::endl;
		if(typeid(MinSG::GeometryNode*) == typeid(node)) std::cout << "GeometryNode: " << std::endl;
		if(typeid(MinSG::GroupNode*) == typeid(node)) std::cout << "GroupNode: " << std::endl;
		if(typeid(MinSG::LightNode*) == typeid(node)) std::cout << "LightNode: " << std::endl;
		if(typeid(MinSG::ListNode*) == typeid(node)) std::cout << "ListNode: " << std::endl;
		if(typeid(MinSG::Node*) == typeid(node)) std::cout << "Node: " << std::endl;
		std::cout << "But itself says it's " << node->getTypeName() << std::endl;
	}

	return CONTINUE_TRAVERSAL;
}

NodeRenderVisitor::NodeRenderVisitor(){

}

void NodeRenderVisitor::init(MinSG::FrameContext* frameContext){
	this->frameContext = frameContext;
	renderParam.setFlags(MinSG::USE_WORLD_MATRIX | MinSG::FRUSTUM_CULLING);
}

NodeRenderVisitor::status NodeRenderVisitor::leave(MinSG::Node* node){
	//travel only geometry nodes
	if(node->getTypeId() == MinSG::GeometryNode::getClassId()){
		frameContext->displayNode(node, renderParam);
	}

	return CONTINUE_TRAVERSAL;
}

LightNodeManager::LightNodeManager(){
	for(unsigned int i = 0; i < 3; i++){
		sceneEnclosingCameras[i] = new MinSG::CameraNodeOrtho();
	}
	debug = new DebugObjects();

	voxelOctreeTextureStatic = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, VOXEL_OCTREE_TEXTURE_SIZE, VOXEL_OCTREE_TEXTURE_SIZE, 1, Util::TypeConstant::UINT32, 1);
	voxelOctreeLocksStatic = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, VOXEL_OCTREE_TEXTURE_SIZE / VOXEL_OCTREE_SIZE_PER_NODE, VOXEL_OCTREE_TEXTURE_SIZE / VOXEL_OCTREE_SIZE_PER_NODE, 1, Util::TypeConstant::UINT32, 1);
#ifdef USE_ATOMIC_COUNTER
	atomicCounter = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_BUFFER, 1, 1, 1, Util::TypeConstant::UINT32, 1);
#else
	atomicCounter = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_1D, 1, 1, 1, Util::TypeConstant::UINT32, 1);
#endif // USE_ATOMIC_COUNTER

	voxelOctreeShaderCreate = Rendering::Shader::loadShader(Util::FileName("./extPlugins/ThesisPeter/voxelOctreeInit.vs"), Util::FileName("./extPlugins/ThesisPeter/voxelOctreeInit.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
	voxelOctreeShaderRead = Rendering::Shader::loadShader(Util::FileName("./extPlugins/ThesisPeter/voxelOctreeReadTexture.vs"), Util::FileName("./extPlugins/ThesisPeter/voxelOctreeReadTexture.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
}

LightNodeManager::~LightNodeManager(){
	cleanUpDebug();
	cleanUp();
}

void LightNodeManager::test(MinSG::FrameContext& frameContext, Util::Reference<MinSG::Node> sceneRootNode){
//	if(sceneRootNode.get()->getTypeId() == MinSG::GeometryNode::getClassId()){
//		MinSG::RenderParam renderParam;
//		renderParam.setFlags(MinSG::USE_WORLD_MATRIX | MinSG::FRUSTUM_CULLING);
//		std::cout << "Before" << std::endl;
//		frameContext.displayNode(sceneRootNode.get(), renderParam);
//		std::cout << "After" << std::endl;
//	}
}

void LightNodeManager::setSceneRootNode(Util::Reference<MinSG::Node> sceneRootNode){
	this->sceneRootNode = sceneRootNode;
	debug->setSceneRootNode(sceneRootNode.get());
}

void LightNodeManager::setRenderingContext(Rendering::RenderingContext& renderingContext){
	this->renderingContext = &renderingContext;

	//set the images to the shader
	voxelOctreeShaderCreate.get()->setUniform(renderingContext, Rendering::Uniform("voxelOctree", (int32_t)0));
	voxelOctreeShaderCreate.get()->setUniform(renderingContext, Rendering::Uniform("voxelOctreeLocks", (int32_t)1));
#ifndef USE_ATOMIC_COUNTER
	voxelOctreeShaderCreate.get()->setUniform(renderingContext, Rendering::Uniform("curVoxelOctreeIndex", (int32_t)2));
#endif // USE_ATOMIC_COUNTER

	voxelOctreeShaderRead.get()->setUniform(renderingContext, Rendering::Uniform("voxelOctree", (int32_t)0));
	voxelOctreeShaderRead.get()->setUniform(renderingContext, Rendering::Uniform("edges", (int32_t)1));
#ifndef USE_ATOMIC_COUNTER
	voxelOctreeShaderCreate.get()->setUniform(renderingContext, Rendering::Uniform("curEdgeIndex", (int32_t)2));
#endif // USE_ATOMIC_COUNTER
}

void LightNodeManager::setFrameContext(MinSG::FrameContext& frameContext){
	this->frameContext = &frameContext;
}

unsigned int LightNodeManager::addTreeToDebug(Geometry::Vec3 parentPos, float parentSize, unsigned int depth, unsigned int curID, Util::PixelAccessor* pixelAccessor){
	Util::Color4f boxColor(1, 0, 1, 1);
	Util::Color4f boxColor2(1, 0, 0, 1);
	unsigned int counter = 1;

	if(depth < VOXEL_OCTREE_DEPTH){
		boxColor = Util::Color4f(1, 1, 1, 1);
		boxColor2 = Util::Color4f(0, 0, 1, 1);
		for(unsigned int i = 0; i < 8; i++){
			unsigned int x = (curID * VOXEL_OCTREE_SIZE_PER_NODE + i) % pixelAccessor->getWidth();
			unsigned int y = (curID * VOXEL_OCTREE_SIZE_PER_NODE + i) / pixelAccessor->getWidth();
			unsigned int childID = pixelAccessor->readColor4f(x, y).r();

			if(childID != 0){
				Geometry::Vec3 childPos = parentPos;
				switch(i){
				case 0:	childPos += Geometry::Vec3(-parentSize * 0.25f, -parentSize * 0.25f, parentSize * 0.25f); break;
				case 1:	childPos += Geometry::Vec3(-parentSize * 0.25f, -parentSize * 0.25f, -parentSize * 0.25f); break;
				case 2:	childPos += Geometry::Vec3(-parentSize * 0.25f, parentSize * 0.25f, parentSize * 0.25f); break;
				case 3:	childPos += Geometry::Vec3(-parentSize * 0.25f, parentSize * 0.25f, -parentSize * 0.25f); break;
				case 4:	childPos += Geometry::Vec3(parentSize * 0.25f, -parentSize * 0.25f, parentSize * 0.25f); break;
				case 5:	childPos += Geometry::Vec3(parentSize * 0.25f, -parentSize * 0.25f, -parentSize * 0.25f); break;
				case 6:	childPos += Geometry::Vec3(parentSize * 0.25f, parentSize * 0.25f, parentSize * 0.25f); break;
				case 7:	childPos += Geometry::Vec3(parentSize * 0.25f, parentSize * 0.25f, -parentSize * 0.25f); break;
				}
				//draw line
				debug->addDebugLine(parentPos, childPos, Util::Color4f(0, 1, 0, 1), Util::Color4f(0, 0.5f, 1, 1));

				counter += addTreeToDebug(childPos, parentSize * 0.5f, depth + 1, childID, pixelAccessor);
			}
		}
	}

	//draw box (as lines)
	float mov = parentSize * 0.5f;
	debug->addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() + mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() + mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() - mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() - mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() - mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() + mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() + mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() + mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() - mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() - mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() - mov), boxColor, boxColor2);
	debug->addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() + mov), boxColor, boxColor2);

	return counter;
}

//here begins the fun
void LightNodeManager::activateLighting(Util::Reference<MinSG::Node> sceneRootNode, Util::Reference<MinSG::Node> lightRootNode, Rendering::RenderingContext& renderingContext, MinSG::FrameContext& frameContext){
	Rendering::enableGLErrorChecking();
	//reset some values
	cleanUp();
	fillTexture(voxelOctreeTextureStatic.get(), 0);
	fillTexture(voxelOctreeLocksStatic.get(), 0);
	fillTexture(atomicCounter.get(), 0);
	debug->clearDebug();

	//some needed variables (for rendering etc.)
	setSceneRootNode(sceneRootNode);
	setLightRootNode(lightRootNode);
	setRenderingContext(renderingContext);
	setFrameContext(frameContext);

	//create the cameras to render the scene orthogonally from each axis
	createWorldBBCameras();
    //create the VoxelOctree from static objects
	buildVoxelOctree(voxelOctreeTextureStatic.get(), atomicCounter.get(), voxelOctreeLocksStatic.get());

	//DEBUG to test the texture size!
	atomicCounter.get()->downloadGLTexture(renderingContext);
	Rendering::checkGLError(__FILE__, __LINE__);
	Util::Reference<Util::PixelAccessor> counterAcc = Rendering::TextureUtils::createColorPixelAccessor(renderingContext, *atomicCounter.get());
	Rendering::checkGLError(__FILE__, __LINE__);
	Util::Color4f numNodes = counterAcc.get()->readColor4f(0, 0);
	if(VOXEL_OCTREE_TEXTURE_SIZE * VOXEL_OCTREE_TEXTURE_SIZE < numNodes.r()){
		std::cout << "ERROR: Texture is too small: " << (numNodes.r() * VOXEL_OCTREE_SIZE_PER_NODE) << " > " << (VOXEL_OCTREE_TEXTURE_SIZE * VOXEL_OCTREE_TEXTURE_SIZE) << std::endl;
	} else {
		std::cout << "Texture is big enough: " << (numNodes.r() * VOXEL_OCTREE_SIZE_PER_NODE) << " <= " << (VOXEL_OCTREE_TEXTURE_SIZE * VOXEL_OCTREE_TEXTURE_SIZE) << std::endl;
	}
	Rendering::checkGLError(__FILE__, __LINE__);

	voxelOctreeTextureStatic.get()->downloadGLTexture(renderingContext);
	Rendering::checkGLError(__FILE__, __LINE__);
	Util::Reference<Util::PixelAccessor> voxelOctreeAcc = Rendering::TextureUtils::createColorPixelAccessor(renderingContext, *voxelOctreeTextureStatic.get());
//	Rendering::checkGLError(__FILE__, __LINE__);
//	for(unsigned int i = 0; i < numNodes.r() * VOXEL_OCTREE_SIZE_PER_NODE || i < 16; i++){
//		unsigned int x = i % voxelOctreeTextureStatic.get()->getWidth();
//		unsigned int y = i / voxelOctreeTextureStatic.get()->getWidth();
//		if(i < 100 * VOXEL_OCTREE_SIZE_PER_NODE /*i % (1 * VOXEL_OCTREE_SIZE_PER_NODE) == 0*/) std::cout << "VoxelOctree value " << i << ": " << voxelOctreeAcc.get()->readColor4f(x, y).r() << std::endl;
//	}

	//draw tree debug
	Geometry::Vec3 rootNodeMidpoint = lightRootNode.get()->getWorldOrigin();
	rootNodeMidpoint.setY(rootNodeMidpoint.y() + lightRootNode->getWorldBB().getExtentMax() * 0.5f);
	std::cout << "Number debug nodes: " << addTreeToDebug(rootNodeMidpoint, lightRootNode->getWorldBB().getExtentMax(), 0, 0, voxelOctreeAcc.get()) << std::endl;

	Rendering::checkGLError(__FILE__, __LINE__);
	//DEBUG END

	createLightNodes();

	//DEBUG to show the nodes
//	debug->addDebugLine(Geometry::Vec3(0, 0, 0), Geometry::Vec3(0, 3, 0), Util::Color4f(1, 1, 1, 1), Util::Color4f(1, 0, 0, 1));
//	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
//		for(unsigned int j = 0; j < lightNodeMaps[i]->lightNodes.size(); j++){
//			Geometry::Vec3 pos = lightNodeMaps[i]->lightNodes[j]->position;
//			debug->addDebugLine(pos, Geometry::Vec3(pos.x(), pos.y() + 0.1, pos.z()));
//		}
//	}
	//DEBUG END

	//reset atomicCounter
	fillTexture(atomicCounter.get(), 0);
	createLightEdges(atomicCounter.get());

	//DEBUG to show the edges
//	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
//		for(unsigned int j = 0; j < lightNodeMaps[i]->internalLightEdges.size(); j++){
//			debug->addDebugLine(lightNodeMaps[i]->internalLightEdges[j]->source->position, lightNodeMaps[i]->internalLightEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
//		}
//		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesStatic.size(); j++){
//			for(unsigned int k = 0; k < lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.size(); k++){
//				debug->addDebugLine(lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->source->position, lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->target->position, Util::Color4f(0, 1, 0.5f, 1), Util::Color4f(0, 0.5f, 1, 1));
//			}
//		}
//	}
	//DEBUG END

	debug->buildDebugLineNode();
	Rendering::checkGLError(__FILE__, __LINE__);
}

void LightNodeManager::createLightNodes(){
	NodeCreaterVisitor createNodes;
	createNodes.nodeIndex = 0;
	createNodes.lightNodeMaps = &lightNodeMaps;
	lightRootNode->traverse(createNodes);
//	sceneRootNode->traverse(createNodes);
}

void LightNodeManager::createLightNodes(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes){
	createLightNodesPerVertexRandom(node, lightNodes, 0.1f);
}

void LightNodeManager::mapLightNodesToObject(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes){
	mapLightNodesToObjectClosest(node, lightNodes);
}

void LightNodeManager::createLightEdges(Rendering::Texture* atomicCounter){
	//TODO: speed up (e.g. with octree)
	for(unsigned int lightNodeMapID = 0; lightNodeMapID < lightNodeMaps.size(); lightNodeMapID++){
		//create (possible) internal edges
		for(unsigned int lightNodeSourceID = 0; lightNodeSourceID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeSourceID++){
			for(unsigned int lightNodeTargetID = lightNodeSourceID + 1; lightNodeTargetID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeTargetID++){
				LightNode* source = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeSourceID];
				LightNode* target = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeTargetID];
				addLightEdge(source, target, &lightNodeMaps[lightNodeMapID]->internalLightEdges);
			}
		}
		filterIncorrectEdges(&lightNodeMaps[lightNodeMapID]->internalLightEdges, voxelOctreeTextureStatic.get(), atomicCounter);

		//create (possible) external edges to other objects
		for(unsigned int lightNodeMapID2 = lightNodeMapID + 1; lightNodeMapID2 < lightNodeMaps.size(); lightNodeMapID2++){
			for(unsigned int lightNodeSourceID = 0; lightNodeSourceID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeSourceID++){
				for(unsigned int lightNodeTargetID = 0; lightNodeTargetID < lightNodeMaps[lightNodeMapID2]->lightNodes.size(); lightNodeTargetID++){
					LightNode* source = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeSourceID];
					LightNode* target = lightNodeMaps[lightNodeMapID2]->lightNodes[lightNodeTargetID];
					LightNodeMapConnection* mapConnection = new LightNodeMapConnection();
					mapConnection->map1 = lightNodeMaps[lightNodeMapID];
					mapConnection->map2 = lightNodeMaps[lightNodeMapID2];
					if(lightNodeMaps[lightNodeMapID]->staticNode == true && lightNodeMaps[lightNodeMapID2]->staticNode == true){
						lightNodeMaps[lightNodeMapID]->externalLightEdgesStatic.push_back(mapConnection);
					} else {
						lightNodeMaps[lightNodeMapID]->externalLightEdgesDynamic.push_back(mapConnection);
					}
					addLightEdge(source, target, &mapConnection->edges);
				}
			}
		}

		//TODO: filter the static edges with the static voxelOctree
	}
}

void LightNodeManager::cleanUpDebug(){
	if(debug != 0){
		delete debug;
		debug = 0;
	}
}

void LightNodeManager::cleanUp(){
//	for(unsigned int i = 0; i < 3; i++){
//		if(sceneEnclosingCameras[i] != 0){
//			delete sceneEnclosingCameras[i];
//			sceneEnclosingCameras[i] = 0;
//		}
//	}
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
		for(unsigned int j = 0; j < lightNodeMaps[i]->lightNodes.size(); j++){
            delete lightNodeMaps[i]->lightNodes[j];
		}
		for(unsigned int j = 0; j < lightNodeMaps[i]->internalLightEdges.size(); j++){
            delete lightNodeMaps[i]->internalLightEdges[j];
		}
		//removing external edges is more difficult, since not every node may delete them, only one
		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesStatic.size(); j++){
			if(lightNodeMaps[i]->externalLightEdgesStatic[j]->map1 == lightNodeMaps[i]){
				if(lightNodeMaps[i]->externalLightEdgesStatic[j]->map2 == 0){
					delete lightNodeMaps[i]->externalLightEdgesStatic[j];
				} else {
					for(int k = 0; k < lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.size(); k++){
						delete lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k];
					}
					lightNodeMaps[i]->externalLightEdgesStatic[j]->map1 = 0;
				}
			} else {
				if(lightNodeMaps[i]->externalLightEdgesStatic[j]->map1 == 0){
					delete lightNodeMaps[i]->externalLightEdgesStatic[j];
				} else {
					for(int k = 0; k < lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.size(); k++){
						delete lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k];
					}
					lightNodeMaps[i]->externalLightEdgesStatic[j]->map2 = 0;
				}
			}
		}
		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesDynamic.size(); j++){
			if(lightNodeMaps[i]->externalLightEdgesDynamic[j]->map1 == lightNodeMaps[i]){
				if(lightNodeMaps[i]->externalLightEdgesDynamic[j]->map2 == 0){
					delete lightNodeMaps[i]->externalLightEdgesDynamic[j];
				} else {
					for(int k = 0; k < lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges.size(); k++){
						delete lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k];
					}
					lightNodeMaps[i]->externalLightEdgesDynamic[j]->map1 = 0;
				}
			} else {
				if(lightNodeMaps[i]->externalLightEdgesDynamic[j]->map1 == 0){
					delete lightNodeMaps[i]->externalLightEdgesDynamic[j];
				} else {
					for(int k = 0; k < lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges.size(); k++){
						delete lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k];
					}
					lightNodeMaps[i]->externalLightEdgesDynamic[j]->map2 = 0;
				}
			}
		}
		delete lightNodeMaps[i];
	}
	lightNodeMaps.clear();
}

unsigned int LightNodeManager::nextPowOf2(unsigned int number){
    --number;
    number |= number >> 1;
    number |= number >> 2;
    number |= number >> 4;
    number |= number >> 8;
    number |= number >> 16;
    return number + 1;
}

void getTexCoords(unsigned int index, unsigned int texWidth, Geometry::Vec2i* texCoords){
    texCoords->x(index % texWidth);
    texCoords->y((int)(index / texWidth));
}

void LightNodeManager::setLightRootNode(Util::Reference<MinSG::Node> rootNode){
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
			Geometry::Vec4 pos(lightNode->position.x(), lightNode->position.y(), lightNode->position.z(), 1);
			pos = node->getWorldMatrix() * pos;
			lightNode->position = Geometry::Vec3(pos.x(), pos.y(), pos.z());
			lightNode->normal = norAcc->getNormal(i);
			lightNode->color = colAcc->getColor4f(i);
			lightNodes->push_back(lightNode);
		}
	}

	if(lightNodes->empty() && mesh->getVertexCount() != 0){
		LightNode* lightNode = new LightNode();
		lightNode->position = posAcc->getPosition(0);
		Geometry::Vec4 pos(lightNode->position.x(), lightNode->position.y(), lightNode->position.z(), 1);
		pos = node->getWorldMatrix() * pos;
		lightNode->position = Geometry::Vec3(pos.x(), pos.y(), pos.z());
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

void LightNodeManager::addLightEdge(LightNode* source, LightNode* target, std::vector<LightEdge*>* lightEdges){
	//take maximum edge length into account
	float distance = source->position.distance(target->position);
	if(distance <= MAX_EDGE_LENGTH){
		if(isVisible(source, target)){
			LightEdge* lightEdge = new LightEdge();
			lightEdge->source = source;
			lightEdge->target = target;
			lightEdge->weight = 1.0f / (distance * distance);	//quadratic decrease of light
			lightEdges->push_back(lightEdge);
		}
	}
}

void LightNodeManager::filterIncorrectEdges(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter){
	filterIncorrectEdgesAsTexture(edges, octreeTexture, atomicCounter);
}

void LightNodeManager::filterIncorrectEdgesAsTexture(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter){
	//some unneeded texture to activate the rendering
	Util::Reference<Rendering::Texture> unneeded = Rendering::TextureUtils::createStdTexture(1, 1, false);

	//setup texture to upload the edges onto the gpu
	const unsigned int dataLengthPerEdge = 6;
	unsigned int textureSize = std::ceil(std::sqrt(edges->size() * dataLengthPerEdge));
	Util::Reference<Rendering::Texture> edgeInput = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, textureSize, textureSize, 1, Util::TypeConstant::FLOAT, 1);

	std::cout << "comming here 1" << std::endl;
	std::cout << "edges: " << edges->size() << std::endl;
	std::cout << "texSize: " << textureSize << std::endl;
	std::cout << "texture: " << edgeInput.get()->getWidth() << "x" << edgeInput.get()->getHeight() << " = " << edgeInput.get()->getDataSize() << std::endl;

	//encode the edges into the texture (writing the positions of the source and target)
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeInput.get());
	for(unsigned int i = 0; i < edges->size(); i++){
		unsigned int i2 = i * dataLengthPerEdge;
		unsigned int x = i2 % textureSize;
		unsigned int y = i2 / textureSize;
		acc.get()->writeColor(x, y, Util::Color4f((*edges)[i]->source->position.x(), 0, 0, 0));
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, Util::Color4f((*edges)[i]->source->position.y(), 0, 0, 0));
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, Util::Color4f((*edges)[i]->source->position.z(), 0, 0, 0));
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, Util::Color4f((*edges)[i]->target->position.x(), 0, 0, 0));
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, Util::Color4f((*edges)[i]->target->position.y(), 0, 0, 0));
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, Util::Color4f((*edges)[i]->target->position.z(), 0, 0, 0));
	}

//	Util::Reference<Rendering::Texture> bla = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, 10, 10, 1, Util::TypeConstant::FLOAT, 1);
//	fillTextureFloat(edgeInput.get(), 5.0f);
//
//	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *bla.get());
//	for(unsigned int y = 0; y < bla.get()->getHeight(); y++){
//		for(unsigned int x = 0; x < bla.get()->getWidth(); x++){
//			std::cout << "blaaa: " << acc.get()->readSingleValueFloat(x, y) << std::endl;
//		}
//	}

	//create the output texture (one pixel per edge, 0 = ok, 1 = edge must be deleted)
	unsigned int outputTextureSize = std::ceil(std::sqrt(edges->size()));
	Util::Reference<Rendering::Texture> edgeOutput = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, outputTextureSize, outputTextureSize, 1, Util::TypeConstant::UINT8, 1);
	fillTexture(edgeOutput.get(), 0);

	std::cout << "Output texture: " << outputTextureSize << " which gives " << outputTextureSize * outputTextureSize << " possible edges!" << std::endl;

	Geometry::Vec3 rootNodeMidpoint = lightRootNode.get()->getWorldOrigin();
	rootNodeMidpoint.setY(rootNodeMidpoint.y() + lightRootNode->getWorldBB().getExtentMax() * 0.5f);
	debug->addDebugLine(rootNodeMidpoint, Geometry::Vec3(rootNodeMidpoint.x(), rootNodeMidpoint.y() + 1, rootNodeMidpoint.z()), Util::Color4f(0, 1, 0, 1), Util::Color4f(0, 1, 0, 1));
	std::cout << "Root Node Midpoint: " << rootNodeMidpoint.x() << "x" << rootNodeMidpoint.y() << "x" << rootNodeMidpoint.z() << std::endl;
	voxelOctreeShaderRead.get()->setUniform(*renderingContext, Rendering::Uniform("rootMidPos", rootNodeMidpoint));
	voxelOctreeShaderRead.get()->setUniform(*renderingContext, Rendering::Uniform("quarterSizeOfRootNode", lightRootNode.get()->getWorldBB().getExtentMax() * 0.25f));
	voxelOctreeShaderRead.get()->setUniform(*renderingContext, Rendering::Uniform("numEdges", (int32_t)edges->size()));
	voxelOctreeShaderRead.get()->setUniform(*renderingContext, Rendering::Uniform("inputTextureSize", (int32_t)textureSize));
	voxelOctreeShaderRead.get()->setUniform(*renderingContext, Rendering::Uniform("outputTextureSize", (int32_t)outputTextureSize));
	voxelOctreeShaderRead.get()->setUniform(*renderingContext, Rendering::Uniform("voxelOctreeTextureSize", (int32_t)octreeTexture->getWidth()));

	std::cout << "comming here 4" << std::endl;

	renderingContext->pushAndSetBoundImage(0, Rendering::ImageBindParameters(octreeTexture));
	renderingContext->pushAndSetBoundImage(1, Rendering::ImageBindParameters(edgeInput.get()));
#ifdef USE_ATOMIC_COUNTER
	renderingContext->pushAndSetAtomicCounterTextureBuffer(0, atomicCounter);
#else
	renderingContext->pushAndSetBoundImage(2, Rendering::ImageBindParameters(atomicCounter));
#endif // USE_ATOMIC_COUNTER

	//activate the shader and check the edges
	TextureProcessor textureProcessor;
	textureProcessor.setRenderingContext(this->renderingContext);
	textureProcessor.setInputTexture(unneeded.get());
	textureProcessor.setOutputTexture(edgeOutput.get());
	textureProcessor.setShader(voxelOctreeShaderRead.get());
	textureProcessor.execute();

	renderingContext->popBoundImage(0);
	renderingContext->popBoundImage(1);
#ifdef USE_ATOMIC_COUNTER
	renderingContext->popAtomicCounterTextureBuffer(0);
#else
	renderingContext->popBoundImage(2);
#endif // USE_ATOMIC_COUNTER

	std::cout << "comming here 5" << std::endl;

	//filter the edge list
	edgeOutput.get()->downloadGLTexture(*renderingContext);
//	uint8_t* data = edgeOutput.get()->openLocalData(*renderingContext);
////	Util::Reference<Util::PixelAccessor> accOut = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeOutput.get());
//	std::vector<LightEdge*> filteredEdges;
////	unsigned int x = 0, y = 0;
//	unsigned int before = 0;
//	for(unsigned int i = 1; i < edges->size(); i++){
////		uint8_t value = accOut.get()->readSingleValuvoid setLightRootNode(MinSG::Node *lightRootNode);eByte(x, y);
////
////		if(value == 0) filteredEdges.push_back((*edges)[i]);
////
////		x = (x + 1) % outputTextureSize;
////		if(x == 0) y++;
//
//		if(data[before] != data[i] - 1){
//			std::cout << i << ": " << (unsigned int)data[i] << " but " << before << ": " << (unsigned int)data[before] << std::endl;
//			before = i;
//			if(i >= 10) break;
//		} else {
//			before = i;
//		}
//
////		if(data[i] == 0) filteredEdges.push_back((*edges)[i]);
//	}
	unsigned int counter = 0;
	Util::Reference<Util::PixelAccessor> acc2 = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeOutput.get());
	for(unsigned int i = 1; i < edges->size(); i++){
		unsigned int x = i % outputTextureSize;
		unsigned int y = i / outputTextureSize;
		unsigned int xb = (i-1) % outputTextureSize;
		unsigned int yb = (i-1) / outputTextureSize;

		uint8_t valueBefore = acc2.get()->readSingleValueByte(xb, yb);
		uint8_t value = acc2.get()->readSingleValueByte(x, y);
		if(valueBefore != value || i % 1000 == 0){
			std::cout << i << " (of " << edges->size() << ")" << ": " << (unsigned int)value << " but " << (i-1) << ": " << (unsigned int)valueBefore << std::endl;
			if(counter++ >= 500) break;
//			if(i >= 10) break;
		}
	}

	std::cout << "comming here 6" << std::endl;

	//write back the filtered edges
//	(*edges) = filteredEdges;
}

void LightNodeManager::fillTexture(Rendering::Texture *texture, Util::Color4f color){
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *texture);
	for(unsigned int y = 0; y < texture->getHeight(); y++){
		for(unsigned int x = 0; x < texture->getWidth(); x++){
			acc.get()->writeColor(x, y, color);
		}
	}
	texture->dataChanged();
}

void LightNodeManager::fillTexture(Rendering::Texture *texture, uint8_t value){
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *texture);
	for(unsigned int y = 0; y < texture->getHeight(); y++){
		for(unsigned int x = 0; x < texture->getWidth(); x++){
			acc.get()->writeColor(x, y, value);
		}
	}
	texture->dataChanged();
}

void LightNodeManager::fillTextureFloat(Rendering::Texture *texture, float value){
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *texture);
	for(unsigned int y = 0; y < texture->getHeight(); y++){
		for(unsigned int x = 0; x < texture->getWidth(); x++){
			acc.get()->writeColor(x, y, Util::Color4f(value, 0, 0, 0));
		}
	}
	texture->dataChanged();
}

void LightNodeManager::createWorldBBCameras(){
	const Geometry::Vec3 worldDirections[3] = {Geometry::Vec3(1, 0, 0), Geometry::Vec3(0, 1, 0), Geometry::Vec3(0, 0, 1)};
	float maxExtend = lightRootNode.get()->getWorldBB().getExtentMax();
	float maxExtendHalf = maxExtend * 0.5f;
//	float diameterHalf = lightRootNode->getWorldBB().getDiameter() * 0.5;
	lightRootCenter = lightRootNode.get()->getWorldBB().getCenter();

	for(unsigned int i = 0; i < 3; i++){
		sceneEnclosingCameras[i].get()->setWorldPosition(lightRootCenter - worldDirections[i] * (maxExtendHalf + 1));
//		camera.setWorldPosition(rootCenter - worldDir * (diameterHalf + 1));
//		camera.setWorldPosition(rootCenter - (worldDir.getNormalized()) * (maxExtend + 1));
		sceneEnclosingCameras[i].get()->rotateToWorldDir(worldDirections[i]);

//		var frustum = Geometry.calcEnclosingOrthoFrustum(lightRootNode.getBB(), camera.getWorldMatrix().inverse() * lightRootNode.getWorldMatrix());
//		camera.setNearFar(frustum.getNear() * 0.99, frustum.getFar() * 1.01);

//		outln("fnear="+frustum.getNear()+" ffar="+frustum.getFar()+" ext="+maxExtend);

		//DEBUG
		Geometry::Vec3 up;
		if(i == 1){
			up = Geometry::Vec3(1, 0, 0);
		} else {
			up = Geometry::Vec3(0, 1, 0);
		}
		Geometry::Vec3 right = worldDirections[i].cross(up);
//		var offset = worldDir * frustum.getNear() * 0.99;
//		var offsetFar = worldDir * frustum.getFar() * 1.01;
		Geometry::Vec3 offset = worldDirections[i];
		Geometry::Vec3 offsetFar = worldDirections[i] * (maxExtend + 1);
//		outln("right: "+right+" offset: "+offset+" offsetFar: "+offsetFar);
		float cl;
		float cr;
		float ct;
		float cb;
		//DEBUG END

//		if(frustum.getRight() - frustum.getLeft() > frustum.getTop() - frustum.getBottom()){
//			camera.rotateLocal_deg(90, new Geometry.Vec3(0, 0, 1));
//			camera.setClippingPlanes(frustum.getBottom(), frustum.getTop(), frustum.getLeft(), frustum.getRight());
//			cl = frustum.getBottom();
//			cr = frustum.getTop();
//			cb = frustum.getLeft();
//			ct = frustum.getRight();
//		} else {
//			camera.setClippingPlanes(frustum.getLeft(), frustum.getRight(), frustum.getBottom(), frustum.getTop());
//			cl = frustum.getLeft();
//			cr = frustum.getRight();
//			cb = frustum.getBottom();
//			ct = frustum.getTop();
//		}

		sceneEnclosingCameras[i].get()->setNearFar(-1, -maxExtend - 1);
//		camera.setNearFar(maxExtend + 1, 1);
//		camera.setNearFar(1, diameterHalf*2 + 1);
		sceneEnclosingCameras[i].get()->setClippingPlanes(-maxExtendHalf, maxExtendHalf, -maxExtendHalf, maxExtendHalf);
		cl = -maxExtendHalf;
		cr = maxExtendHalf;
		ct = maxExtendHalf;
		cb = -maxExtendHalf;

//		//DEBUG
//		Geometry::Vec3 debPos = sceneEnclosingCameras[i].get()->getWorldPosition();
//		debug->addDebugLine(debPos + offset + right * cl + up * cb, debPos + offset + right * cr + up * cb);
//		debug->addDebugLine(debPos + offset + right * cl + up * cb, debPos + offset + right * cl + up * ct);
//		debug->addDebugLine(debPos + offset + right * cr + up * cb, debPos + offset + right * cr + up * ct);
//		debug->addDebugLine(debPos + offset + right * cl + up * ct, debPos + offset + right * cr + up * ct);
//		debug->addDebugLine(debPos + offsetFar + right * cl + up * cb, debPos + offsetFar + right * cr + up * cb);
//		debug->addDebugLine(debPos + offsetFar + right * cl + up * cb, debPos + offsetFar + right * cl + up * ct);
//		debug->addDebugLine(debPos + offsetFar + right * cr + up * cb, debPos + offsetFar + right * cr + up * ct);
//		debug->addDebugLine(debPos + offsetFar + right * cl + up * ct, debPos + offsetFar + right * cr + up * ct);
////		outln("left: "+cl+" right: "+cr+" top: "+ct+" bottom: "+cb);
//
//		debug->addDebugLine(sceneEnclosingCameras[i].get()->getWorldPosition(), sceneEnclosingCameras[i].get()->getWorldPosition() + offset, Util::Color4f(0.0,0.0,1.0,1.0), Util::Color4f(1.0,0.0,0.0,1.0));
//		debug->addDebugLine(sceneEnclosingCameras[i].get()->getWorldPosition() + offset, sceneEnclosingCameras[i].get()->getWorldPosition() + offsetFar, Util::Color4f(0.0,1.0,0.0,1.0), Util::Color4f(1.0,0.0,0.0,1.0));
//		//DEBUG END
	}
//	debug->addDebugLineCol2(cameras[0].getWorldPosition(), cameras[0].getWorldPosition() + worldDirections[0], new Util.Color4f(0.0,0.0,1.0,1.0), new Util.Color4f(1.0,0.0,0.0,1.0));
//	debug->addDebugLineCol2(cameras[0].getWorldPosition() + worldDirections[0], cameras[0].getWorldPosition() + worldDirections[0] * (maxExtend + 1), new Util.Color4f(0.0,1.0,0.0,1.0), new Util.Color4f(1.0,0.0,0.0,1.0));

//	debug->addDebugLineCol2(new Geometry.Vec3(0, 0, 0), new Geometry.Vec3(1, 0, 0), new Util.Color4f(1, 0, 0, 1), new Util.Color4f(1, 0, 0, 1));
//	debug->addDebugLineCol2(new Geometry.Vec3(0, 0, 0), new Geometry.Vec3(0, 1, 0), new Util.Color4f(0, 1, 0, 1), new Util.Color4f(0, 1, 0, 1));
//	debug->addDebugLineCol2(new Geometry.Vec3(0, 0, 0), new Geometry.Vec3(0, 0, 1), new Util.Color4f(0, 0, 1, 1), new Util.Color4f(0, 0, 1, 1));
}

void LightNodeManager::buildVoxelOctree(Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, Rendering::Texture* octreeLocks){
	//TODO: set correct size of texture and convert to image, instead of texture
	unsigned int textureSize = std::pow(2, VOXEL_OCTREE_DEPTH);
	Util::Reference<Rendering::Texture> outputTex = Rendering::TextureUtils::createStdTexture(textureSize, textureSize, true);

	std::cout << "TextureSize: " << textureSize << std::endl;

	renderingContext->pushAndSetBoundImage(0, Rendering::ImageBindParameters(octreeTexture));
	renderingContext->pushAndSetBoundImage(1, Rendering::ImageBindParameters(octreeLocks));
#ifdef USE_ATOMIC_COUNTER
	renderingContext->pushAndSetAtomicCounterTextureBuffer(0, atomicCounter);
#else
	renderingContext->pushAndSetBoundImage(2, Rendering::ImageBindParameters(atomicCounter));
#endif // USE_ATOMIC_COUNTER

	Geometry::Vec3 rootNodeMidpoint = lightRootNode.get()->getWorldOrigin();
	rootNodeMidpoint.setY(rootNodeMidpoint.y() + lightRootNode->getWorldBB().getExtentMax() * 0.5f);
	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("rootMidPos", rootNodeMidpoint));
	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("quarterSizeOfRootNode", lightRootNode.get()->getWorldBB().getExtentMax() * 0.25f));
	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("textureWidth", (int32_t)octreeTexture->getWidth()));
	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("lockTextureWidth", (int32_t)octreeLocks->getWidth()));

	//activate new camera
	frameContext->pushCamera();
	//activate new culling
	renderingContext->pushCullFace();
	for(unsigned int i = 0; i < 1; i++){
		frameContext->setCamera(sceneEnclosingCameras[i].get());
//		voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("inverseMat", sceneEnclosingCameras[i]->getMatrix().inverse()));

		//write into a texture
		TextureProcessor textureProcessor;
		textureProcessor.setRenderingContext(renderingContext);
		textureProcessor.setOutputTexture(outputTex.get());
		textureProcessor.setShader(voxelOctreeShaderCreate.get());
		textureProcessor.begin();

		renderingContext->clearScreen(Util::Color4f(1, 1, 1, 1));

		renderingContext->setCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters::CULL_BACK));
		renderAllNodes(lightRootNode.get());
		renderingContext->pushAndSetCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters::CULL_FRONT));
		renderAllNodes(lightRootNode.get());

		textureProcessor.end();
	}

	//DEBUG output of the unneeded data as png
	outputTex.get()->downloadGLTexture(*renderingContext);
	Rendering::Serialization::saveTexture(*renderingContext, outputTex.get(), Util::FileName("screens/voxelOctreeTex.png"));
//	Rendering::TextureUtils::drawTextureToScreen(*renderingContext, Geometry::Rect_i(0, 0, textureSize, textureSize), *outputTex.get(), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
	//DEBUG END

//	Util::Reference<Util::PixelAccessor> acc2 = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *octreeTexture);
//	std::cout << "Number Nodes: " << acc2->getWidth() * acc2->getHeight() / VOXEL_OCTREE_SIZE_PER_NODE << std::endl;
//	for(unsigned int i = 0; i < acc2->getWidth() * acc2->getHeight() / VOXEL_OCTREE_SIZE_PER_NODE; i++){
//		unsigned int x = i % acc2->getWidth();
//		unsigned int y = i / acc2->getWidth();
//		acc2->writeColor(x, y, Util::Color4f(i + 1, 0, 0, 0));
//	}

	//reset culling
	renderingContext->popCullFace();
	//reset camera
	frameContext->popCamera();

	renderingContext->popBoundImage(0);
	renderingContext->popBoundImage(1);
#ifdef USE_ATOMIC_COUNTER
	renderingContext->popAtomicCounterTextureBuffer(0);
#else
	renderingContext->popBoundImage(2);
#endif // USE_ATOMIC_COUNTER
}

void LightNodeManager::renderAllNodes(MinSG::Node* node){
	NodeRenderVisitor renderVisitor;
	renderVisitor.init(frameContext);
	node->traverse(renderVisitor);
}

}
}

#endif /* MINSG_EXT_THESISPETER */
