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
#include <Util/Graphics/PixelAccessor.h>
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
const unsigned int LightNodeManager::VOXEL_OCTREE_SIZE_PER_NODE = 9;

#define USE_ATOMIC_COUNTER

NodeCreaterVisitor::NodeCreaterVisitor(){
	nodeIndex = 0;
}

NodeCreaterVisitor::status NodeCreaterVisitor::leave(MinSG::Node* node){
	std::cout << "Traveling Node" << std::endl;
//	if(typeid(node) == typeid(MinSG::GeometryNode)){
	if(node->getTypeId() == MinSG::GeometryNode::getClassId()){
		std::cout << "Found GeometryNode!" << std::endl;
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
	std::cout << "Traveling node" << std::endl;
	if(node->getTypeId() == MinSG::GeometryNode::getClassId()){
		std::cout << "Found GeometryNode" << std::endl;
		frameContext->displayNode(node, renderParam);
		std::cout << "rendered GeometryNode" << std::endl;
	} else {
		std::cout << "lala" << std::endl;
		if(typeid(MinSG::AbstractCameraNode*) == typeid(node)) std::cout << "AbstractCameraNode: " << std::endl;
		if(typeid(MinSG::CameraNode*) == typeid(node)) std::cout << "CameraNode: " << std::endl;
		if(typeid(MinSG::CameraNodeOrtho*) == typeid(node)) std::cout << "CameraNodeOrtho: " << std::endl;
		if(typeid(MinSG::GeometryNode*) == typeid(node)) std::cout << "GeometryNode: " << std::endl;
		if(typeid(MinSG::GroupNode*) == typeid(node)) std::cout << "GroupNode: " << std::endl;
		if(typeid(MinSG::LightNode*) == typeid(node)) std::cout << "LightNode: " << std::endl;
		if(typeid(MinSG::ListNode*) == typeid(node)) std::cout << "ListNode: " << std::endl;
		if(typeid(MinSG::Node*) == typeid(node)) std::cout << "Node: " << std::endl;
	}

//	MinSG::RenderParam renderParam;
//	renderParam.setFlags(MinSG::USE_WORLD_MATRIX | MinSG::FRUSTUM_CULLING);
//	std::cout << "Before" << std::endl;
//	frameContext->displayNode(node, renderParam);
//	std::cout << "After" << std::endl;

	return CONTINUE_TRAVERSAL;
}

LightNodeManager::LightNodeManager(){
	for(unsigned int i = 0; i < 3; i++){
		sceneEnclosingCameras[i] = new MinSG::CameraNodeOrtho();
	}
	debug = new DebugObjects();

	voxelOctreeTextureStatic = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, VOXEL_OCTREE_TEXTURE_SIZE, VOXEL_OCTREE_TEXTURE_SIZE, 1, Util::TypeConstant::UINT32, 1);
#ifdef USE_ATOMIC_COUNTER
	atomicCounter = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_BUFFER, 1, 1, 1, Util::TypeConstant::UINT32, 1);
#else
	atomicCounter = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_1D, 1, 1, 1, Util::TypeConstant::UINT32, 1);
#endif // USE_ATOMIC_COUNTER

	voxelOctreeShaderCreate = Rendering::Shader::loadShader(Util::FileName("./extPlugins/ThesisPeter/voxelOctreeInit.vs"), Util::FileName("./extPlugins/ThesisPeter/voxelOctreeInit.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);

	voxelOctreeShaderRead = Rendering::Shader::loadShader(Util::FileName("./extPlugins/ThesisPeter/voxelOctreeRead.vs"), Util::FileName("./extPlugins/ThesisPeter/voxelOctreeRead.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
}

LightNodeManager::~LightNodeManager(){
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
	voxelOctreeShaderCreate.get()->setUniform(renderingContext, Rendering::Uniform("voxelOctree", (int32_t)0));
#ifdef USE_ATOMIC_COUNTER
    //no need to do anything here?
#else
	voxelOctreeShaderCreate.get()->setUniform(renderingContext, Rendering::Uniform("curVoxelOctreeIndex", (int32_t)1));
#endif // USE_ATOMIC_COUNTER
}

void LightNodeManager::setFrameContext(MinSG::FrameContext& frameContext){
	this->frameContext = &frameContext;
}

//here begins the fun
void LightNodeManager::activateLighting(Util::Reference<MinSG::Node> sceneRootNode, Util::Reference<MinSG::Node> lightRootNode, Rendering::RenderingContext& renderingContext, MinSG::FrameContext& frameContext){
	//reset some values
	fillTexture(voxelOctreeTextureStatic.get(), 0);
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
	buildVoxelOctree(voxelOctreeTextureStatic.get(), atomicCounter.get());

	//DEBUG to test the texture size!
	atomicCounter.get()->downloadGLTexture(renderingContext);
	Rendering::checkGLError(__FILE__, __LINE__);
	Util::Reference<Util::PixelAccessor> counterAcc = Rendering::TextureUtils::createColorPixelAccessor(renderingContext, *atomicCounter.get());
	Rendering::checkGLError(__FILE__, __LINE__);
	Util::Color4f value = counterAcc.get()->readColor4f(0, 0);
	if(VOXEL_OCTREE_TEXTURE_SIZE * VOXEL_OCTREE_TEXTURE_SIZE < value.r()){
		std::cout << "ERROR: Texture is too small: " << (value.r() * VOXEL_OCTREE_SIZE_PER_NODE) << " > " << (VOXEL_OCTREE_TEXTURE_SIZE * VOXEL_OCTREE_TEXTURE_SIZE);
	} else {
		std::cout << "Texture is big enough: " << (value.r() * VOXEL_OCTREE_SIZE_PER_NODE) << " <= " << (VOXEL_OCTREE_TEXTURE_SIZE * VOXEL_OCTREE_TEXTURE_SIZE);
	}
	//DEBUG END

//	createLightNodes();

//	createLightEdges();

	debug->buildDebugLineNode();
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

void LightNodeManager::createLightEdges(){
	//TODO: fasten up (e.g. with octree)
	for(unsigned int lightNodeMapID = 0; lightNodeMapID < lightNodeMaps.size(); lightNodeMapID++){
		//create (possible) internal edges
		for(unsigned int lightNodeSourceID = 0; lightNodeSourceID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeSourceID++){
			for(unsigned int lightNodeTargetID = lightNodeSourceID + 1; lightNodeTargetID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeTargetID++){
				LightNode* source = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeSourceID];
				LightNode* target = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeTargetID];
				addLightEdge(source, target, &lightNodeMaps[lightNodeMapID]->internalLightEdges);
			}
			filterIncorrectEdges(&lightNodeMaps[lightNodeMapID]->internalLightEdges);
		}

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
	cleanUpDebug();
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

void LightNodeManager::filterIncorrectEdges(std::vector<LightEdge*> *edges){
	filterIncorrectEdgesAsTexture(edges);
}

void LightNodeManager::filterIncorrectEdgesAsTexture(std::vector<LightEdge*> *edges){
	//setup texture to upload the edges onto the gpu
	const unsigned int dataLengthPerEdge = 6;
	unsigned int textureSize = std::ceil(std::sqrt(edges->size() * dataLengthPerEdge));
	Util::Reference<Rendering::Texture> edgeInput = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, textureSize, textureSize, 1, Util::TypeConstant::FLOAT, 1);

	//encode the edges into the texture (writing the positions of the source and target)
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeInput.get());
	for(unsigned int i = 0; i < edges->size() * dataLengthPerEdge; i += dataLengthPerEdge){
		unsigned int x = i % textureSize;
		unsigned int y = i / textureSize;
		acc.get()->writeColor(x, y, (*edges)[i]->source->position.x());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, (*edges)[i]->source->position.y());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, (*edges)[i]->source->position.z());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, (*edges)[i]->target->position.x());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, (*edges)[i]->target->position.y());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeColor(x, y, (*edges)[i]->target->position.z());
	}

	//create the output texture (one pixel per edge, 0 = ok, 1 = edge must be deleted)
	unsigned int outputTextureSize = std::ceil(std::sqrt(edges->size()));
	Util::Reference<Rendering::Texture> edgeOutput = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, outputTextureSize, outputTextureSize, 1, Util::TypeConstant::UINT8, 1);
	fillTexture(edgeOutput.get(), 0);

	//activate the shader and check the edges
	TextureProcessor textureProcessor;
	textureProcessor.setRenderingContext(this->renderingContext);
	textureProcessor.setInputTexture(edgeInput.get());
	textureProcessor.setOutputTexture(edgeOutput.get());
	textureProcessor.setShader(voxelOctreeShaderRead.get());
	textureProcessor.execute();

	//filter the edge list
	uint8_t* data = edgeOutput.get()->openLocalData(*renderingContext);
//	Util::Reference<Util::PixelAccessor> accOut = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeOutput.get());
	std::vector<LightEdge*> filteredEdges;
//	unsigned int x = 0, y = 0;
	for(unsigned int i = 0; i < edges->size(); i++){
//		uint8_t value = accOut.get()->readSingleValuvoid setLightRootNode(MinSG::Node *lightRootNode);eByte(x, y);
//
//		if(value == 0) filteredEdges.push_back((*edges)[i]);
//
//		x = (x + 1) % outputTextureSize;
//		if(x == 0) y++;

		if(data[i] == 0) filteredEdges.push_back((*edges)[i]);
	}

	//write back the filtered edges
	(*edges) = filteredEdges;
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

		Geometry::Vec3 debPos = sceneEnclosingCameras[i].get()->getWorldPosition();
		debug->addDebugLine(debPos + offset + right * cl + up * cb, debPos + offset + right * cr + up * cb);
		debug->addDebugLine(debPos + offset + right * cl + up * cb, debPos + offset + right * cl + up * ct);
		debug->addDebugLine(debPos + offset + right * cr + up * cb, debPos + offset + right * cr + up * ct);
		debug->addDebugLine(debPos + offset + right * cl + up * ct, debPos + offset + right * cr + up * ct);
		debug->addDebugLine(debPos + offsetFar + right * cl + up * cb, debPos + offsetFar + right * cr + up * cb);
		debug->addDebugLine(debPos + offsetFar + right * cl + up * cb, debPos + offsetFar + right * cl + up * ct);
		debug->addDebugLine(debPos + offsetFar + right * cr + up * cb, debPos + offsetFar + right * cr + up * ct);
		debug->addDebugLine(debPos + offsetFar + right * cl + up * ct, debPos + offsetFar + right * cr + up * ct);
//		outln("left: "+cl+" right: "+cr+" top: "+ct+" bottom: "+cb);

		debug->addDebugLine(sceneEnclosingCameras[i].get()->getWorldPosition(), sceneEnclosingCameras[i].get()->getWorldPosition() + offset, Util::Color4f(0.0,0.0,1.0,1.0), Util::Color4f(1.0,0.0,0.0,1.0));
		debug->addDebugLine(sceneEnclosingCameras[i].get()->getWorldPosition() + offset, sceneEnclosingCameras[i].get()->getWorldPosition() + offsetFar, Util::Color4f(0.0,1.0,0.0,1.0), Util::Color4f(1.0,0.0,0.0,1.0));
	}
//	debug->addDebugLineCol2(cameras[0].getWorldPosition(), cameras[0].getWorldPosition() + worldDirections[0], new Util.Color4f(0.0,0.0,1.0,1.0), new Util.Color4f(1.0,0.0,0.0,1.0));
//	debug->addDebugLineCol2(cameras[0].getWorldPosition() + worldDirections[0], cameras[0].getWorldPosition() + worldDirections[0] * (maxExtend + 1), new Util.Color4f(0.0,1.0,0.0,1.0), new Util.Color4f(1.0,0.0,0.0,1.0));

//	debug->addDebugLineCol2(new Geometry.Vec3(0, 0, 0), new Geometry.Vec3(1, 0, 0), new Util.Color4f(1, 0, 0, 1), new Util.Color4f(1, 0, 0, 1));
//	debug->addDebugLineCol2(new Geometry.Vec3(0, 0, 0), new Geometry.Vec3(0, 1, 0), new Util.Color4f(0, 1, 0, 1), new Util.Color4f(0, 1, 0, 1));
//	debug->addDebugLineCol2(new Geometry.Vec3(0, 0, 0), new Geometry.Vec3(0, 0, 1), new Util.Color4f(0, 0, 1, 1), new Util.Color4f(0, 0, 1, 1));
}

void LightNodeManager::buildVoxelOctree(Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter){
	//TODO: set correct size of texture and convert to image, instead of texture
	unsigned int textureSize = std::pow(2, VOXEL_OCTREE_DEPTH);
	Util::Reference<Rendering::Texture> outputTex = Rendering::TextureUtils::createStdTexture(textureSize, textureSize, true);

	std::cout << "TextureSize: " << textureSize << std::endl;

	renderingContext->pushAndSetBoundImage(0, Rendering::ImageBindParameters(octreeTexture));
#ifdef USE_ATOMIC_COUNTER
	renderingContext->pushAndSetAtomicCounterTextureBuffer(0, atomicCounter);
#else
	renderingContext->pushAndSetBoundImage(1, Rendering::ImageBindParameters(atomicCounter));
#endif // USE_ATOMIC_COUNTER

	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("sizeOfRootNode", lightRootNode.get()->getWorldBB().getExtentMax() * 0.25f));
	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("textureWidth", (int32_t)VOXEL_OCTREE_TEXTURE_SIZE));

	frameContext->pushCamera();
	for(unsigned int i = 0; i < 1; i++){
		frameContext->setCamera(sceneEnclosingCameras[i].get());

		//write into a texture
		TextureProcessor textureProcessor;
		textureProcessor.setRenderingContext(renderingContext);
		textureProcessor.setOutputTexture(outputTex.get());
		textureProcessor.setShader(voxelOctreeShaderCreate.get());
		textureProcessor.begin();

		renderingContext->clearScreen(Util::Color4f(1, 1, 1, 1));
		renderAllNodes(lightRootNode.get());

		textureProcessor.end();
	}

//	Rendering::showDebugTexture(outputTex);
	outputTex.get()->downloadGLTexture(*renderingContext);
	Rendering::Serialization::saveTexture(*renderingContext, outputTex.get(), Util::FileName("screens/voxelOctreeTex.png"));
//	Rendering::TextureUtils::drawTextureToScreen(*renderingContext, Geometry::Rect_i(0, 0, textureSize, textureSize), *outputTex.get(), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
//	Util::Utils::sleep(1000);

	//reset camera
	frameContext->popCamera();

	renderingContext->popBoundImage(0);
#ifdef USE_ATOMIC_COUNTER
	renderingContext->popAtomicCounterTextureBuffer(0);
#else
	renderingContext->popBoundImage(1);
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
