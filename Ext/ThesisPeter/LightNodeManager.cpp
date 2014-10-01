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
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Helper.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/LightNode.h>
#include <MinSG/Core/FrameContext.h>
#include <MinSG/Helper/DataDirectory.h>
#include <MinSG/Helper/Helper.h>
#include <random>
#include <Rendering/Serialization/Serialization.h>
#include <MinSG/Core/Nodes/CameraNode.h>
#include <MinSG/Core/Nodes/ListNode.h>
#include <MinSG/Core/Transformations.h>
#include <Util/IO/FileLocator.h>
#include <Geometry/Definitions.h>

namespace Rendering {
const std::string Rendering::LightNodeIndexAttributeAccessor::noAttrErrorMsg("No attribute named '");
const std::string Rendering::LightNodeIndexAttributeAccessor::unimplementedFormatMsg("Attribute format not implemented for attribute '");
}

namespace MinSG {
namespace ThesisPeter {

const Util::StringIdentifier NodeCreaterVisitor::staticNodeIdent = Util::StringIdentifier(NodeAttributeModifier::create("staticNode", NodeAttributeModifier::PRIVATE_ATTRIBUTE));
const Util::StringIdentifier LightNodeManager::lightNodeIDIdent("lightNodeID");
DebugObjects LightNodeManager::debug;
unsigned int NodeCreaterVisitor::nodeIndex = 0;

#define TP_ACTIVATE_DEBUG
#define TP_SHOW_OCTREE
#define TP_SHOW_NODES
#define TP_SHOW_EDGES
#define TP_INTERNAL_FILTER_OCTREE

//parameters
const unsigned int LightNodeManager::NUMBER_LIGHT_PROPAGATION_CYCLES = 4;
const float LightNodeManager::PERCENT_NODES = 0.05f;
const float LightNodeManager::MAX_EDGE_LENGTH = 1.0f;
const float LightNodeManager::MIN_EDGE_WEIGHT = 0.00001f;
const float LightNodeManager::MAX_EDGE_LENGTH_LIGHT = 400.0f;
const float LightNodeManager::MIN_EDGE_WEIGHT_LIGHT = 0.00001f;
const unsigned int LightNodeManager::VOXEL_OCTREE_DEPTH = 7;
const unsigned int LightNodeManager::VOXEL_OCTREE_TEXTURE_SIZE = 2048;
const unsigned int LightNodeManager::VOXEL_OCTREE_SIZE_PER_NODE = 8;
float LightNodeManager::LIGHT_STRENGTH = 2000;
float LightNodeManager::LIGHT_STRENGTH_FACTOR = 3000.0f;

unsigned int LightNodeManager::globalNodeCounter = 0;
bool LightNodeManager::SHOW_EDGES = false;
bool LightNodeManager::SHOW_OCTREE = false;

#define USE_ATOMIC_COUNTER

NodeCreaterVisitor::NodeCreaterVisitor(){
	nodeIndex = 0;
}

NodeCreaterVisitor::status NodeCreaterVisitor::leave(MinSG::Node* node){
//	if(typeid(node) == typeid(MinSG::GeometryNode)){
	if(node->getTypeId() == MinSG::GeometryNode::getClassId()){
		if(useGeometryNodes){
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
		}
	} else if(node->getTypeId() == MinSG::LightNode::getClassId()){
		if(useLightNodes){
			LightNodeLightMap* lightNodeLightMap = new LightNodeLightMap();
			lightNodeLightMap->lightNode = static_cast<MinSG::LightNode*>(node);
			lightNodeLightMap->light.position = lightNodeLightMap->lightNode->getWorldOrigin();
			lightNodeLightMap->light.color = lightNodeLightMap->lightNode->getAmbientLightColor();
			lightNodeLightMap->light.normal = Geometry::Vec3(0, 1, 0);	//just no 0 vector, although this should not be used at all for lights
			lightNodeLightMap->light.id = LightNodeManager::globalNodeCounter++;
			lightNodeLightMap->staticNode = true;
			lightNodeLightMaps->push_back(lightNodeLightMap);

			std::cout << "Found LightNode!" << std::endl;
		}
	} else {
//		if(typeid(MinSG::AbstractCameraNode*) == typeid(node)) std::cout << "AbstractCameraNode: " << std::endl;
//		if(typeid(MinSG::CameraNode*) == typeid(node)) std::cout << "CameraNode: " << std::endl;
//		if(typeid(MinSG::CameraNodeOrtho*) == typeid(node)) std::cout << "CameraNodeOrtho: " << std::endl;
//		if(typeid(MinSG::GeometryNode*) == typeid(node)) std::cout << "GeometryNode: " << std::endl;
//		if(typeid(MinSG::GroupNode*) == typeid(node)) std::cout << "GroupNode: " << std::endl;
//		if(typeid(MinSG::LightNode*) == typeid(node)) std::cout << "LightNode: " << std::endl;
//		if(typeid(MinSG::ListNode*) == typeid(node)) std::cout << "ListNode: " << std::endl;
//		if(typeid(MinSG::Node*) == typeid(node)) std::cout << "Node: " << std::endl;
//		std::cout << "But itself says it's " << node->getTypeName() << std::endl;
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
	globalNodeCounter = 0;
	globalMinEdgeWeight = 99999;
	globalMaxEdgeWeight = -99999;

	for(unsigned int i = 0; i < 3; i++){
		sceneEnclosingCameras[i] = new MinSG::CameraNodeOrtho();
	}
//	debug = new DebugObjects();

	unsigned int tmpTextureSize = std::pow(2, VOXEL_OCTREE_DEPTH) * 4;
	tmpTexVoxelOctreeSize = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, tmpTextureSize, tmpTextureSize, 1, Util::TypeConstant::UINT8, 1);
	voxelOctreeTextureStatic = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, VOXEL_OCTREE_TEXTURE_SIZE, VOXEL_OCTREE_TEXTURE_SIZE, 1, Util::TypeConstant::UINT32, 1);
	voxelOctreeTextureComplete = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, VOXEL_OCTREE_TEXTURE_SIZE, VOXEL_OCTREE_TEXTURE_SIZE, 1, Util::TypeConstant::UINT32, 1);
	voxelOctreeLocks = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, VOXEL_OCTREE_TEXTURE_SIZE / VOXEL_OCTREE_SIZE_PER_NODE, VOXEL_OCTREE_TEXTURE_SIZE / VOXEL_OCTREE_SIZE_PER_NODE, 1, Util::TypeConstant::UINT32, 1);
#ifdef USE_ATOMIC_COUNTER
	atomicCounter = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_BUFFER, 1, 1, 1, Util::TypeConstant::UINT32, 1);
#else
	atomicCounter = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_1D, 1, 1, 1, Util::TypeConstant::UINT32, 1);
#endif // USE_ATOMIC_COUNTER

	tmpTexSmallest = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_BUFFER, 1, 1, 1, Util::TypeConstant::UINT8, 1);
	tmpDepthTexSmallest = Rendering::TextureUtils::createDepthTexture(1, 1);

	std::string shaderPath = "./extPlugins/ThesisPeter/";
	voxelOctreeShaderCreate = Rendering::Shader::loadShader(Util::FileName(shaderPath + "voxelOctreeInit.vs"), Util::FileName(shaderPath + "voxelOctreeInit.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
	voxelOctreeShaderReadTexture = Rendering::Shader::loadShader(Util::FileName(shaderPath + "voxelOctreeReadTexture.vs"), Util::FileName(shaderPath + "voxelOctreeReadTexture.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
	voxelOctreeShaderReadObject = Rendering::Shader::loadShader(Util::FileName(shaderPath + "voxelOctreeReadObject.vs"), Util::FileName(shaderPath + "voxelOctreeReadObject.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);
	propagateLightShader = Rendering::Shader::loadShader(Util::FileName(shaderPath + "propagateLight.vs"), Util::FileName(shaderPath + "propagateLight.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);

	lightGraphShader = new ShaderState;
	std::string stdShaderPath = "./plugins/LibRenderingExt/resources/shader/universal3/";
	std::vector<std::string> vsFiles;
	vsFiles.push_back(stdShaderPath + "main.sfn");
	vsFiles.push_back(stdShaderPath + "sgHelpers.sfn");
	vsFiles.push_back(stdShaderPath + "vertexEffect_none.sfn");
	vsFiles.push_back(stdShaderPath + "surfaceProps_matTex.sfn");
	vsFiles.push_back(stdShaderPath + "surfaceEffect_none.sfn");
	vsFiles.push_back(shaderPath + "lighting_phongGraph.sfn");
	vsFiles.push_back(stdShaderPath + "fragmentEffect_none.sfn");
	std::vector<std::string> gsFiles;
	std::vector<std::string> fsFiles(vsFiles);
	initShaderState(lightGraphShader.get(), vsFiles, gsFiles, fsFiles, Rendering::Shader::USE_UNIFORMS, Util::FileLocator());
	graphShaderAssigned = false;
}

LightNodeManager::~LightNodeManager(){
#ifdef TP_ACTIVATE_DEBUG
	cleanUpDebug();
#endif
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
#ifdef TP_ACTIVATE_DEBUG
	debug.setSceneRootNode(sceneRootNode.get());
#endif
}

void LightNodeManager::setRenderingContext(Rendering::RenderingContext& renderingContext){
	this->renderingContext = &renderingContext;

	//set the images to the shader
	voxelOctreeShaderCreate.get()->setUniform(renderingContext, Rendering::Uniform("voxelOctree", (int32_t)0));
	voxelOctreeShaderCreate.get()->setUniform(renderingContext, Rendering::Uniform("voxelOctreeLocks", (int32_t)1));
#ifndef USE_ATOMIC_COUNTER
	voxelOctreeShaderCreate.get()->setUniform(renderingContext, Rendering::Uniform("curVoxelOctreeIndex", (int32_t)2));
#endif // USE_ATOMIC_COUNTER

	voxelOctreeShaderReadTexture.get()->setUniform(renderingContext, Rendering::Uniform("voxelOctree", (int32_t)0));
	voxelOctreeShaderReadTexture.get()->setUniform(renderingContext, Rendering::Uniform("edges", (int32_t)1));
#ifndef USE_ATOMIC_COUNTER
	voxelOctreeShaderReadTexture.get()->setUniform(renderingContext, Rendering::Uniform("curEdgeIndex", (int32_t)2));
#endif // USE_ATOMIC_COUNTER

	voxelOctreeShaderReadObject.get()->setUniform(renderingContext, Rendering::Uniform("voxelOctree", (int32_t)0));
	voxelOctreeShaderReadObject.get()->setUniform(renderingContext, Rendering::Uniform("edgeOutput", (int32_t)1));
	voxelOctreeShaderReadObject.get()->setUniform(renderingContext, Rendering::Uniform("edgeInput", (int32_t)2));

	propagateLightShader->setUniform(renderingContext, Rendering::Uniform("edgeNodes", (int32_t)0));
	propagateLightShader->setUniform(renderingContext, Rendering::Uniform("edgeWeights", (int32_t)1));
	propagateLightShader->setUniform(renderingContext, Rendering::Uniform("sourceColors", (int32_t)2));
	propagateLightShader->setUniform(renderingContext, Rendering::Uniform("targetColors", (int32_t)3));

	lightGraphShader->setUniform(Rendering::Uniform("globalLighting", (int32_t)0));
}

void LightNodeManager::setFrameContext(MinSG::FrameContext& frameContext){
	this->frameContext = &frameContext;
}

unsigned int LightNodeManager::addTreeToDebug(Geometry::Vec3 parentPos, float parentSize, unsigned int depth, unsigned int curID, Util::PixelAccessor* pixelAccessor, unsigned int maxDepth){
	Util::Color4f boxColor(1, 0, 1, 1);
	Util::Color4f boxColor2(1, 0, 0, 1);
	unsigned int counter = 1;

	if(depth < maxDepth){
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
//				debug.addDebugLine(parentPos, childPos, Util::Color4f(0, 1, 0, 1), Util::Color4f(0, 0.5f, 1, 1));

				counter += addTreeToDebug(childPos, parentSize * 0.5f, depth + 1, childID, pixelAccessor, maxDepth);
			}
		}
	}

	if(depth >= maxDepth){
		//draw box (as lines)
//		float mov = parentSize * 0.5f;
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() + mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() + mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() - mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() - mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() - mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() + mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() + mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() + mov, parentPos.z() + mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() + mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() - mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() - mov, parentPos.y() - mov, parentPos.z() - mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() + mov, parentPos.z() - mov), boxColor, boxColor2);
//		debug.addDebugLine(Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() - mov), Geometry::Vec3(parentPos.x() + mov, parentPos.y() - mov, parentPos.z() + mov), boxColor, boxColor2);
		debug.addDebugBox(parentPos, parentSize, boxColor, boxColor2);
	}

	return counter;
}

//here begins the fun
void LightNodeManager::activateLighting(Util::Reference<MinSG::Node> sceneRootNode, Util::Reference<MinSG::Node> lightRootNode, Rendering::RenderingContext& renderingContext, MinSG::FrameContext& frameContext){
	std::cout << "start algorithm" << std::endl;

	Rendering::enableGLErrorChecking();
	//reset some values
	cleanUp();
	fillTexture(voxelOctreeTextureStatic.get(), 0);
	fillTexture(voxelOctreeLocks.get(), 0);
#ifdef TP_ACTIVATE_DEBUG
	debug.clearDebug();
#endif

	std::cout << "initialize data" << std::endl;

	//some needed variables (for rendering etc.)
	setSceneRootNode(sceneRootNode);
	setLightRootNode(lightRootNode);
	setRenderingContext(renderingContext);
	setFrameContext(frameContext);

	std::cout << "create lightNodes" << std::endl;

	//create the light nodes and map them to the vertices
	createLightNodes();

#ifdef TP_ACTIVATE_DEBUG
	std::cout << "debug lightNodes" << std::endl;

	//DEBUG to read the light id's of the mesh
//	for(unsigned int i = 0; i < lightNodeLightMaps.size(); i++){
//		Rendering::Mesh* mesh = lightNodeMaps[i]->geometryNode->getMesh();
//		Util::Reference<Rendering::LightNodeIndexAttributeAccessor> lniAcc = Rendering::LightNodeIndexAttributeAccessor::create(mesh->openVertexData(), lightNodeIDIdent);
//		for(unsigned int j = 0; j < mesh->getVertexCount(); ++j){
//			unsigned int lightNodeID = lniAcc->getLightNodeIndex(j);
//			std::cout << "LightNodeID of " << j << " is " << lightNodeID << std::endl;
//		}
//	}
	//DEBUG END


	//DEBUG to show the nodes
#ifdef TP_SHOW_NODES
//	debug.addDebugLine(Geometry::Vec3(0, 0, 0), Geometry::Vec3(0, 3, 0), Util::Color4f(1, 1, 1, 1), Util::Color4f(1, 0, 0, 1));
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
		for(unsigned int j = 0; j < lightNodeMaps[i]->lightNodes.size(); j++){
			Geometry::Vec3 pos = lightNodeMaps[i]->lightNodes[j]->position;
			debug.addDebugLine(pos, Geometry::Vec3(pos.x(), pos.y() - 0.01, pos.z()));
		}
	}
#endif //TP_SHOW_NODES
	//DEBUG END
#endif //TP_ACTIVATE_DEBUG

	std::cout << "create cameras and octree" << std::endl;

	//create the cameras to render the scene orthogonally from each axis
	createWorldBBCameras(lightRootNode.get(), sceneEnclosingCameras);
    //create the VoxelOctree from static objects
	buildVoxelOctree(voxelOctreeTextureStatic.get(), atomicCounter.get(), 0, voxelOctreeLocks.get(), sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), true, false, true);

	std::cout << "debugging octree" << std::endl;

	//DEBUG to test the texture size!
	atomicCounter.get()->downloadGLTexture(renderingContext);
	Rendering::checkGLError(__FILE__, __LINE__);
	Util::Reference<Util::PixelAccessor> counterAcc = Rendering::TextureUtils::createColorPixelAccessor(renderingContext, *atomicCounter.get());
	Rendering::checkGLError(__FILE__, __LINE__);
	Util::Color4f numNodes = counterAcc.get()->readColor4f(0, 0);
	numberStaticOctreeNodes = (unsigned int)numNodes.r();
	if(VOXEL_OCTREE_TEXTURE_SIZE * VOXEL_OCTREE_TEXTURE_SIZE < numberStaticOctreeNodes * VOXEL_OCTREE_SIZE_PER_NODE){
		std::cout << "ERROR: Texture is too small: " << (numberStaticOctreeNodes * VOXEL_OCTREE_SIZE_PER_NODE) << " > " << (VOXEL_OCTREE_TEXTURE_SIZE * VOXEL_OCTREE_TEXTURE_SIZE) << std::endl;
		cleanUp();
		return;
	} else {
		std::cout << "Texture is big enough: " << (numberStaticOctreeNodes * VOXEL_OCTREE_SIZE_PER_NODE) << " <= " << (VOXEL_OCTREE_TEXTURE_SIZE * VOXEL_OCTREE_TEXTURE_SIZE) << std::endl;
	}
	Rendering::checkGLError(__FILE__, __LINE__);

#ifdef TP_ACTIVATE_DEBUG
	voxelOctreeTextureStatic.get()->downloadGLTexture(renderingContext);
	Rendering::checkGLError(__FILE__, __LINE__);
	Util::Reference<Util::PixelAccessor> voxelOctreeAcc = Rendering::TextureUtils::createColorPixelAccessor(renderingContext, *voxelOctreeTextureStatic.get());
//	Rendering::checkGLError(__FILE__, __LINE__);
//	for(unsigned int i = 0; i < numberStaticOctreeNodes * VOXEL_OCTREE_SIZE_PER_NODE || i < 16; i++){
//		unsigned int x = i % voxelOctreeTextureStatic.get()->getWidth();
//		unsigned int y = i / voxelOctreeTextureStatic.get()->getWidth();
//		if(i < 100 * VOXEL_OCTREE_SIZE_PER_NODE /*i % (1 * VOXEL_OCTREE_SIZE_PER_NODE) == 0*/) std::cout << "VoxelOctree value " << i << ": " << voxelOctreeAcc.get()->readColor4f(x, y).r() << std::endl;
//	}

	//draw tree debug
#ifdef TP_SHOW_OCTREE
	std::cout << "Number debug nodes: " << addTreeToDebug(lightingArea.center, lightingArea.extend, 0, 0, voxelOctreeAcc.get(), VOXEL_OCTREE_DEPTH) << std::endl;
#endif //TP_SHOW_OCTREE

	Rendering::checkGLError(__FILE__, __LINE__);
	//DEBUG END
#endif //TP_ACTIVATE_DEBUG

	//create the VoxelOctree from dynamic objects
	voxelOctreeTextureStatic.get()->downloadGLTexture(renderingContext);
	copyTexture(voxelOctreeTextureStatic.get(), voxelOctreeTextureComplete.get());
	buildVoxelOctree(voxelOctreeTextureComplete.get(), atomicCounter.get(), numberStaticOctreeNodes, voxelOctreeLocks.get(), sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), false, true, true);

	std::cout << "create edges" << std::endl;

	//reset atomicCounter TODO: not really needed for edge-generation?!? so remove?!?
	fillTexture(atomicCounter.get(), 0);
	//create the light edges between the nodes
	createLightEdges(atomicCounter.get());

#ifdef TP_ACTIVATE_DEBUG
	std::cout << "debug edges" << std::endl;

	std::cout << "Min/Max Edge Weight: " << globalMinEdgeWeight << "/" << globalMaxEdgeWeight << std::endl;
	std::cout << "Should give something like " << LIGHT_STRENGTH * NUMBER_LIGHT_PROPAGATION_CYCLES * globalMinEdgeWeight << "/" << LIGHT_STRENGTH * NUMBER_LIGHT_PROPAGATION_CYCLES * globalMaxEdgeWeight << std::endl;

	//DEBUG to show the edges
#ifdef TP_SHOW_EDGES
	//edges from lights
	for(unsigned int i = 0; i < lightNodeLightMaps.size(); i++){
		for(unsigned int j = 0; j < lightNodeLightMaps[i]->staticEdges.size(); j++){
			debug.addDebugLine(lightNodeLightMaps[i]->staticEdges[j]->source->position, lightNodeLightMaps[i]->staticEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
		}
		for(unsigned int j = 0; j < lightNodeLightMaps[i]->dynamicEdges.size(); j++){
			debug.addDebugLine(lightNodeLightMaps[i]->dynamicEdges[j]->source->position, lightNodeLightMaps[i]->dynamicEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
		}
	}
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
		//internal edges
		for(unsigned int j = 0; j < lightNodeMaps[i]->internalLightEdges.size(); j++){
			debug.addDebugLine(lightNodeMaps[i]->internalLightEdges[j]->source->position, lightNodeMaps[i]->internalLightEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
		}
		//external edges
		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesStatic.size(); j++){
			for(unsigned int k = 0; k < lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.size(); k++){
				debug.addDebugLine(lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->source->position, lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->target->position, Util::Color4f(0, 1, 0.5f, 1), Util::Color4f(0, 0.5f, 1, 1));
			}
		}
	}
#endif //TP_SHOW_EDGES
	//DEBUG END
#endif //TP_ACTIVATE_DEBUG

	std::cout << "propagate light" << std::endl;

	//first initialize the graph structures on the GPU
	createNodeTextures();
	createEdgeTextures();
	//then propagate the light the first time
	propagateLight();

	std::cout << "activate lighting" << std::endl;

	//assign the graph-using-shader and the textureState to the selected node
	if(!graphShaderAssigned){
		lightGraphTextureState = new ImageState();
		lightGraphTextureState->setTextureUnit(0);
		lightRootNode->addState(lightGraphTextureState.get());
		graphShaderAssigned = true;
		lightRootNode->addState(lightGraphShader.get());
	}
	//create
	lightGraphShader->setUniform(Rendering::Uniform("globalLightingSize", (int32_t)nodeTextureRendering[curNodeTextureRenderingIndex]->getWidth()));
	lightGraphShader->setUniform(Rendering::Uniform("lightStrengthFactor", LIGHT_STRENGTH_FACTOR / NUMBER_LIGHT_PROPAGATION_CYCLES));
	lightGraphTextureState->setTexture(nodeTextureRendering[curNodeTextureRenderingIndex].get());

	std::cout << "register events" << std::endl;

	sceneRootNode->clearTransformationObservers();
	sceneRootNode->addTransformationObserver(std::bind(&LightNodeManager::onNodeTransformed,this,std::placeholders::_1));

#ifdef TP_ACTIVATE_DEBUG
	std::cout << "build debug" << std::endl;

	debug.buildDebugLineNode();
	debug.buildDebugFaceNode();
	setShowEdges(SHOW_EDGES);
	setShowOctree(SHOW_OCTREE);
	Rendering::checkGLError(__FILE__, __LINE__);
#endif //TP_ACTIVATE_DEBUG

	std::cout << "finish algorithm" << std::endl;
}

void LightNodeManager::createLightNodes(){
	globalNodeCounter = 0;

	NodeCreaterVisitor createNodes;
	createNodes.nodeIndex = 0;
	createNodes.lightNodeMaps = &lightNodeMaps;
	createNodes.lightNodeLightMaps = &lightNodeLightMaps;

	//create from active selection the geometry lighting
	createNodes.useGeometryNodes = true;
	createNodes.useLightNodes = false;
	lightRootNode->traverse(createNodes);

	//take all lights inside the scene into account
	createNodes.useGeometryNodes = false;
	createNodes.useLightNodes = true;
	sceneRootNode->traverse(createNodes);
}

void LightNodeManager::createLightNodes(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes){
	createLightNodesPerVertexPercent(node, lightNodes, PERCENT_NODES);
}

void LightNodeManager::mapLightNodesToObject(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes){
	for(unsigned int i = 0; i < lightNodes->size(); i++){
		(*lightNodes)[i]->id = globalNodeCounter++;
	}
	mapLightNodesToObjectClosest(node, lightNodes);
}

void LightNodeManager::createLightEdges(Rendering::Texture* atomicCounter){
	createLightEdgesStaticFromLights(atomicCounter);
	createLightEdgesDynamicFromLights(atomicCounter);
	createLightEdgesDynamicFromDynamicLights(atomicCounter);
	createLightEdgesInternal(atomicCounter);
	createLightEdgesExternalStatic(atomicCounter);
	createLightEdgesExternalDynamic(atomicCounter);
}

void LightNodeManager::createLightEdgesStaticFromLights(Rendering::Texture* atomicCounter){
//	for(unsigned int lightNodeMapID = 0; lightNodeMapID < lightNodeMaps.size(); lightNodeMapID++){
//		//create (possible) edges from the light sources to this node
//		for(unsigned int lightNodeLightMapID = 0; lightNodeLightMapID < lightNodeLightMaps.size(); lightNodeLightMapID++){
//			LightNode* source = &lightNodeLightMaps[lightNodeLightMapID]->light;
//			for(unsigned int lightNodeTargetID = 0; lightNodeTargetID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeTargetID++){
//				LightNode* target = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeTargetID];
//				addLightEdge(source, target, &lightNodeLightMaps[lightNodeLightMapID]->edges, MAX_EDGE_LENGTH_LIGHT, MIN_EDGE_WEIGHT_LIGHT, false, false);
//			}
//		}
//	}
//	//filter the edges from the lights
//	for(unsigned int lightNodeLightMapID = 0; lightNodeLightMapID < lightNodeLightMaps.size(); lightNodeLightMapID++){
//		filterIncorrectEdges(&lightNodeLightMaps[lightNodeLightMapID]->edges, voxelOctreeTextureStatic.get(), atomicCounter, sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), true);
//	}
	//create (possible) edges from the static light sources to the static nodes
	for(unsigned int lightNodeLightMapID = 0; lightNodeLightMapID < lightNodeLightMaps.size(); lightNodeLightMapID++){
		if(lightNodeLightMaps[lightNodeLightMapID]->staticNode){
			for(unsigned int lightNodeMapID = 0; lightNodeMapID < lightNodeMaps.size(); lightNodeMapID++){
				if(lightNodeMaps[lightNodeMapID]->staticNode){
					LightNode* source = &lightNodeLightMaps[lightNodeLightMapID]->light;
					LightNode* target;
					for(unsigned int lightNodeTargetID = 0; lightNodeTargetID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeTargetID++){
						target = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeTargetID];
						addLightEdge(source, target, &lightNodeLightMaps[lightNodeLightMapID]->staticEdges, MAX_EDGE_LENGTH_LIGHT, MIN_EDGE_WEIGHT_LIGHT, false, false);
					}
				}
			}
		}
	}
	//filter the edges from the lights
	for(unsigned int lightNodeLightMapID = 0; lightNodeLightMapID < lightNodeLightMaps.size(); lightNodeLightMapID++){
		filterIncorrectEdges(&lightNodeLightMaps[lightNodeLightMapID]->staticEdges, voxelOctreeTextureComplete.get(), atomicCounter, sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), true);
	}
}

void LightNodeManager::createLightEdgesDynamicFromLights(Rendering::Texture* atomicCounter){
	//create (possible) edges from the static light sources to the dynamic nodes
	for(unsigned int lightNodeLightMapID = 0; lightNodeLightMapID < lightNodeLightMaps.size(); lightNodeLightMapID++){
		if(lightNodeLightMaps[lightNodeLightMapID]->staticNode){
			for(unsigned int lightNodeMapID = 0; lightNodeMapID < lightNodeMaps.size(); lightNodeMapID++){
				if(!lightNodeMaps[lightNodeMapID]->staticNode){
					LightNode* source = &lightNodeLightMaps[lightNodeLightMapID]->light;
					LightNode* target;
					for(unsigned int lightNodeTargetID = 0; lightNodeTargetID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeTargetID++){
						target = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeTargetID];
						addLightEdge(source, target, &lightNodeLightMaps[lightNodeLightMapID]->dynamicEdges, MAX_EDGE_LENGTH_LIGHT, MIN_EDGE_WEIGHT_LIGHT, false, false);
					}
				}
			}
		}
	}
	//filter the edges from the lights
	for(unsigned int lightNodeLightMapID = 0; lightNodeLightMapID < lightNodeLightMaps.size(); lightNodeLightMapID++){
		filterIncorrectEdges(&lightNodeLightMaps[lightNodeLightMapID]->dynamicEdges, voxelOctreeTextureComplete.get(), atomicCounter, sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), true);
	}
}

void LightNodeManager::createLightEdgesDynamicFromDynamicLights(Rendering::Texture* atomicCounter){
	//create (possible) edges from the static light sources to the dynamic nodes
	for(unsigned int lightNodeLightMapID = 0; lightNodeLightMapID < lightNodeLightMaps.size(); lightNodeLightMapID++){
		if(!lightNodeLightMaps[lightNodeLightMapID]->staticNode){
			for(unsigned int lightNodeMapID = 0; lightNodeMapID < lightNodeMaps.size(); lightNodeMapID++){
				LightNode* source = &lightNodeLightMaps[lightNodeLightMapID]->light;
				LightNode* target;
				for(unsigned int lightNodeTargetID = 0; lightNodeTargetID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeTargetID++){
					target = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeTargetID];
					addLightEdge(source, target, &lightNodeLightMaps[lightNodeLightMapID]->dynamicEdges, MAX_EDGE_LENGTH_LIGHT, MIN_EDGE_WEIGHT_LIGHT, false, false);
				}
			}
		}
	}
	//filter the edges from the lights
	for(unsigned int lightNodeLightMapID = 0; lightNodeLightMapID < lightNodeLightMaps.size(); lightNodeLightMapID++){
		filterIncorrectEdges(&lightNodeLightMaps[lightNodeLightMapID]->dynamicEdges, voxelOctreeTextureComplete.get(), atomicCounter, sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), true);
	}
}

void LightNodeManager::createLightEdgesInternal(Rendering::Texture* atomicCounter){
#ifdef TP_INTERNAL_FILTER_OCTREE
	unsigned int internalEdgesOctreeDepth = std::min(VOXEL_OCTREE_DEPTH, (unsigned int)7);
	unsigned int ieTextureSize = std::pow(2, internalEdgesOctreeDepth) * 4;
	Util::Reference<Rendering::Texture> ieTexSize = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, ieTextureSize, ieTextureSize, 1, Util::TypeConstant::UINT8, 1);
	Util::Reference<Rendering::Texture> ieVoxelOctreeTexture = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, VOXEL_OCTREE_TEXTURE_SIZE, VOXEL_OCTREE_TEXTURE_SIZE, 1, Util::TypeConstant::UINT32, 1);
	Util::Reference<Rendering::Texture> ieVoxelOctreeLocks = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, VOXEL_OCTREE_TEXTURE_SIZE / VOXEL_OCTREE_SIZE_PER_NODE, VOXEL_OCTREE_TEXTURE_SIZE / VOXEL_OCTREE_SIZE_PER_NODE, 1, Util::TypeConstant::UINT32, 1);
	Util::Reference<MinSG::CameraNodeOrtho> ieCameras[3];
	for(unsigned int i = 0; i < 3; i++){
		ieCameras[i] = new MinSG::CameraNodeOrtho();
	}
#endif

	//TODO: speed up (e.g. with octree)
	for(unsigned int lightNodeMapID = 0; lightNodeMapID < lightNodeMaps.size(); lightNodeMapID++){
		//create (possible) internal edges
#ifdef TP_INTERNAL_FILTER_OCTREE
		fillTexture(atomicCounter, 0);
		fillTexture(ieVoxelOctreeTexture.get(), 0);
		fillTexture(ieVoxelOctreeLocks.get(), 0);
		createWorldBBCameras(lightNodeMaps[lightNodeMapID]->geometryNode, ieCameras);
		buildVoxelOctree(ieVoxelOctreeTexture.get(), atomicCounter, 0, ieVoxelOctreeLocks.get(), ieCameras, internalEdgesOctreeDepth, lightNodeMaps[lightNodeMapID]->geometryNode, true, true);
#endif
		for(unsigned int lightNodeSourceID = 0; lightNodeSourceID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeSourceID++){
			for(unsigned int lightNodeTargetID = lightNodeSourceID + 1; lightNodeTargetID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeTargetID++){
				LightNode* source = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeSourceID];
				LightNode* target = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeTargetID];
				addLightEdge(source, target, &lightNodeMaps[lightNodeMapID]->internalLightEdges, MAX_EDGE_LENGTH, MIN_EDGE_WEIGHT, true, true);
			}
		}
#ifdef TP_INTERNAL_FILTER_OCTREE
		filterIncorrectEdges(&lightNodeMaps[lightNodeMapID]->internalLightEdges, ieVoxelOctreeTexture.get(), atomicCounter, ieCameras, internalEdgesOctreeDepth, lightNodeMaps[lightNodeMapID]->geometryNode);
//		ieVoxelOctreeTexture.get()->downloadGLTexture(*renderingContext);
//		Util::Reference<Util::PixelAccessor> voxelOctreeAcc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *ieVoxelOctreeTexture.get());
//		addTreeToDebug(lightNodeMaps[lightNodeMapID]->geometryNode->getWorldBB().getCenter(), lightNodeMaps[lightNodeMapID]->geometryNode->getWorldBB().getExtentMax(), 0, 0, voxelOctreeAcc.get(), internalEdgesOctreeDepth);
#else
		filterIncorrectEdges(&lightNodeMaps[lightNodeMapID]->internalLightEdges, voxelOctreeTextureStatic.get(), atomicCounter, sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), true);
#endif
	}
}

void LightNodeManager::createLightEdgesExternalStatic(Rendering::Texture* atomicCounter){
	for(unsigned int lightNodeMapID = 0; lightNodeMapID < lightNodeMaps.size(); lightNodeMapID++){
		//create (possible) external edges to other objects
		for(unsigned int lightNodeMapID2 = lightNodeMapID + 1; lightNodeMapID2 < lightNodeMaps.size(); lightNodeMapID2++){
			if(lightNodeMaps[lightNodeMapID]->staticNode == true && lightNodeMaps[lightNodeMapID2]->staticNode == true){
				LightNodeMapConnection* mapConnection = new LightNodeMapConnection();
				mapConnection->map1 = lightNodeMaps[lightNodeMapID];
				mapConnection->map2 = lightNodeMaps[lightNodeMapID2];

				if(isDistanceLess(mapConnection->map1->geometryNode, mapConnection->map2->geometryNode, MAX_EDGE_LENGTH)){
					for(unsigned int lightNodeSourceID = 0; lightNodeSourceID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeSourceID++){
						for(unsigned int lightNodeTargetID = 0; lightNodeTargetID < lightNodeMaps[lightNodeMapID2]->lightNodes.size(); lightNodeTargetID++){
							LightNode* source = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeSourceID];
							LightNode* target = lightNodeMaps[lightNodeMapID2]->lightNodes[lightNodeTargetID];
							addLightEdge(source, target, &mapConnection->edges, MAX_EDGE_LENGTH, MIN_EDGE_WEIGHT, true, true);
						}
					}
				}
				if(mapConnection->edges.size() > 0){
					filterIncorrectEdges(&mapConnection->edges, voxelOctreeTextureStatic.get(), atomicCounter, sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), true);
					if(mapConnection->edges.size() > 0){
						lightNodeMaps[lightNodeMapID]->externalLightEdgesStatic.push_back(mapConnection);
					} else {
						delete mapConnection;
					}
				} else {
					delete mapConnection;
				}
			}
		}
	}
}

void LightNodeManager::createLightEdgesExternalDynamic(Rendering::Texture* atomicCounter){
	for(unsigned int lightNodeMapID = 0; lightNodeMapID < lightNodeMaps.size(); lightNodeMapID++){
		//create (possible) external edges to other objects
		for(unsigned int lightNodeMapID2 = lightNodeMapID + 1; lightNodeMapID2 < lightNodeMaps.size(); lightNodeMapID2++){
			if(lightNodeMaps[lightNodeMapID]->staticNode == false || lightNodeMaps[lightNodeMapID2]->staticNode == false){
				LightNodeMapConnection* mapConnection = new LightNodeMapConnection();
				mapConnection->map1 = lightNodeMaps[lightNodeMapID];
				mapConnection->map2 = lightNodeMaps[lightNodeMapID2];

//				std::cout << "trying to create dynamic edges" << std::endl;
				if(isDistanceLess(mapConnection->map1->geometryNode, mapConnection->map2->geometryNode, MAX_EDGE_LENGTH)){
					std::cout << "trying to create dynamic edges2" << std::endl;
					for(unsigned int lightNodeSourceID = 0; lightNodeSourceID < lightNodeMaps[lightNodeMapID]->lightNodes.size(); lightNodeSourceID++){
						for(unsigned int lightNodeTargetID = 0; lightNodeTargetID < lightNodeMaps[lightNodeMapID2]->lightNodes.size(); lightNodeTargetID++){
							LightNode* source = lightNodeMaps[lightNodeMapID]->lightNodes[lightNodeSourceID];
							LightNode* target = lightNodeMaps[lightNodeMapID2]->lightNodes[lightNodeTargetID];
							addLightEdge(source, target, &mapConnection->edges, MAX_EDGE_LENGTH, MIN_EDGE_WEIGHT, true, true);
						}
					}
				}
//				std::cout << "trying to create dynamic edges3" << std::endl;
				if(mapConnection->edges.size() > 0){
//					std::cout << "trying to create dynamic edges4" << std::endl;
					filterIncorrectEdges(&mapConnection->edges, voxelOctreeTextureComplete.get(), atomicCounter, sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), true);
					if(mapConnection->edges.size() > 0){
						std::cout << "trying to create dynamic edges5" << std::endl;
						lightNodeMaps[lightNodeMapID]->externalLightEdgesDynamic.push_back(mapConnection);
					} else {
						std::cout << "trying to create dynamic edges6" << std::endl;
						delete mapConnection;
					}
				} else {
					delete mapConnection;
				}
			}
		}
	}
}

void LightNodeManager::cleanUpDebug(){
//	if(debug != 0){
//		delete debug;
//		debug = 0;
//	}
}

void LightNodeManager::cleanUp(){
//	for(unsigned int i = 0; i < 3; i++){
//		if(sceneEnclosingCameras[i] != 0){
//			delete sceneEnclosingCameras[i];
//			sceneEnclosingCameras[i] = 0;
//		}
//	}

	removeAllStaticMapConnections();
	removeAllDynamicMapConnections();

	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
		for(unsigned int j = 0; j < lightNodeMaps[i]->lightNodes.size(); j++){
            delete lightNodeMaps[i]->lightNodes[j];
		}
		lightNodeMaps[i]->lightNodes.clear();
		for(unsigned int j = 0; j < lightNodeMaps[i]->internalLightEdges.size(); j++){
            delete lightNodeMaps[i]->internalLightEdges[j];
		}
		lightNodeMaps[i]->internalLightEdges.clear();
		//removing external edges is more difficult, since not every node may delete them, only one
//		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesStatic.size(); j++){
//			if(lightNodeMaps[i]->externalLightEdgesStatic[j]->map1 == lightNodeMaps[i]){
//				if(lightNodeMaps[i]->externalLightEdgesStatic[j]->map2 == 0){
//					delete lightNodeMaps[i]->externalLightEdgesStatic[j];
//				} else {
//					for(int k = 0; k < lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.size(); k++){
//						delete lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k];
//					}
//					lightNodeMaps[i]->externalLightEdgesStatic[j]->map1 = 0;
//				}
//			} else {
//				if(lightNodeMaps[i]->externalLightEdgesStatic[j]->map1 == 0){
//					delete lightNodeMaps[i]->externalLightEdgesStatic[j];
//				} else {
//					for(int k = 0; k < lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.size(); k++){
//						delete lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k];
//					}
//					lightNodeMaps[i]->externalLightEdgesStatic[j]->map2 = 0;
//				}
//			}
//		}
//		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesDynamic.size(); j++){
//			if(lightNodeMaps[i]->externalLightEdgesDynamic[j]->map1 == lightNodeMaps[i]){
//				if(lightNodeMaps[i]->externalLightEdgesDynamic[j]->map2 == 0){
//					delete lightNodeMaps[i]->externalLightEdgesDynamic[j];
//				} else {
//					for(int k = 0; k < lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges.size(); k++){
//						delete lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k];
//					}
//					lightNodeMaps[i]->externalLightEdgesDynamic[j]->map1 = 0;
//				}
//			} else {
//				if(lightNodeMaps[i]->externalLightEdgesDynamic[j]->map1 == 0){
//					delete lightNodeMaps[i]->externalLightEdgesDynamic[j];
//				} else {
//					for(int k = 0; k < lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges.size(); k++){
//						delete lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k];
//					}
//					lightNodeMaps[i]->externalLightEdgesDynamic[j]->map2 = 0;
//				}
//			}
//		}
		delete lightNodeMaps[i];
	}
	lightNodeMaps.clear();

	//clear memory of the lightsNodes from the light in the scene
	removeAllStaticLightMapConnections();
	removeAllDynamicLightMapConnections();
	for(unsigned int i = 0; i < lightNodeLightMaps.size(); i++){
//		for(unsigned int j = 0; j < lightNodeLightMaps[i]->edges.size(); j++){
//			delete lightNodeLightMaps[i]->edges[j];
//		}
//		lightNodeLightMaps[i]->edges.clear();
		delete lightNodeLightMaps[i];
	}
	lightNodeLightMaps.clear();
}

void LightNodeManager::setShowEdges(bool showEdges){
	SHOW_EDGES = showEdges;
	debug.setLinesShown(showEdges);
}

void LightNodeManager::setShowOctree(bool showOctree){
	std::cout << "Show Octree is now " << showOctree << std::endl;
	SHOW_OCTREE = showOctree;
	debug.setFacesShown(showOctree);
}

void LightNodeManager::onNodeTransformed(Node* node){
//	std::cout << "Node Moved!" << std::endl;
	if(node->getTypeId() == MinSG::GeometryNode::getClassId()){
		//search the corresponding nodeMap
		for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
			if(lightNodeMaps[i]->geometryNode == node){
#ifdef TP_ACTIVATE_DEBUG
					debug.clearDebug();
#endif //TP_ACTIVATE_DEBUG

//				std::cout << "Found node!" << std::endl;
				if(lightNodeMaps[i]->staticNode){
//					//if static before, remove all static edges (from itself and the other static nodes)
//					for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesStatic.size(); j++){
//						if(lightNodeMaps[i]->externalLightEdgesStatic[j]->map1 == lightNodeMaps[i]){
//							removeStaticLightNodeMapConnection(lightNodeMaps[i]->externalLightEdgesStatic[j]->map2, lightNodeMaps[i]->externalLightEdgesStatic[j]);
//						} else {
//							removeStaticLightNodeMapConnection(lightNodeMaps[i]->externalLightEdgesStatic[j]->map1, lightNodeMaps[i]->externalLightEdgesStatic[j]);
//						}
//						for(unsigned int k = 0; k < lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.size(); k++){
//							delete lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k];
//						}
//						lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.clear();
//						delete lightNodeMaps[i]->externalLightEdgesStatic[j];
//					}
//					lightNodeMaps[i]->externalLightEdgesStatic.clear();

					//now recreate the static external edges
					removeAllStaticMapConnections();
					removeAllStaticLightMapConnections();
					lightNodeMaps[i]->staticNode = false;
					fillTexture(voxelOctreeTextureStatic.get(), 0);
					buildVoxelOctree(voxelOctreeTextureStatic.get(), atomicCounter.get(), 0, voxelOctreeLocks.get(), sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), true, false, true);
					//read it from GPU to be able to copy it
					voxelOctreeTextureStatic.get()->downloadGLTexture(*renderingContext);
					//read the atomic counter
					atomicCounter.get()->downloadGLTexture(*renderingContext);
					Util::Reference<Util::PixelAccessor> counterAcc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *atomicCounter.get());
					Util::Color4f numNodes = counterAcc.get()->readColor4f(0, 0);
					numberStaticOctreeNodes = (unsigned int)numNodes.r();
					//recreate the static external edges
					createLightEdgesExternalStatic(atomicCounter.get());
					createLightEdgesStaticFromLights(atomicCounter.get());
				} else {
					//set the atomicCounter to the number of static voxelOctreeNodes
					fillTexture(atomicCounter.get(), 0);
				}

				//and the dynamic edges have always to be recreated
				refreshLightNodeParameters(lightNodeMaps[i]->geometryNode, &lightNodeMaps[i]->lightNodes);
				copyTexture(voxelOctreeTextureStatic.get(), voxelOctreeTextureComplete.get());
				buildVoxelOctree(voxelOctreeTextureComplete.get(), atomicCounter.get(), numberStaticOctreeNodes, voxelOctreeLocks.get(), sceneEnclosingCameras, VOXEL_OCTREE_DEPTH, lightRootNode.get(), false, true, true);
				removeAllDynamicMapConnections();
				removeAllDynamicLightMapConnections();
				createLightEdgesExternalDynamic(atomicCounter.get());
				createLightEdgesDynamicFromLights(atomicCounter.get());
				createLightEdgesDynamicFromDynamicLights(atomicCounter.get());
				createEdgeTextures();
				propagateLight();

#ifdef TP_ACTIVATE_DEBUG
				std::cout << "Min/Max Edge Weight: " << globalMinEdgeWeight << "/" << globalMaxEdgeWeight << std::endl;
				std::cout << "Should give something like " << LIGHT_STRENGTH * NUMBER_LIGHT_PROPAGATION_CYCLES * globalMinEdgeWeight << "/" << LIGHT_STRENGTH * NUMBER_LIGHT_PROPAGATION_CYCLES * globalMaxEdgeWeight << std::endl;

				std::cout << "number static edges: " << lightNodeMaps[i]->externalLightEdgesStatic.size() << " number dynamic edges: " << lightNodeMaps[i]->externalLightEdgesDynamic.size() << std::endl;
				if(lightNodeMaps[i]->externalLightEdgesDynamic.size() >= 1) std::cout << "num dynamic edges2: " << lightNodeMaps[i]->externalLightEdgesDynamic[0]->edges.size() << std::endl;

				for(unsigned int j = 0; j < lightNodeMaps.size(); j++){
					if(lightNodeMaps[j]->externalLightEdgesDynamic.size() > 0){
						std::cout << "Node " << j << " has " << lightNodeMaps[j]->externalLightEdgesDynamic.size() << " LightConnections with ";
						for(unsigned int k = 0; k < lightNodeMaps[j]->externalLightEdgesDynamic.size(); k++){
							std::cout << "(" << k << ") " << lightNodeMaps[j]->externalLightEdgesDynamic[k]->edges.size() << " edges ";
						}
						std::cout << std::endl;
					}
				}

				//draw debug lines
#ifdef TP_SHOW_EDGES
				//edges from lights
				for(unsigned int k = 0; k < lightNodeLightMaps.size(); k++){
					for(unsigned int j = 0; j < lightNodeLightMaps[k]->staticEdges.size(); j++){
						debug.addDebugLine(lightNodeLightMaps[k]->staticEdges[j]->source->position, lightNodeLightMaps[k]->staticEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
					}
					for(unsigned int j = 0; j < lightNodeLightMaps[k]->dynamicEdges.size(); j++){
						debug.addDebugLine(lightNodeLightMaps[k]->dynamicEdges[j]->source->position, lightNodeLightMaps[k]->dynamicEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
					}
				}

				for(unsigned int l = 0; l < lightNodeMaps.size(); l++){
					//internal edges
					for(unsigned int j = 0; j < lightNodeMaps[l]->internalLightEdges.size(); j++){
						debug.addDebugLine(lightNodeMaps[l]->internalLightEdges[j]->source->position, lightNodeMaps[l]->internalLightEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
					}
					//external edges
					for(unsigned int j = 0; j < lightNodeMaps[l]->externalLightEdgesStatic.size(); j++){
						for(unsigned int k = 0; k < lightNodeMaps[l]->externalLightEdgesStatic[j]->edges.size(); k++){
							debug.addDebugLine(lightNodeMaps[l]->externalLightEdgesStatic[j]->edges[k]->source->position, lightNodeMaps[l]->externalLightEdgesStatic[j]->edges[k]->target->position, Util::Color4f(0, 1, 0.5f, 1), Util::Color4f(0, 0.5f, 1, 1));
						}
					}
					for(unsigned int j = 0; j < lightNodeMaps[l]->externalLightEdgesDynamic.size(); j++){
						for(unsigned int k = 0; k < lightNodeMaps[l]->externalLightEdgesDynamic[j]->edges.size(); k++){
							debug.addDebugLine(lightNodeMaps[l]->externalLightEdgesDynamic[j]->edges[k]->source->position, lightNodeMaps[l]->externalLightEdgesDynamic[j]->edges[k]->target->position, Util::Color4f(0, 1, 0.5f, 1), Util::Color4f(0, 0.5f, 1, 1));
						}
					}
				}
#endif //TP_SHOW_EDGES

				//DEBUG to show the voxelOctree!
				voxelOctreeTextureComplete.get()->downloadGLTexture(*renderingContext);
				Util::Reference<Util::PixelAccessor> voxelOctreeAcc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *voxelOctreeTextureComplete.get());

				//draw tree debug
#ifdef TP_SHOW_OCTREE
				std::cout << "Number debug nodes: " << addTreeToDebug(lightingArea.center, lightingArea.extend, 0, 0, voxelOctreeAcc.get(), VOXEL_OCTREE_DEPTH) << std::endl;
#endif //TP_SHOW_OCTREE
				debug.buildDebugLineNode();
				debug.buildDebugFaceNode();
				setShowEdges(SHOW_EDGES);
				setShowOctree(SHOW_OCTREE);
				//DEBUG END
#endif //TP_ACTIVATE_DEBUG

				break;
			}
		}
	} else if(node->getTypeId() == MinSG::LightNode::getClassId()){
		std::cout << "Found a light Node moving" << std::endl;
		//search the corresponding nodeLightMap
		for(unsigned int i = 0; i < lightNodeLightMaps.size(); i++){
			if(lightNodeLightMaps[i]->lightNode == node){
#ifdef TP_ACTIVATE_DEBUG
				debug.clearDebug();
#endif //TP_ACTIVATE_DEBUG

				std::cout << "Found the light Node!" << std::endl;
				if(lightNodeLightMaps[i]->staticNode){
					//if static before, remove all static edges
					removeAllStaticLightMapConnections();
					lightNodeLightMaps[i]->staticNode = false;
					createLightEdgesStaticFromLights(atomicCounter.get());
					createLightEdgesDynamicFromLights(atomicCounter.get());
				}

				refreshLightNodeLightParameters(lightNodeLightMaps[i]->lightNode, &lightNodeLightMaps[i]->light);
				removeAllDynamicLightMapConnections();
				createLightEdgesDynamicFromDynamicLights(atomicCounter.get());
				createEdgeTextures();
				propagateLight();

#ifdef TP_ACTIVATE_DEBUG
				//draw debug lines
#ifdef TP_SHOW_EDGES
				std::cout << "Min/Max Edge Weight: " << globalMinEdgeWeight << "/" << globalMaxEdgeWeight << std::endl;
				std::cout << "Should give something like " << LIGHT_STRENGTH * NUMBER_LIGHT_PROPAGATION_CYCLES * globalMinEdgeWeight << "/" << LIGHT_STRENGTH * NUMBER_LIGHT_PROPAGATION_CYCLES * globalMaxEdgeWeight << std::endl;

				//edges from lights
				for(unsigned int k = 0; k < lightNodeLightMaps.size(); k++){
					for(unsigned int j = 0; j < lightNodeLightMaps[k]->staticEdges.size(); j++){
						debug.addDebugLine(lightNodeLightMaps[k]->staticEdges[j]->source->position, lightNodeLightMaps[k]->staticEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
					}
					for(unsigned int j = 0; j < lightNodeLightMaps[k]->dynamicEdges.size(); j++){
						debug.addDebugLine(lightNodeLightMaps[k]->dynamicEdges[j]->source->position, lightNodeLightMaps[k]->dynamicEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
					}
				}

				for(unsigned int l = 0; l < lightNodeMaps.size(); l++){
					//internal edges
					for(unsigned int j = 0; j < lightNodeMaps[l]->internalLightEdges.size(); j++){
						debug.addDebugLine(lightNodeMaps[l]->internalLightEdges[j]->source->position, lightNodeMaps[l]->internalLightEdges[j]->target->position, Util::Color4f(1, 0.5f, 0, 1), Util::Color4f(0.5f, 1, 0, 1));
					}
					//external edges
					for(unsigned int j = 0; j < lightNodeMaps[l]->externalLightEdgesStatic.size(); j++){
						for(unsigned int k = 0; k < lightNodeMaps[l]->externalLightEdgesStatic[j]->edges.size(); k++){
							debug.addDebugLine(lightNodeMaps[l]->externalLightEdgesStatic[j]->edges[k]->source->position, lightNodeMaps[l]->externalLightEdgesStatic[j]->edges[k]->target->position, Util::Color4f(0, 1, 0.5f, 1), Util::Color4f(0, 0.5f, 1, 1));
						}
					}
					for(unsigned int j = 0; j < lightNodeMaps[l]->externalLightEdgesDynamic.size(); j++){
						for(unsigned int k = 0; k < lightNodeMaps[l]->externalLightEdgesDynamic[j]->edges.size(); k++){
							debug.addDebugLine(lightNodeMaps[l]->externalLightEdgesDynamic[j]->edges[k]->source->position, lightNodeMaps[l]->externalLightEdgesDynamic[j]->edges[k]->target->position, Util::Color4f(0, 1, 0.5f, 1), Util::Color4f(0, 0.5f, 1, 1));
						}
					}
				}
#endif //TP_SHOW_EDGES

				//DEBUG to show the voxelOctree!
				voxelOctreeTextureComplete.get()->downloadGLTexture(*renderingContext);
				Util::Reference<Util::PixelAccessor> voxelOctreeAcc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *voxelOctreeTextureComplete.get());

				//draw tree debug
#ifdef TP_SHOW_OCTREE
				std::cout << "Number debug nodes: " << addTreeToDebug(lightingArea.center, lightingArea.extend, 0, 0, voxelOctreeAcc.get(), VOXEL_OCTREE_DEPTH) << std::endl;
#endif //TP_SHOW_OCTREE
				debug.buildDebugLineNode();
				debug.buildDebugFaceNode();
				setShowEdges(SHOW_EDGES);
				setShowOctree(SHOW_OCTREE);
				//DEBUG END
#endif //TP_ACTIVATE_DEBUG

				break;
			}
		}
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
	lightingArea.center = lightRootNode->getWorldBB().getCenter();
	lightingArea.extend = lightRootNode->getWorldBB().getExtentMax();
}

void LightNodeManager::refreshLightNodeParameters(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes){
	LightNode* lightNode;
	for(unsigned int i = 0; i < lightNodes->size(); i++){
		lightNode = (*lightNodes)[i];
		Geometry::Vec4 pos(lightNode->osPosition.x(), lightNode->osPosition.y(), lightNode->osPosition.z(), 1);
		pos = node->getWorldMatrix() * pos;
//		std::cout << "pos: " << pos.x() << "x" << pos.y() << "x" << pos.z() << std::endl;
		lightNode->position = Geometry::Vec3(pos.x(), pos.y(), pos.z());
	}
}

void LightNodeManager::refreshLightNodeLightParameters(MinSG::LightNode* node, LightNode* lightNode){
	lightNode->position = node->getWorldOrigin();
}

void LightNodeManager::createLightNodesPerVertexPercent(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes, float percentage){
	Rendering::Mesh* mesh = node->getMesh();

	Util::Reference<Rendering::PositionAttributeAccessor> posAcc = Rendering::PositionAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::POSITION);
	Util::Reference<Rendering::NormalAttributeAccessor> norAcc = Rendering::NormalAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::NORMAL);
	Util::Reference<Rendering::ColorAttributeAccessor> colAcc = Rendering::ColorAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::COLOR);

	unsigned int modNum = (int)(1 / percentage);
	if(modNum <= 0) modNum = 1;

	for(unsigned int i = 0; i < mesh->getVertexCount(); ++i){
		if(i % modNum == 0){
			LightNode* lightNode = new LightNode();
			lightNode->osPosition = posAcc->getPosition(i);
			Geometry::Vec4 pos(lightNode->osPosition.x(), lightNode->osPosition.y(), lightNode->osPosition.z(), 1);
			pos = node->getWorldMatrix() * pos;
			lightNode->position = Geometry::Vec3(pos.x(), pos.y(), pos.z());
			lightNode->normal = norAcc->getNormal(i);
			lightNode->color = colAcc->getColor4f(i);
			lightNodes->push_back(lightNode);
		}
	}
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
			lightNode->osPosition = posAcc->getPosition(i);
			Geometry::Vec4 pos(lightNode->osPosition.x(), lightNode->osPosition.y(), lightNode->osPosition.z(), 1);
			pos = node->getWorldMatrix() * pos;
			lightNode->position = Geometry::Vec3(pos.x(), pos.y(), pos.z());
			lightNode->normal = norAcc->getNormal(i);
			lightNode->color = colAcc->getColor4f(i);
			lightNodes->push_back(lightNode);
		}
	}

	if(lightNodes->empty() && mesh->getVertexCount() != 0){
		LightNode* lightNode = new LightNode();
		lightNode->osPosition = posAcc->getPosition(0);
		Geometry::Vec4 pos(lightNode->osPosition.x(), lightNode->osPosition.y(), lightNode->osPosition.z(), 1);
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
//		description.appendUnsignedIntAttribute(lightNodeIDIdent, 1);
		description.appendFloatAttribute(lightNodeIDIdent, 1);
		Rendering::MeshVertexData& oldData = mesh->openVertexData();
		std::unique_ptr<Rendering::MeshVertexData> newData(Rendering::MeshUtils::convertVertices(oldData, description));
		oldData.swap(*newData);
	}

//	unsigned int mini = 999999, maxi = 0;
	Util::Reference<Rendering::PositionAttributeAccessor> posAcc = Rendering::PositionAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::POSITION);
	Util::Reference<Rendering::LightNodeIndexAttributeAccessor> lniAcc = Rendering::LightNodeIndexAttributeAccessor::create(mesh->openVertexData(), lightNodeIDIdent);
	for(unsigned int i = 0; i < mesh->getVertexCount(); ++i){
		unsigned int bestIndex = 0;
		Geometry::Vec3 pos = posAcc->getPosition(i);
		Geometry::Vec4 pos2 = node->getWorldMatrix() * Geometry::Vec4(pos.x(), pos.y(), pos.z(), 1);
		pos.setX(pos2.x());
		pos.setY(pos2.y());
		pos.setZ(pos2.z());
		float bestDistance = pos.distanceSquared((*lightNodes)[0]->position);
		for(unsigned int j = 1; j < lightNodes->size(); j++){
			float newDist = pos.distanceSquared((*lightNodes)[j]->position);
			if(bestDistance > newDist){
				bestDistance = newDist;
				bestIndex = j;
			}
		}
		//set lightNodeIndex to vertex
//		if(mini > (*lightNodes)[bestIndex]->id) mini = (*lightNodes)[bestIndex]->id;
//		if(maxi < (*lightNodes)[bestIndex]->id) maxi = (*lightNodes)[bestIndex]->id;
		lniAcc->setLightNodeIndex(i, (*lightNodes)[bestIndex]->id);
	}
	mesh->openVertexData().markAsChanged();

//	std::cout << "min: " << mini << " max: " << maxi << std::endl;
}

bool LightNodeManager::isVisible(LightNode* source, LightNode* target){
	Geometry::Vec3 direction = target->position - source->position;
	//quick test, if the angles could fit
	if(source->normal.dot(direction) > 0 && target->normal.dot(direction) < 0){
//		debug.addDebugLine(source->position, target->position, Util::Color4f(1, 1, 1, 1), Util::Color4f(0.5f, 0.5f, 0.5f, 1));
//		debug.addDebugLine(target->position, target->position + target->normal * 0.1, Util::Color4f(0.7, 0.7, 0.1, 1), Util::Color4f(0.7, 0.7, 0.1, 1));
//		debug.addDebugLine(target->position, target->position + target->normal * 0.1, Util::Color4f(0.7, 0.7, 0.1, 1), Util::Color4f(0.7, 0.7, 0.1, 1));
		return true;
	} else {
//		debug.addDebugLine(source->position, target->position, Util::Color4f(1, 1, 1, 1), Util::Color4f(0.5f, 0.5f, 0.5f, 1));
//		debug.addDebugLine(target->position, target->position + target->normal * 0.1, Util::Color4f(0.7, 0.7, 0.1, 1), Util::Color4f(0.7, 0.7, 0.1, 1));
//		debug.addDebugLine(target->position, target->position + target->normal * 0.1, Util::Color4f(0.7, 0.7, 0.1, 1), Util::Color4f(0.7, 0.7, 0.1, 1));
		return false;
	}
}

void LightNodeManager::addLightEdge(LightNode* source, LightNode* target, std::vector<LightEdge*>* lightEdges, float maxEdgeLength, float minEdgeWeight, bool checkVisibility, bool useNormal){
	//take maximum edge length into account
	float distance = source->position.distance(target->position) + 1;		//+1, since we want to have a function from f(0)=1: f(x) = 1 / (x + 1)^2
	if(distance <= maxEdgeLength + 1){
		if((checkVisibility && isVisible(source, target)) || (!checkVisibility)){
			LightEdge* lightEdge = new LightEdge();
			lightEdge->source = source;
			lightEdge->target = target;
			if(useNormal){
				float dotN = std::max(-source->normal.dot(target->normal), 0.0f);										//taking normal into account
				float distanceDecrease = 1.0f / (distance * distance);													//quadratic decrease of light
				lightEdge->weight.setX(dotN * distanceDecrease * std::min(source->color.getR(), target->color.getR()));	//taking color into account
				lightEdge->weight.setY(dotN * distanceDecrease * std::min(source->color.getG(), target->color.getG()));	//taking color into account
				lightEdge->weight.setZ(dotN * distanceDecrease * std::min(source->color.getB(), target->color.getB()));	//taking color into account
			} else {
				float distanceDecrease = 1.0f / (distance * distance);												//quadratic decrease of light
				lightEdge->weight.setX(distanceDecrease * std::min(source->color.getR(), target->color.getR()));	//taking color into account
				lightEdge->weight.setY(distanceDecrease * std::min(source->color.getG(), target->color.getG()));	//taking color into account
				lightEdge->weight.setZ(distanceDecrease * std::min(source->color.getB(), target->color.getB()));	//taking color into account
			}
			//discard edge, if the weight is too small
			if(std::abs(lightEdge->weight.x()) + std::abs(lightEdge->weight.y()) + std::abs(lightEdge->weight.z()) <= minEdgeWeight){
				delete lightEdge;
			} else {
				globalMinEdgeWeight = std::min(globalMinEdgeWeight, lightEdge->weight.x());
				globalMinEdgeWeight = std::min(globalMinEdgeWeight, lightEdge->weight.y());
				globalMinEdgeWeight = std::min(globalMinEdgeWeight, lightEdge->weight.z());
				globalMaxEdgeWeight = std::max(globalMaxEdgeWeight, lightEdge->weight.x());
				globalMaxEdgeWeight = std::max(globalMaxEdgeWeight, lightEdge->weight.y());
				globalMaxEdgeWeight = std::max(globalMaxEdgeWeight, lightEdge->weight.z());

				lightEdges->push_back(lightEdge);
			}
		}
	}
}

bool LightNodeManager::isDistanceLess(MinSG::Node* n1, MinSG::Node* n2, float distance){
    for(unsigned int i = 0; i < 8; i++){
		for(unsigned int j = i; j < 8; j++){
			if(n1->getWorldBB().getCorner((Geometry::corner_t)i).distance(n2->getWorldBB().getCorner((Geometry::corner_t)j)) < distance) return true;
		}
    }
    return false;
}

void LightNodeManager::filterIncorrectEdges(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, Util::Reference<MinSG::CameraNodeOrtho> enclosingCameras[3], unsigned int maxOctreeDepth, MinSG::Node* node, bool useStartDimensions){
	if(edges->size() > 0) filterIncorrectEdgesAsObjects(edges, octreeTexture, enclosingCameras, maxOctreeDepth, node, useStartDimensions);
}

void LightNodeManager::filterIncorrectEdgesAsObjects(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Util::Reference<MinSG::CameraNodeOrtho> enclosingCameras[3], unsigned int maxOctreeDepth, MinSG::Node* node, bool useStartDimensions){
	//setup texture to upload the edges onto the GPU
	const unsigned int dataLengthPerEdge = 6;
	unsigned int textureSize = std::ceil(std::sqrt(edges->size() * dataLengthPerEdge));
	Util::Reference<Rendering::Texture> edgeInput = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, textureSize, textureSize, 1, Util::TypeConstant::FLOAT, 1);

//	std::cout << "comming here 1" << std::endl;
//	std::cout << "edges: " << edges->size() << std::endl;
//	std::cout << "texSize: " << textureSize << std::endl;
//	std::cout << "texture: " << edgeInput.get()->getWidth() << "x" << edgeInput.get()->getHeight() << " = " << edgeInput.get()->getDataSize() << std::endl;

//	float extendHalf = lightingArea.extend * 0.5f;
//	Geometry::Vec3 treeMin(lightingArea.center.x() - extendHalf, lightingArea.center.y() - extendHalf, lightingArea.center.z() - extendHalf);
//	Geometry::Vec3 treeMax(lightingArea.center.x() + extendHalf, lightingArea.center.y() + extendHalf, lightingArea.center.z() + extendHalf);

	//encode the edges into the texture (writing the positions of the source and target)
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeInput.get());
	for(unsigned int i = 0; i < edges->size(); i++){
		//get the correct index, taking the edge length into account
		unsigned int i2 = i * dataLengthPerEdge;
		//calculate the 2D index for the position inside the texture
		unsigned int x = i2 % textureSize;
		unsigned int y = i2 / textureSize;

		//DEBUG
//		if((*edges)[i]->source->position.x() < treeMin.x() || (*edges)[i]->source->position.y() < treeMin.y() || (*edges)[i]->source->position.z() < treeMin.z() ||
//			(*edges)[i]->source->position.x() > treeMax.x() || (*edges)[i]->source->position.y() > treeMax.y() || (*edges)[i]->source->position.z() > treeMax.z()){
//			std::cout << "Distance is too big!" << std::endl;
//		}
		//DEBUG END

		//write the data to the texture
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->source->position.x());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->source->position.y());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->source->position.z());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->target->position.x());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->target->position.y());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->target->position.z());
	}
	edgeInput->dataChanged();

	static const Util::StringIdentifier EDGE_ID("edgeID");
//	static const Util::StringIdentifier EDGE_ID2("edgeID2");
	Rendering::VertexDescription vertexDesc;
	vertexDesc.appendPosition3D();
//	const Rendering::VertexAttribute & edgeIDAttr = vertexDesc.appendUnsignedIntAttribute(EDGE_ID, 1);
//	const Rendering::VertexAttribute & edgeID2Attr = vertexDesc.appendUnsignedIntAttribute(EDGE_ID2, 1);
	const Rendering::VertexAttribute & edgeIDAttr = vertexDesc.appendFloatAttribute(EDGE_ID, 1);
//	const Rendering::VertexAttribute & edgeID2Attr = vertexDesc.appendFloatAttribute(EDGE_ID2, 1);

	Rendering::MeshUtils::MeshBuilder mb(vertexDesc);
	for(unsigned int i = 0; i < edges->size(); i++){
		mb.position((*edges)[i]->source->position);
		mb.addVertex();

		mb.position((*edges)[i]->target->position);
		mb.addVertex();
	}

	Rendering::Mesh *mesh = mb.buildMesh();
	mesh->setUseIndexData(false);
	mesh->setDrawMode(Rendering::Mesh::DRAW_LINES);
	const size_t vertexSize = vertexDesc.getVertexSize();
	Rendering::MeshVertexData & vertexData = mesh->openVertexData();
	uint8_t* vertexDataPtr = vertexData.data() + edgeIDAttr.getOffset();
	for(uint32_t i = 0; i < edges->size(); ++i) {
		*reinterpret_cast<float *>(vertexDataPtr + i * 2 * vertexSize) = i;
		*reinterpret_cast<float *>(vertexDataPtr + ((i * 2) + 1) * vertexSize) = i;
	}
//	vertexDataPtr = vertexData.data() + edgeID2Attr.getOffset();
//	for(uint32_t i = 0; i < edges->size(); ++i) {
//		*reinterpret_cast<float *>(vertexDataPtr + i * 2 * vertexSize) = 0;
//		*reinterpret_cast<float *>(vertexDataPtr + ((i * 2) + 1) * vertexSize) = 1;
//	}
//	vertexData.markAsChanged();
//	vertexData.upload();
//	vertexData.releaseLocalData();
//	Rendering::MeshVertexData & vertexData2 = mesh->openVertexData();
//	std::cout << "Num Vertices: " << vertexData2.getVertexCount() << " vs " << edges->size()*2 << std::endl;
//	for(unsigned int i = 0; i < 100 /*vertexData2.getVertexCount()*/; i++){
//		unsigned int index = i * 4 * 4;
//		std::cout << "Vertex " << i << " Pos: ";
//		for(unsigned int j = 0; j < 3; j++){
//			vertexDataPtr = vertexData2.data();
//			std::cout << *reinterpret_cast<float*>(vertexDataPtr + index + j * 4) << ", ";
//		}
//		std::cout << " ID: " << *reinterpret_cast<float*>(vertexDataPtr + index + 3 * 4) << std::endl;
//	}

//	std::cout << "Vertex description: " << mesh->getVertexDescription().toString() << std::endl;

	//create the output texture (one pixel per edge, 0 = ok, 1 = edge must be deleted)
	unsigned int outputTextureSize = std::ceil(std::sqrt(edges->size()));
	Util::Reference<Rendering::Texture> edgeOutput = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, outputTextureSize, outputTextureSize, 1, Util::TypeConstant::UINT32, 1);
	fillTexture(edgeOutput.get(), 0);

//	std::cout << "Output texture: " << outputTextureSize << " which gives " << outputTextureSize * outputTextureSize << " possible edges!" << std::endl;

//	debug.addDebugLine(lightingArea.center, Geometry::Vec3(lightingArea.center.x(), lightingArea.center.y() + 1, lightingArea.center.z()), Util::Color4f(0, 1, 0, 1), Util::Color4f(0, 1, 0, 1));
//	std::cout << "Root Node Midpoint: " << lightingArea.center.x() << "x" << lightingArea.center.y() << "x" << lightingArea.center.z() << std::endl;
//	std::cout << "halfSizeOfRootNode: " << lightingArea.extend * 0.5f << std::endl;
	if(useStartDimensions){
		voxelOctreeShaderReadObject.get()->setUniform(*renderingContext, Rendering::Uniform("rootMidPos", lightingArea.center));
		voxelOctreeShaderReadObject.get()->setUniform(*renderingContext, Rendering::Uniform("quarterSizeOfRootNode", lightingArea.extend * 0.25f));
	} else {
		voxelOctreeShaderReadObject.get()->setUniform(*renderingContext, Rendering::Uniform("rootMidPos", node->getWorldBB().getCenter()));
		voxelOctreeShaderReadObject.get()->setUniform(*renderingContext, Rendering::Uniform("quarterSizeOfRootNode", node->getWorldBB().getExtentMax() * 0.25f));
	}
//	voxelOctreeShaderReadObject.get()->setUniform(*renderingContext, Rendering::Uniform("numEdges", (int32_t)edges->size()));
	voxelOctreeShaderReadObject.get()->setUniform(*renderingContext, Rendering::Uniform("outputTextureSize", (int32_t)outputTextureSize));
	voxelOctreeShaderReadObject.get()->setUniform(*renderingContext, Rendering::Uniform("inputTextureSize", (int32_t)textureSize));
	voxelOctreeShaderReadObject.get()->setUniform(*renderingContext, Rendering::Uniform("voxelOctreeTextureSize", (int32_t)octreeTexture->getWidth()));
	voxelOctreeShaderReadObject.get()->setUniform(*renderingContext, Rendering::Uniform("maxTreeDepth", (int32_t)maxOctreeDepth));

//	std::cout << "comming here 4" << std::endl;

	renderingContext->pushAndSetBoundImage(0, Rendering::ImageBindParameters(octreeTexture));
	renderingContext->pushAndSetBoundImage(1, Rendering::ImageBindParameters(edgeOutput.get()));
	renderingContext->pushAndSetBoundImage(2, Rendering::ImageBindParameters(edgeInput.get()));

//	std::cout << "comming here 4.5" << std::endl;

	//activate new camera
	frameContext->pushCamera();
	//deactivate culling and depth tests
	renderingContext->pushCullFace();
	renderingContext->pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::LEQUAL));

	for(unsigned int i = 0; i < 3; i++){
		frameContext->setCamera(enclosingCameras[i].get());
		//activate the shader and check the edges
		TextureProcessor textureProcessor;
		textureProcessor.setRenderingContext(this->renderingContext);
		textureProcessor.setOutputTexture(tmpTexVoxelOctreeSize.get());
		textureProcessor.setDepthTexture(tmpDepthTexSmallest.get());
		textureProcessor.setShader(voxelOctreeShaderReadObject.get());
		textureProcessor.begin();

		renderingContext->clearDepth(1.0f);

		renderingContext->setCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters()));
		frameContext->displayMesh(mesh);
//		renderingContext->setCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters::CULL_BACK));
//		frameContext->displayMesh(mesh);
//		renderingContext->setCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters::CULL_FRONT));
//		frameContext->displayMesh(mesh);

		textureProcessor.end();
	}

	//reset depth test
	renderingContext->popDepthBuffer();
	//reset culling
	renderingContext->popCullFace();
	//reset camera
	frameContext->popCamera();

	renderingContext->popBoundImage(0);
	renderingContext->popBoundImage(1);
	renderingContext->popBoundImage(2);

//	std::cout << "comming here 5" << std::endl;

	//filter the edge list
	edgeOutput.get()->downloadGLTexture(*renderingContext);
//	uint8_t* data = edgeOutput.get()->openLocalData(*renderingContext);
	Util::Reference<Util::PixelAccessor> accOut = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeOutput.get());
	std::vector<LightEdge*> filteredEdges;
//	unsigned int x, y;
	unsigned int before = 0;
	for(unsigned int i = 0; i < edges->size(); i++){
		unsigned int x = i % outputTextureSize;
		unsigned int y = i / outputTextureSize;

		uint8_t value = accOut.get()->readColor4f(x, y).r();//SingleValueByte(x, y);

//		std::cout << "got a " << (int)value << " at " << i << std::endl;
		if(value == 0) filteredEdges.push_back((*edges)[i]);
#ifdef TP_ACTIVATE_DEBUG
//		else debug.addDebugLine((*edges)[i]->source->position, (*edges)[i]->target->position, Util::Color4f(0.5f, 0.5f, 1, 1), Util::Color4f(0, 0, 1, 1));
#endif //TP_ACTIVATE_DEBUG

//		x = (x + 1) % outputTextureSize;
//		if(x == 0) y++;

//		if(data[before] != data[i] - 1){
//			std::cout << i << ": " << (unsigned int)data[i] << " but " << before << ": " << (unsigned int)data[before] << std::endl;
//			before = i;
//			if(i >= 10) break;
//		} else {
//			before = i;
//		}

//		if(data[i] == 0) filteredEdges.push_back((*edges)[i]);
	}

//	unsigned int counter = 0, counter2 = 0, counter3 = 0;
//	edgeOutput.get()->downloadGLTexture(*renderingContext);
//	Util::Reference<Util::PixelAccessor> acc2 = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeOutput.get());
//	for(unsigned int i = 1; i < edgeOutput->getWidth() * edgeOutput->getHeight(); i++){
//		unsigned int x = i % edgeOutput->getWidth();
//		unsigned int y = i / edgeOutput->getWidth();
//		unsigned int xb = (i-1) % edgeOutput->getWidth();
//		unsigned int yb = (i-1) / edgeOutput->getWidth();
//
//		uint8_t valueBefore = acc2.get()->readSingleValueByte(xb, yb);
//		uint8_t value = acc2.get()->readSingleValueByte(x, y);
//		if(valueBefore != value || i % 1000 == 0 || i == 1){
//			std::cout << i << " (of " << edges->size() << ")" << ": " << (unsigned int)value << " but " << (i-1) << ": " << (unsigned int)valueBefore << std::endl;
//			if(counter++ >= 100) break;
////			if(i >= 10) break;
//		}
////		if(value == 2){
////			counter++;
////		} else if(value == 4){
////			counter2++;
////		} else {
////			counter3++;
////		}
//	}
//	std::cout << "Counter: " << counter << " Counter2: " << counter2 << " Counter3: " << counter3 << std::endl;
//
//	std::cout << "comming here 6" << std::endl;

	//write back the filtered edges
	(*edges) = filteredEdges;
}

void LightNodeManager::filterIncorrectEdgesAsTexture(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, MinSG::Node* node){
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

	float extendHalf = lightingArea.extend * 0.5f;
	Geometry::Vec3 treeMin(lightingArea.center.x() - extendHalf, lightingArea.center.y() - extendHalf, lightingArea.center.z() - extendHalf);
	Geometry::Vec3 treeMax(lightingArea.center.x() + extendHalf, lightingArea.center.y() + extendHalf, lightingArea.center.z() + extendHalf);

	//encode the edges into the texture (writing the positions of the source and target)
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeInput.get());
	for(unsigned int i = 0; i < edges->size(); i++){
		//get the correct index, taking the edge length into account
		unsigned int i2 = i * dataLengthPerEdge;
		//calculate the 2D index for the position inside the texture
		unsigned int x = i2 % textureSize;
		unsigned int y = i2 / textureSize;

		//DEBUG
		if((*edges)[i]->source->position.x() < treeMin.x() || (*edges)[i]->source->position.y() < treeMin.y() || (*edges)[i]->source->position.z() < treeMin.z() ||
			(*edges)[i]->source->position.x() > treeMax.x() || (*edges)[i]->source->position.y() > treeMax.y() || (*edges)[i]->source->position.z() > treeMax.z()){
			std::cout << "Distance is too big!" << std::endl;
		}
		//DEBUG END

		//write the data to the texture
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->source->position.x());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->source->position.y());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->source->position.z());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->target->position.x());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->target->position.y());
		x = (x + 1) % textureSize;
		if(x == 0) y++;
		acc.get()->writeSingleValueFloat(x, y, (*edges)[i]->target->position.z());
	}
	edgeInput->dataChanged();

//	for(unsigned int i = 0; i < edges->size(); i++){
//		unsigned int i2 = i * dataLengthPerEdge;
//		unsigned int x = i2 % textureSize;
//		unsigned int y = i2 / textureSize;
//
//		Geometry::Vec3 startPos, endPos;
//		startPos.setX(acc->readSingleValueFloat(x, y));
//		i2++;
//		x = i2 % textureSize;
//		y = i2 / textureSize;
//		startPos.setY(acc->readSingleValueFloat(x, y));
//		i2++;
//		x = i2 % textureSize;
//		y = i2 / textureSize;
//		startPos.setZ(acc->readSingleValueFloat(x, y));
//		i2++;
//		x = i2 % textureSize;
//		y = i2 / textureSize;
//		endPos.setX(acc->readSingleValueFloat(x, y));
//		i2++;
//		x = i2 % textureSize;
//		y = i2 / textureSize;
//		endPos.setY(acc->readSingleValueFloat(x, y));
//		i2++;
//		x = i2 % textureSize;
//		y = i2 / textureSize;
//		endPos.setZ(acc->readSingleValueFloat(x, y));
//		debug.addDebugLine(startPos, endPos, Util::Color4f(1, 0, 1, 1), Util::Color4f(1, 0, 1, 1));
//	}

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

//	debug.addDebugLine(lightingArea.center, Geometry::Vec3(lightingArea.center.x(), lightingArea.center.y() + 1, lightingArea.center.z()), Util::Color4f(0, 1, 0, 1), Util::Color4f(0, 1, 0, 1));
//	std::cout << "Root Node Midpoint: " << lightingArea.center.x() << "x" << lightingArea.center.y() << "x" << lightingArea.center.z() << std::endl;
//	std::cout << "halfSizeOfRootNode: " << lightingArea.extend * 0.5f << std::endl;
	voxelOctreeShaderReadTexture.get()->setUniform(*renderingContext, Rendering::Uniform("rootMidPos", lightingArea.center));
	voxelOctreeShaderReadTexture.get()->setUniform(*renderingContext, Rendering::Uniform("quarterSizeOfRootNode", lightingArea.extend * 0.25f));
	voxelOctreeShaderReadTexture.get()->setUniform(*renderingContext, Rendering::Uniform("numEdges", (int32_t)edges->size()));
	voxelOctreeShaderReadTexture.get()->setUniform(*renderingContext, Rendering::Uniform("inputTextureSize", (int32_t)textureSize));
	voxelOctreeShaderReadTexture.get()->setUniform(*renderingContext, Rendering::Uniform("outputTextureSize", (int32_t)outputTextureSize));
	voxelOctreeShaderReadTexture.get()->setUniform(*renderingContext, Rendering::Uniform("voxelOctreeTextureSize", (int32_t)octreeTexture->getWidth()));

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
	textureProcessor.setShader(voxelOctreeShaderReadTexture.get());
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
	Util::Reference<Util::PixelAccessor> accOut = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeOutput.get());
	std::vector<LightEdge*> filteredEdges;
//	unsigned int x, y;
	unsigned int before = 0;
	for(unsigned int i = 1; i < edges->size(); i++){
		unsigned int x = i % outputTextureSize;
		unsigned int y = i / outputTextureSize;

		uint8_t value = accOut.get()->readSingleValueByte(x, y);

		if(value == 0) filteredEdges.push_back((*edges)[i]);
#ifdef TP_ACTIVATE_DEBUG
		else debug.addDebugLine((*edges)[i]->source->position, (*edges)[i]->target->position, Util::Color4f(0.5f, 0.5f, 1, 1), Util::Color4f(0, 0, 1, 1));
#endif //TP_ACTIVATE_DEBUG

//		x = (x + 1) % outputTextureSize;
//		if(x == 0) y++;

//		if(data[before] != data[i] - 1){
//			std::cout << i << ": " << (unsigned int)data[i] << " but " << before << ": " << (unsigned int)data[before] << std::endl;
//			before = i;
//			if(i >= 10) break;
//		} else {
//			before = i;
//		}

//		if(data[i] == 0) filteredEdges.push_back((*edges)[i]);
	}

	unsigned int counter = 0, counter2 = 0, counter3 = 0;
	Util::Reference<Util::PixelAccessor> acc2 = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeOutput.get());
	for(unsigned int i = 1; i < outputTextureSize * outputTextureSize; i++){
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
//		if(value == 2){
//			counter++;
//		} else if(value == 4){
//			counter2++;
//		} else {
//			counter3++;
//		}
	}
	std::cout << "Counter: " << counter << " Counter2: " << counter2 << " Counter3: " << counter3 << std::endl;

	std::cout << "comming here 6" << std::endl;

	//write back the filtered edges
	(*edges) = filteredEdges;
}

void LightNodeManager::filterIncorrectEdgesAsTextureCPU(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, MinSG::Node* node){
	float rootSizeHalf = lightingArea.extend * 0.5f;
	Geometry::Vec3 treeMin(lightingArea.center.x() - rootSizeHalf, lightingArea.center.y() - rootSizeHalf, lightingArea.center.z() - rootSizeHalf);
	Geometry::Vec3 treeMax(lightingArea.center.x() + rootSizeHalf, lightingArea.center.y() + rootSizeHalf, lightingArea.center.z() + rootSizeHalf);

	std::vector<LightEdge*> filteredEdges;

	quarterSizeOfRootNode = lightingArea.extend * 0.25f;
	voxelOctreeTextureSize = octreeTexture->getWidth();
	Util::Reference<Util::PixelAccessor> octreeAcc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *octreeTexture);

//	int falseEdges = 8;

	for(unsigned int i = 132; i < edges->size(); i++){
//		Geometry::Vec3 lineStart = (*edges)[i]->source->position;
//		Geometry::Vec3 lineEnd = (*edges)[i]->target->position;
		Geometry::Vec3 lineStart(lightingArea.center.x() - quarterSizeOfRootNode * 1.1, lightingArea.center.y() - quarterSizeOfRootNode * 1.1, lightingArea.center.z() - quarterSizeOfRootNode * 0.9);
		Geometry::Vec3 lineEnd(lightingArea.center.x() + quarterSizeOfRootNode * 1.01, lightingArea.center.y() + quarterSizeOfRootNode * 1.1, lightingArea.center.z() - quarterSizeOfRootNode * 0.9);
		Geometry::Vec3 lineDirection = lineEnd - lineStart;
		lineDirection.normalize();

		setStartEndNodes(lineStart, lineEnd, octreeAcc);

		if(checkLine(treeMin, treeMax, lineStart, lineDirection, octreeAcc) == 0){
			filteredEdges.push_back((*edges)[i]);
//			debug.addDebugLine((*edges)[i]->source->position, (*edges)[i]->target->position, Util::Color4f(0.5f, 0.5f, 0, 1), Util::Color4f(0, 1, 0, 1));
//			if(i > 0 && (*edges)[i]->source != (*edges)[i - 1]->source) falseEdges--;
//			if(falseEdges <= 0){
				std::cout << "Edge number: " << i << std::endl;
//				break;
//			}
		} else {
			debug.addDebugLine(lineStart, lineEnd, Util::Color4f(0.5f, 0.5f, 1, 1), Util::Color4f(0, 0, 1, 1));
		}
		break;
	}

	*edges = filteredEdges;
}

//return 1, if the line shall be deleted, 0 if not
int LightNodeManager::checkLine(Geometry::Vec3 octreeMin, Geometry::Vec3 octreeMax, Geometry::Vec3 rayOrigin, Geometry::Vec3 rayDirection, Util::Reference<Util::PixelAccessor> octreeAcc){
	Geometry::Vec3 newRayStart = rayOrigin;
	Geometry::Vec3 newRayDir = rayDirection;

//	debug.addDebugLine(rayOrigin, Geometry::Vec3(rayOrigin.x(), rayOrigin.y() + 0.1f, rayOrigin.z()), Util::Color4f(1, 0, 0, 1), Util::Color4f(1, 1, 0, 1));
//	debug.addDebugLine(rayOrigin, rayOrigin + rayDirection, Util::Color4f(1, 0, 0, 1), Util::Color4f(1, 1, 0, 1));
	//change lookup to only walk in positive directions
	octreeLookupDifference = 0;
	if(newRayDir.x() < 0.0){
		newRayStart.setX(newRayStart.x() + 2 * (lightingArea.center.x() - newRayStart.x()));
		newRayDir.setX(-newRayDir.x());
		octreeLookupDifference |= 4;
	}
	if(newRayDir.y() < 0.0){
		newRayStart.setY(newRayStart.y() + 2 * (lightingArea.center.y() - newRayStart.y()));
		newRayDir.setY(-newRayDir.y());
		octreeLookupDifference |= 2;
	}
	if(newRayDir.z() < 0.0){
		newRayStart.setZ(newRayStart.z() + 2 * (lightingArea.center.z() - newRayStart.z()));
		newRayDir.setZ(-newRayDir.z());
		octreeLookupDifference |= 1;
	}

	debug.addDebugLine(newRayStart, Geometry::Vec3(newRayStart.x(), newRayStart.y() + 0.1f, newRayStart.z()), Util::Color4f(0, 1, 0, 1), Util::Color4f(0, 1, 1, 1));
	debug.addDebugLine(newRayStart, newRayStart + newRayDir, Util::Color4f(0, 1, 0, 1), Util::Color4f(0, 1, 1, 1));

	//calculate the "boundings" for the ray within the octree
	Geometry::Vec3 t0 = (octreeMin - newRayStart);// / newRayDir;
	Geometry::Vec3 t1 = (octreeMax - newRayStart);// / newRayDir;
	t0.setX(t0.x() / ((newRayDir.x() == 0) ? 0.00001 : newRayDir.x()));
	t0.setY(t0.y() / ((newRayDir.y() == 0) ? 0.00001 : newRayDir.y()));
	t0.setZ(t0.z() / ((newRayDir.z() == 0) ? 0.00001 : newRayDir.z()));
	t1.setX(t1.x() / ((newRayDir.x() == 0) ? 0.00001 : newRayDir.x()));
	t1.setY(t1.y() / ((newRayDir.y() == 0) ? 0.00001 : newRayDir.y()));
	t1.setZ(t1.z() / ((newRayDir.z() == 0) ? 0.00001 : newRayDir.z()));

//	debug.addDebugBoxLinesMinMax(octreeMin, octreeMax);
//	debug.addDebugBoxLinesMinMax(t0, t1);

	//look, if the line is inside the octree (TODO: might be skipped, since it will always be true in our scenario)
	if(std::max(t0.x(), std::max(t0.y(), t0.z())) < std::min(t1.x(), std::min(t1.y(), t1.z()))){
		//do the actual intersection test
		return testIntersection(t0, t1, newRayStart, rayOrigin, newRayDir, octreeAcc);
	} else {
		//if the line is not inside the box, delete it (since the box surrounds the whole scene), should never happen though
		return 1;
	}
}

int LightNodeManager::testIntersection(Geometry::Vec3 t0, Geometry::Vec3 t1, Geometry::Vec3 lineStart, Geometry::Vec3 origLineStart, Geometry::Vec3 lineDir, Util::Reference<Util::PixelAccessor> octreeAcc){
	int curDepth = 0;
	bool moveUp = false;

	//the stack to iteratively simulate the recursive function calls
	FilterEdgeState nodeStates[VOXEL_OCTREE_DEPTH + 1];

	firstNodeRoot(t0, t1, origLineStart, nodeStates, &curDepth, octreeAcc);

//	Geometry::Vec3 midpos = lineStart + Geometry::Vec3(lineDir.x() * t0.x(), lineDir.y() * t0.y(), lineDir.z() * t0.z());;
//	debug.addDebugBox(midpos, 0.01);
//	Geometry::Vec3 midpos2 = lineStart + Geometry::Vec3(lineDir.x() * t1.x(), lineDir.y() * t1.y(), lineDir.z() * t1.z());;
//	debug.addDebugBox(midpos2, 0.1);

	Geometry::Vec3 midPoints[VOXEL_OCTREE_DEPTH + 1];
	midPoints[0] = lightingArea.center;
	debug.addDebugBoxLines(midPoints[0], (4 * quarterSizeOfRootNode) / (1 << 0));
	std::cout << "lookupDifference: " << octreeLookupDifference << std::endl;
	std::cout << "Child ids: " << (nodeStates[0].childID ^ octreeLookupDifference);
	for(unsigned int i = 1; i < curDepth; i++){
		std::cout << " " << (nodeStates[i].childID ^ octreeLookupDifference) << " (" << (nodeStates[i].tm.z()) << ")";
		getNewMidPos(midPoints[i - 1], nodeStates[i - 1].childID ^ octreeLookupDifference, (i - 1), &midPoints[i]);
		debug.addDebugBoxLines(midPoints[i], (4 * quarterSizeOfRootNode) / (1 << i));
	}
	std::cout << std::endl;

////	if(bigChange){
//		gl_FragColor = (1 + dot(t1, lineDir)) / 255.0;
////	} else {
////		gl_FragColor = 2 / 255.0;
////	}
//	return;

//	gl_FragColor = curDepth / 255.0;// nodeStates[curDepth-1].childID / 255.0;
//	return;

	//test, if the maximum point is inside the node area TODO: needed? if yes needed for every node?
//	if(t1.x < 0.0 || t1.y < 0.0 || t1.z < 0.0) return;
//
//	//initialize the root node data
//	nodeStates[curDepth].t0 = t0;
//	nodeStates[curDepth].t1 = t1;
//	nodeStates[curDepth].nodeOffset = 0;
//	nodeStates[curDepth].tm = 0.5 * (t0 + t1);
//	nodeStates[curDepth].childID = getChildOffset(lightingArea.center, lineStart); //firstNode(nodeStates[curDepth].t0, nodeStates[curDepth].tm);

	do {
		if(nodeStates[curDepth - 1].childID < 8 && !moveUp){
			std::cout << "childID was " << (nodeStates[curDepth - 1].childID ^ octreeLookupDifference) << std::endl;
			//Way is blocked, if not the same field as start or ending of the node
			if(nodeStates[curDepth - 1].nodeOffset == endNodeID){
				std::cout << "found end " << (nodeStates[curDepth - 1].childID ^ octreeLookupDifference) << std::endl;
				//currentNode is the endPoint of the line, so the search is finished and no interference with objects was found
//				gl_FragColor = 3.0 / 255.0;
//				return;
				return 0;
			} else if(curDepth >= VOXEL_OCTREE_DEPTH + 1){
				std::cout << "end not found: " << nodeStates[curDepth - 1].nodeOffset << ", but end: " << endNodeID << " with child " << (nodeStates[curDepth - 1].childID ^ octreeLookupDifference) << std::endl;
				if(nodeStates[VOXEL_OCTREE_DEPTH].nodeOffset == startNodeID){
					std::cout << "found start" << std::endl;
					//current node is the startPoint of the line, so do not delete edge here, move on searching
//					gl_FragColor = 2.0 / 255.0;
//					return;
//					return 0;
					moveUp = true;
				} else {
					std::cout << "start not found: " << nodeStates[curDepth - 1].nodeOffset << ", but end: " << startNodeID << " with child " << (nodeStates[curDepth - 1].childID ^ octreeLookupDifference) << std::endl;
					//current node is not the start or end point of the line, so delete the edge
//					imageStore(voxelOctree, getNodeIndex(curNodeID * NODE_SIZE), uvec4(1, 0, 0, 0));
					return 1;
				}
			} else {
				std::cout << "traveling down" << " with child " << (nodeStates[curDepth - 1].childID ^ octreeLookupDifference) << std::endl;
				//travel down to a leaf
//				gl_FragColor = 4.0 / 255.0;
//				return;
				Geometry::Vec2i childIndex;
				getNodeIndexVoxelOctree(nodeStates[curDepth - 1].nodeOffset * VOXEL_OCTREE_SIZE_PER_NODE + (nodeStates[curDepth - 1].childID ^ octreeLookupDifference), &childIndex);
				nodeStates[curDepth].nodeOffset = (int)(octreeAcc->readColor4f(childIndex.x(), childIndex.y()).r());
				if(nodeStates[curDepth].nodeOffset == 0){
					std::cout << "node not existent" << " with child " << (nodeStates[curDepth - 1].childID ^ octreeLookupDifference) << std::endl;
					//node did not exist, so move one level up again
//					gl_FragColor = (curDepth + 6) / 255.0;
//					return;
					curDepth++;
					moveUp = true;
				} else {
					std::cout << "node exists" << " with child " << (nodeStates[curDepth - 1].childID ^ octreeLookupDifference) << std::endl;
					getNewMidPos(midPoints[curDepth - 1], nodeStates[curDepth - 1].childID ^ octreeLookupDifference, (curDepth - 1), &midPoints[curDepth]);
					debug.addDebugBoxLines(midPoints[curDepth], (4 * quarterSizeOfRootNode) / (1 << curDepth));
					//node exists, so fill stack with data
//					gl_FragColor = 4.0 / 255.0;
//					return;
					switch(nodeStates[curDepth - 1].childID){
					case 0:
						nodeStates[curDepth].t0 = nodeStates[curDepth - 1].t0;
						nodeStates[curDepth].t1 = nodeStates[curDepth - 1].tm;
						break;
					case 1:
						nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].t0.x(), nodeStates[curDepth - 1].t0.y(), nodeStates[curDepth - 1].tm.z());
						nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t1.z());
						break;
					case 2:
						nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].t0.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t0.z());
						nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t1.y(), nodeStates[curDepth - 1].tm.z());
						break;
					case 3:
						nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].t0.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].tm.z());
						nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t1.y(), nodeStates[curDepth - 1].t1.z());
						break;
					case 4:
						nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t0.y(), nodeStates[curDepth - 1].t0.z());
						nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].t1.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].tm.z());
						break;
					case 5:
						nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t0.y(), nodeStates[curDepth - 1].tm.z());
						nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].t1.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t1.z());
						break;
					case 6:
						nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t0.z());
						nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].t1.x(), nodeStates[curDepth - 1].t1.y(), nodeStates[curDepth - 1].tm.z());
						break;
					case 7:
						nodeStates[curDepth].t0 = nodeStates[curDepth - 1].tm;
						nodeStates[curDepth].t1 = nodeStates[curDepth - 1].t1;
						break;
					}
					nodeStates[curDepth].tm = (nodeStates[curDepth].t0 + nodeStates[curDepth].t1) * 0.5f;
					nodeStates[curDepth].childID = firstNode(nodeStates[curDepth].t0, nodeStates[curDepth].tm);
					std::cout << "New childID = " << nodeStates[curDepth].childID << std::endl;
					debug.addDebugLine(midPoints[curDepth - 1], midPoints[curDepth]);
					curDepth++;
				}
			}
		} else {
			std::cout << "traveling up" << " with child " << (nodeStates[curDepth - 1].childID ^ octreeLookupDifference) << std::endl;
//			gl_FragColor = 5.0 / 255.0;
//			return;
			moveUp = false;
			curDepth--;
			debug.addDebugLine(midPoints[curDepth], midPoints[curDepth - 1], Util::Color4f(0, 1, 0, 1), Util::Color4f(1, 1, 0, 1));
			std::cout << "child is now " << (nodeStates[curDepth - 1].childID ^ octreeLookupDifference) << " (" << nodeStates[curDepth - 1].childID << ")" << std::endl;
			switch(nodeStates[curDepth - 1].childID){
			case 0:
				nodeStates[curDepth - 1].childID = nextNode(nodeStates[curDepth - 1].tm, 4, 2, 1);
				break;
			case 1:
				nodeStates[curDepth - 1].childID = nextNode(Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t1.z()), 5, 3, 8);
				break;
			case 2:
				nodeStates[curDepth - 1].childID = nextNode(Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t1.y(), nodeStates[curDepth - 1].tm.z()), 6, 8, 3);
				break;
			case 3:
				nodeStates[curDepth - 1].childID = nextNode(Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t1.y(), nodeStates[curDepth - 1].t1.z()), 7, 8, 8);
				break;
			case 4:
				nodeStates[curDepth - 1].childID = nextNode(Geometry::Vec3(nodeStates[curDepth - 1].t1.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].tm.z()), 8, 6, 5);
				break;
			case 5:
				nodeStates[curDepth - 1].childID = nextNode(Geometry::Vec3(nodeStates[curDepth - 1].t1.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t1.z()), 8, 7, 8);
				break;
			case 6:
				nodeStates[curDepth - 1].childID = nextNode(Geometry::Vec3(nodeStates[curDepth - 1].t1.x(), nodeStates[curDepth - 1].t1.y(), nodeStates[curDepth - 1].tm.z()), 8, 8, 7);
				break;
			case 7:
				nodeStates[curDepth - 1].childID = 8;
				break;
			}
//			getNewMidPos(midPoints[curDepth - 1], nodeStates[curDepth - 1].childID ^ octreeLookupDifference, (curDepth - 1), &midPoints[curDepth]);
//			debug.addDebugBoxLines(midPoints[curDepth], (4 * quarterSizeOfRootNode) / (1 << curDepth));
		}
	} while(nodeStates[0].childID < 8);

	return 1;
}

void LightNodeManager::firstNodeRoot(Geometry::Vec3 t0, Geometry::Vec3 t1, Geometry::Vec3 lineStart, FilterEdgeState* nodeStates, int* depth, Util::Reference<Util::PixelAccessor> octreeAcc){
	Geometry::Vec3 curMidPos;
	Geometry::Vec2i childIndex;

//	debug.addDebugLine(lineStart, Geometry::Vec3(lineStart.x(), lineStart.y() + 0.1f, lineStart.z()));
//	std::cout << "Line: " << lineStart.x() << "x" << lineStart.y() << "x" << lineStart.z() << std::endl;
//	std::cout << "lookupDifference: " << octreeLookupDifference << std::endl;
//
//	debug.addDebugBox(lineStart, 0.01);

	int curDepth = 0;

	std::cout << "Values: " << t0.z() << " " << t1.z() << std::endl;

	nodeStates[0].t0 = t0;
	nodeStates[0].t1 = t1;
	nodeStates[0].nodeOffset = 0;
	nodeStates[0].tm = (t0 + t1) * 0.5f;
	nodeStates[0].childID = getChildOffset(lightingArea.center, lineStart) ^ octreeLookupDifference;
//	std::cout << "childID: " << nodeStates[0].childID << std::endl;
	getNewMidPos(lightingArea.center, nodeStates[0].childID ^ octreeLookupDifference, 0, &curMidPos);
//	childIndex = getNodeIndexVoxelOctree(0 * NODE_SIZE + nodeStates[0].childID);
//	nodeStates[0].nodeOffset = int(imageLoad(voxelOctree, childIndex).r);

	for(curDepth = 1; curDepth < VOXEL_OCTREE_DEPTH + 1; curDepth++){
//		debug.addDebugBoxLines(curMidPos, (4 * quarterSizeOfRootNode) / (1 << curDepth));

		nodeStates[curDepth].childID = getChildOffset(curMidPos, lineStart) ^ octreeLookupDifference;
		getNewMidPos(curMidPos, nodeStates[curDepth].childID ^ octreeLookupDifference, curDepth, &curMidPos);
		getNodeIndexVoxelOctree(nodeStates[curDepth - 1].nodeOffset * VOXEL_OCTREE_SIZE_PER_NODE + nodeStates[curDepth - 1].childID ^ octreeLookupDifference, &childIndex);
		nodeStates[curDepth].nodeOffset = (int)(octreeAcc->readColor4f(childIndex.x(), childIndex.y()).r());

		if(nodeStates[curDepth].nodeOffset == 0){
//			curDepth--;
			break;
		}

		switch(nodeStates[curDepth - 1].childID){
		case 0:
			nodeStates[curDepth].t0 = nodeStates[curDepth - 1].t0;
			nodeStates[curDepth].t1 = nodeStates[curDepth - 1].tm;
			break;
		case 1:
			nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].t0.x(), nodeStates[curDepth - 1].t0.y(), nodeStates[curDepth - 1].tm.z());
			nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t1.z());
			break;
		case 2:
			nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].t0.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t0.z());
			nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t1.y(), nodeStates[curDepth - 1].tm.z());
			break;
		case 3:
			nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].t0.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].tm.z());
			nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t1.y(), nodeStates[curDepth - 1].t1.z());
			break;
		case 4:
			nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t0.y(), nodeStates[curDepth - 1].t0.z());
			nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].t1.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].tm.z());
			break;
		case 5:
			nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].t0.y(), nodeStates[curDepth - 1].tm.z());
			nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].t1.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t1.z());
			break;
		case 6:
			nodeStates[curDepth].t0 = Geometry::Vec3(nodeStates[curDepth - 1].tm.x(), nodeStates[curDepth - 1].tm.y(), nodeStates[curDepth - 1].t0.z());
			nodeStates[curDepth].t1 = Geometry::Vec3(nodeStates[curDepth - 1].t1.x(), nodeStates[curDepth - 1].t1.y(), nodeStates[curDepth - 1].tm.z());
			break;
		case 7:
			nodeStates[curDepth].t0 = nodeStates[curDepth - 1].tm;
			nodeStates[curDepth].t1 = nodeStates[curDepth - 1].t1;
			break;
		}

//		debug.addDebugBoxLines(nodeStates[curDepth].t0, nodeStates[curDepth].t1);

		nodeStates[curDepth].tm = (nodeStates[curDepth].t0 + nodeStates[curDepth].t1) * 0.5f;
	}

	std::cout << "Running to depth " << curDepth << std::endl;

	*depth = curDepth;


//	nodeStates[0].t0 = t0;
//	nodeStates[0].t1 = t1;
//	nodeStates[0].nodeOffset = 0;
//	nodeStates[0].tm = 0.5 * (t0 + t1);
//	nodeStates[0].childID = getChildOffset(lightingArea.center, lineStart);
//	vec3 curMidPos = getNewMidPos(lightingArea.center, nodeStates[0].childID, 0);
//
////	for(int i = 0; i < MAX_TREE_DEPTH; i++){
////		nodeStates[i].childID = 1;
////	}
//
//	for(curDepth = 1; curDepth < MAX_TREE_DEPTH; ++curDepth){
//		ivec2 childIndex = getNodeIndexVoxelOctree(nodeStates[curDepth - 1].nodeOffset * NODE_SIZE + (nodeStates[curDepth - 1].childID));
//		nodeStates[curDepth].nodeOffset = int(imageLoad(voxelOctree, childIndex).r);
//
//		if(nodeStates[curDepth].nodeOffset == 0){
//			curDepth--;
//			break;
//		}
//
//		switch(nodeStates[curDepth - 1].childID ^ octreeLookupDifference){
//		case 0:
//			nodeStates[curDepth].t0 = nodeStates[curDepth - 1].t0;
//			nodeStates[curDepth].t1 = nodeStates[curDepth - 1].tm;
//			break;
//		case 1:
//			nodeStates[curDepth].t0 = vec3(nodeStates[curDepth - 1].t0.xy, nodeStates[curDepth - 1].tm.z);
//			nodeStates[curDepth].t1 = vec3(nodeStates[curDepth - 1].tm.xy, nodeStates[curDepth - 1].t1.z);
//			break;
//		case 2:
//			nodeStates[curDepth].t0 = vec3(nodeStates[curDepth - 1].t0.x, nodeStates[curDepth - 1].tm.y, nodeStates[curDepth - 1].t0.z);
//			nodeStates[curDepth].t1 = vec3(nodeStates[curDepth - 1].tm.x, nodeStates[curDepth - 1].t1.y, nodeStates[curDepth - 1].tm.z);
//			break;
//		case 3:
//			nodeStates[curDepth].t0 = vec3(nodeStates[curDepth - 1].t0.x, nodeStates[curDepth - 1].tm.yz);
//			nodeStates[curDepth].t1 = vec3(nodeStates[curDepth - 1].tm.x, nodeStates[curDepth - 1].t1.yz);
//			break;
//		case 4:
//			nodeStates[curDepth].t0 = vec3(nodeStates[curDepth - 1].tm.x, nodeStates[curDepth - 1].t0.yz);
//			nodeStates[curDepth].t1 = vec3(nodeStates[curDepth - 1].t1.x, nodeStates[curDepth - 1].tm.yz);
//			break;
//		case 5:
//			nodeStates[curDepth].t0 = vec3(nodeStates[curDepth - 1].tm.x, nodeStates[curDepth - 1].t0.y, nodeStates[curDepth - 1].tm.z);
//			nodeStates[curDepth].t1 = vec3(nodeStates[curDepth - 1].t1.x, nodeStates[curDepth - 1].tm.y, nodeStates[curDepth - 1].t1.z);
//			break;
//		case 6:
//			nodeStates[curDepth].t0 = vec3(nodeStates[curDepth - 1].tm.xy, nodeStates[curDepth - 1].t0.z);
//			nodeStates[curDepth].t1 = vec3(nodeStates[curDepth - 1].t1.xy, nodeStates[curDepth - 1].tm.z);
//			break;
//		case 7:
//			nodeStates[curDepth].t0 = nodeStates[curDepth - 1].tm;
//			nodeStates[curDepth].t1 = nodeStates[curDepth - 1].t1;
//			break;
//		}
//		nodeStates[curDepth].tm = 0.5 * (nodeStates[curDepth - 1].t0 + nodeStates[curDepth - 1].t1);
//		nodeStates[curDepth].childID = getChildOffset(curMidPos, lineStart);
//		curMidPos = getNewMidPos(curMidPos, nodeStates[curDepth].childID, curDepth);
//	}

//	nodeStates[curDepth - 1].nodeOffset = getNodeID(lineStart);
}

int LightNodeManager::nextNode(Geometry::Vec3 tm, int next1, int next2, int next3){
	//return the int according to the smallest float
	if(tm.x() < tm.y()){
		if(tm.x() < tm.z()){
			std::cout << "returning 1 (" << next1 << "), because tmX < tmY and tmX < tmZ: " << tm.x() << " < " << tm.y() << " and " << tm.x() << " < " << tm.z() << std::endl;
			return next1;
		} else {
			std::cout << "returning 3 (" << next3 << "), because tmX < tmY and tmX >= tmZ: " << tm.x() << " < " << tm.y() << " and " << tm.x() << " >= " << tm.z() << std::endl;
			return next3;
		}
	} else {
		if(tm.y() < tm.z()){
			std::cout << "returning 2 (" << next2 << "), because tmX >= tmY and tmY < tmZ: " << tm.x() << " >= " << tm.y() << " and " << tm.y() << " < " << tm.z() << std::endl;
			return next2;
		} else {
			std::cout << "returning 3 (" << next3 << "), because tmX >= tmY and tmY >= tmZ: " << tm.x() << " >= " << tm.y() << " and " << tm.y() << " >= " << tm.z() << std::endl;
			return next3;
		}
	}
}

int LightNodeManager::firstNode(Geometry::Vec3 t0, Geometry::Vec3 tm){
	int childID = 0;
	if(t0.x() > t0.y()){
		if(t0.x() > t0.z()){
			std::cout << "x > y && x > z" << ": " << t0.x() << " > " << t0.y() << " && " << t0.x() << " > " << t0.z() << std::endl;
			//plane YZ
			if(tm.y() < t0.x()) childID += 2;
			if(tm.z() < t0.x()) childID += 4;
		} else {
			std::cout << "x > y && x <= z" << ": " << t0.x() << " > " << t0.y() << " && " << t0.x() << " <= " << t0.z() << std::endl;
			//plane XY
			if(tm.x() < t0.z()) childID += 1;
			if(tm.y() < t0.z()) childID += 2;
		}
	} else {
		if(t0.y() > t0.z()){
			std::cout << "x <= y && y > z" << ": " << t0.x() << " <= " << t0.y() << " && " << t0.y() << " > " << t0.z() << std::endl;
			//plane XZ
			if(tm.x() < t0.y()) childID += 1;
			if(tm.z() < t0.y()) childID += 4;
		} else {
			std::cout << "x <= y && y <= z" << ": " << t0.x() << " <= " << t0.y() << " && " << t0.y() << " <= " << t0.z() << std::endl;
			//plane XY
			if(tm.x() < t0.z()) childID += 1;
			if(tm.y() < t0.z()) childID += 2;
		}
	}
	return childID;
}

void LightNodeManager::setStartEndNodes(Geometry::Vec3 posStart, Geometry::Vec3 posEnd, Util::Reference<Util::PixelAccessor> octreeAcc){
	startNodeID = getNodeID(posStart, octreeAcc);
	endNodeID = getNodeID(posEnd, octreeAcc);
}

int LightNodeManager::getNodeID(Geometry::Vec3 pos, Util::Reference<Util::PixelAccessor> octreeAcc){
	int curNodeID = 0;
	int lastID;
	Geometry::Vec3 curMidPos = lightingArea.center;
	int childOffset;
	Geometry::Vec2i childIndex;

	for(int i = 0; i < VOXEL_OCTREE_DEPTH; i++){
		childOffset = getChildOffset(curMidPos, pos);
		getNewMidPos(curMidPos, childOffset, i, &curMidPos);
		getNodeIndexVoxelOctree(curNodeID * VOXEL_OCTREE_SIZE_PER_NODE + childOffset, &childIndex);
		lastID = curNodeID;
		curNodeID = (int)(octreeAcc->readColor4f(childIndex.x(), childIndex.y()).r());
		if(curNodeID == 0) return lastID;
	}

	return curNodeID;
}

void LightNodeManager::getNewMidPos(Geometry::Vec3 oldMidPos, int childOffset, int curDepth, Geometry::Vec3* newMidPos){
	switch(childOffset){
	case 0: *newMidPos = Geometry::Vec3(-quarterSizeOfRootNode, -quarterSizeOfRootNode, quarterSizeOfRootNode) / (1 << curDepth) + oldMidPos; break;
	case 1: *newMidPos = Geometry::Vec3(-quarterSizeOfRootNode, -quarterSizeOfRootNode, -quarterSizeOfRootNode) / (1 << curDepth) + oldMidPos; break;
	case 2: *newMidPos = Geometry::Vec3(-quarterSizeOfRootNode, quarterSizeOfRootNode, quarterSizeOfRootNode) / (1 << curDepth) + oldMidPos; break;
	case 3: *newMidPos = Geometry::Vec3(-quarterSizeOfRootNode, quarterSizeOfRootNode, -quarterSizeOfRootNode) / (1 << curDepth) + oldMidPos; break;
	case 4: *newMidPos = Geometry::Vec3(quarterSizeOfRootNode, -quarterSizeOfRootNode, quarterSizeOfRootNode) / (1 << curDepth) + oldMidPos; break;
	case 5: *newMidPos = Geometry::Vec3(quarterSizeOfRootNode, -quarterSizeOfRootNode, -quarterSizeOfRootNode) / (1 << curDepth) + oldMidPos; break;
	case 6: *newMidPos = Geometry::Vec3(quarterSizeOfRootNode, quarterSizeOfRootNode, quarterSizeOfRootNode) / (1 << curDepth) + oldMidPos; break;
	case 7: *newMidPos = Geometry::Vec3(quarterSizeOfRootNode, quarterSizeOfRootNode, -quarterSizeOfRootNode) / (1 << curDepth) + oldMidPos; break;
	default: *newMidPos = Geometry::Vec3(quarterSizeOfRootNode, quarterSizeOfRootNode, quarterSizeOfRootNode) / (1 << curDepth) + oldMidPos; break;	//must never happen
	}
}

int LightNodeManager::getChildOffset(Geometry::Vec3 midPos, Geometry::Vec3 targetPos){
	int offset = 0;
	if(targetPos.x() > midPos.x()) offset += 4;
	if(targetPos.y() > midPos.y()) offset += 2;
	if(targetPos.z() < midPos.z()) offset += 1;
	return offset;
}

void LightNodeManager::getNodeIndexVoxelOctree(int nodeID, Geometry::Vec2i* index){
	index->setX(nodeID % voxelOctreeTextureSize);
	index->setY((int)(nodeID / voxelOctreeTextureSize));
}

void LightNodeManager::createNodeTextures(){
	//count the nodes
	unsigned int numNodes = lightNodeLightMaps.size();
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
        numNodes += lightNodeMaps[i]->lightNodes.size();
	}

	std::cout << "Number nodes: " << numNodes << std::endl;

	std::cout << "filling nodes" << std::endl;

	//create the node textures
	const unsigned int dataLengthPerNode = 3;
	unsigned int nodeTextureSize = std::ceil(std::sqrt(numNodes * dataLengthPerNode));
	nodeTextureStatic = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, nodeTextureSize, nodeTextureSize, 1, Util::TypeConstant::UINT32, 1);
	nodeTextureRendering[0] = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, nodeTextureSize, nodeTextureSize, 1, Util::TypeConstant::UINT32, 1);
	nodeTextureRendering[1] = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, nodeTextureSize, nodeTextureSize, 1, Util::TypeConstant::UINT32, 1);

	std::cout << "filling nodes normal" << std::endl;

	//fill the node textures and set the node id's
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *nodeTextureStatic.get());
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
		for(unsigned int j = 0; j < lightNodeMaps[i]->lightNodes.size(); j++){
			unsigned int x = (lightNodeMaps[i]->lightNodes[j]->id * dataLengthPerNode) % nodeTextureSize;
			unsigned int y = (lightNodeMaps[i]->lightNodes[j]->id * dataLengthPerNode) / nodeTextureSize;

			acc->writeColor(x, y, Util::Color4f(0, 0, 0, 0));
			x = (x + 1) % nodeTextureSize;
			if(x == 0) y++;
			acc->writeColor(x, y, Util::Color4f(0, 0, 0, 0));
			x = (x + 1) % nodeTextureSize;
			if(x == 0) y++;
			acc->writeColor(x, y, Util::Color4f(0, 0, 0, 0));
		}
	}

	std::cout << "filling nodes with light" << std::endl;

	//fill with the light nodes, too
	for(unsigned int i = 0; i < lightNodeLightMaps.size(); i++){
		unsigned int x = (lightNodeLightMaps[i]->light.id * dataLengthPerNode) % nodeTextureSize;
		unsigned int y = (lightNodeLightMaps[i]->light.id * dataLengthPerNode) / nodeTextureSize;

		acc->writeColor(x, y, Util::Color4f(lightNodeLightMaps[i]->light.color.r() * LIGHT_STRENGTH, 0, 0, 0));
		x = (x + 1) % nodeTextureSize;
		if(x == 0) y++;
		acc->writeColor(x, y, Util::Color4f(lightNodeLightMaps[i]->light.color.g() * LIGHT_STRENGTH, 0, 0, 0));
		x = (x + 1) % nodeTextureSize;
		if(x == 0) y++;
		acc->writeColor(x, y, Util::Color4f(lightNodeLightMaps[i]->light.color.b() * LIGHT_STRENGTH, 0, 0, 0));
	}
}

void LightNodeManager::createEdgeTextures(){
	//count the edges
	unsigned int numEdges = 0;
	for(unsigned int i = 0; i < lightNodeLightMaps.size(); i++){
		numEdges += lightNodeLightMaps[i]->staticEdges.size();
		numEdges += lightNodeLightMaps[i]->dynamicEdges.size();
	}
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
		numEdges += lightNodeMaps[i]->internalLightEdges.size();
		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesStatic.size(); j++){
			numEdges += lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.size();
		}
		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesDynamic.size(); j++){
			numEdges += lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges.size();
		}
	}
	curNumEdges = numEdges;

	std::cout << "filling edges" << std::endl;

	//create the edge texture
	unsigned int nodeDataLengthPerEdge = 2;
	unsigned int weightDataLengthPerEdge = 3;
	unsigned int tmpEdgeTextureSize = std::ceil(std::sqrt(numEdges));
	unsigned int nodeEdgeTextureSize = std::ceil(std::sqrt(numEdges * nodeDataLengthPerEdge));
	unsigned int weightEdgeTextureSize = std::ceil(std::sqrt(numEdges * weightDataLengthPerEdge));
	tmpTexEdgeSize = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, tmpEdgeTextureSize, tmpEdgeTextureSize, 1, Util::TypeConstant::UINT8, 1);
	edgeTextureNodes = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, nodeEdgeTextureSize, nodeEdgeTextureSize, 1, Util::TypeConstant::UINT32, 1);
	edgeTextureWeights = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, weightEdgeTextureSize, weightEdgeTextureSize, 1, Util::TypeConstant::FLOAT, 1);
//	edgeTextureNodesSources = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, nodeEdgeTextureSize, nodeEdgeTextureSize, 1, Util::TypeConstant::UINT32, 1);
//	edgeTextureNodesTargets = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, nodeEdgeTextureSize, nodeEdgeTextureSize, 1, Util::TypeConstant::UINT32, 1);
//	edgeTextureWeightsR = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, weightEdgeTextureSize, weightEdgeTextureSize, 1, Util::TypeConstant::FLOAT, 1);
//	edgeTextureWeightsG = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, weightEdgeTextureSize, weightEdgeTextureSize, 1, Util::TypeConstant::FLOAT, 1);
//	edgeTextureWeightsB = Rendering::TextureUtils::createDataTexture(Rendering::TextureType::TEXTURE_2D, weightEdgeTextureSize, weightEdgeTextureSize, 1, Util::TypeConstant::FLOAT, 1);

	//fill the edge texture
	unsigned int curEdgeID = 0;
	Util::Reference<Util::PixelAccessor> accEdgesNodes = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeTextureNodes.get());
	Util::Reference<Util::PixelAccessor> accEdgesWeights = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeTextureWeights.get());
//	Util::Reference<Util::PixelAccessor> accEdgesSources = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeTextureNodesSources.get());
//	Util::Reference<Util::PixelAccessor> accEdgesTargets = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeTextureNodesTargets.get());
//	Util::Reference<Util::PixelAccessor> accEdgesWeightsR = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeTextureWeightsR.get());
//	Util::Reference<Util::PixelAccessor> accEdgesWeightsG = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeTextureWeightsG.get());
//	Util::Reference<Util::PixelAccessor> accEdgesWeightsB = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *edgeTextureWeightsB.get());
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
		//internal edges
		for(unsigned int j = 0; j < lightNodeMaps[i]->internalLightEdges.size(); j++){
			unsigned int nodeX = (curEdgeID * nodeDataLengthPerEdge) % nodeEdgeTextureSize;
			unsigned int nodeY = (curEdgeID * nodeDataLengthPerEdge) / nodeEdgeTextureSize;
			unsigned int weightX = (curEdgeID * weightDataLengthPerEdge) % weightEdgeTextureSize;
			unsigned int weightY = (curEdgeID * weightDataLengthPerEdge) / weightEdgeTextureSize;

//			accEdgesSources->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->internalLightEdges[j]->source->id, 0, 0, 0));
//			accEdgesTargets->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->internalLightEdges[j]->target->id, 0, 0, 0));
//			accEdgesWeightsR->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->internalLightEdges[j]->weight.x());
//			accEdgesWeightsG->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->internalLightEdges[j]->weight.y());
//			accEdgesWeightsB->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->internalLightEdges[j]->weight.z());

			accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->internalLightEdges[j]->source->id, 0, 0, 0));
			nodeX = (nodeX + 1) % nodeEdgeTextureSize;
			if(nodeX == 0) nodeY++;
			accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->internalLightEdges[j]->target->id, 0, 0, 0));

			accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeMaps[i]->internalLightEdges[j]->weight.x());
			weightX = (weightX + 1) % weightEdgeTextureSize;
			if(weightX == 0) weightY++;
			accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeMaps[i]->internalLightEdges[j]->weight.y());
			weightX = (weightX + 1) % weightEdgeTextureSize;
			if(weightX == 0) weightY++;
			accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeMaps[i]->internalLightEdges[j]->weight.z());

			curEdgeID++;
		}
		//external edges
		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesStatic.size(); j++){
			for(unsigned int k = 0; k < lightNodeMaps[i]->externalLightEdgesStatic[j]->edges.size(); k++){
				LightNode *source = lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->source;
				LightNode *target = lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->target;

				unsigned int nodeX = (curEdgeID * nodeDataLengthPerEdge) % nodeEdgeTextureSize;
				unsigned int nodeY = (curEdgeID * nodeDataLengthPerEdge) / nodeEdgeTextureSize;
				unsigned int weightX = (curEdgeID * weightDataLengthPerEdge) % weightEdgeTextureSize;
				unsigned int weightY = (curEdgeID * weightDataLengthPerEdge) / weightEdgeTextureSize;

//				accEdgesSources->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->externalLightEdgesStatic[j]->source->id, 0, 0, 0));
//				accEdgesTargets->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->externalLightEdgesStatic[j]->target->id, 0, 0, 0));
//				accEdgesWeightsR->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->externalLightEdgesStatic[j]->weight.x());
//				accEdgesWeightsG->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->externalLightEdgesStatic[j]->weight.y());
//				accEdgesWeightsB->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->externalLightEdgesStatic[j]->weight.z());

				accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->source->id, 0, 0, 0));
				nodeX = (nodeX + 1) % nodeEdgeTextureSize;
				if(nodeX == 0) nodeY++;
				accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->target->id, 0, 0, 0));

				accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->weight.x());
				weightX = (weightX + 1) % weightEdgeTextureSize;
				if(weightX == 0) weightY++;
				accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->weight.y());
				weightX = (weightX + 1) % weightEdgeTextureSize;
				if(weightX == 0) weightY++;
				accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeMaps[i]->externalLightEdgesStatic[j]->edges[k]->weight.z());

				curEdgeID++;
			}
		}
		for(unsigned int j = 0; j < lightNodeMaps[i]->externalLightEdgesDynamic.size(); j++){
			for(unsigned int k = 0; k < lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges.size(); k++){
				LightNode *source = lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k]->source;
				LightNode *target = lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k]->target;

				unsigned int nodeX = (curEdgeID * nodeDataLengthPerEdge) % nodeEdgeTextureSize;
				unsigned int nodeY = (curEdgeID * nodeDataLengthPerEdge) / nodeEdgeTextureSize;
				unsigned int weightX = (curEdgeID * weightDataLengthPerEdge) % weightEdgeTextureSize;
				unsigned int weightY = (curEdgeID * weightDataLengthPerEdge) / weightEdgeTextureSize;

//				accEdgesSources->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->externalLightEdgesDynamic[j]->source->id, 0, 0, 0));
//				accEdgesTargets->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->externalLightEdgesDynamic[j]->target->id, 0, 0, 0));
//				accEdgesWeightsR->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->externalLightEdgesDynamic[j]->weight.x());
//				accEdgesWeightsG->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->externalLightEdgesDynamic[j]->weight.y());
//				accEdgesWeightsB->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->externalLightEdgesDynamic[j]->weight.z());

				accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k]->source->id, 0, 0, 0));
				nodeX = (nodeX + 1) % nodeEdgeTextureSize;
				if(nodeX == 0) nodeY++;
				accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k]->target->id, 0, 0, 0));

				accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k]->weight.x());
				weightX = (weightX + 1) % weightEdgeTextureSize;
				if(weightX == 0) weightY++;
				accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k]->weight.y());
				weightX = (weightX + 1) % weightEdgeTextureSize;
				if(weightX == 0) weightY++;
				accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeMaps[i]->externalLightEdgesDynamic[j]->edges[k]->weight.z());

				curEdgeID++;
			}
		}
	}

	std::cout << "filling edges with light" << std::endl;

	//fill with edges from the lights
	for(unsigned int i = 0; i < lightNodeLightMaps.size(); i++){
		for(unsigned int j = 0; j < lightNodeLightMaps[i]->staticEdges.size(); j++){
			LightNode *source = lightNodeLightMaps[i]->staticEdges[j]->source;
			LightNode *target = lightNodeLightMaps[i]->staticEdges[j]->target;

			unsigned int nodeX = (curEdgeID * nodeDataLengthPerEdge) % nodeEdgeTextureSize;
			unsigned int nodeY = (curEdgeID * nodeDataLengthPerEdge) / nodeEdgeTextureSize;
			unsigned int weightX = (curEdgeID * weightDataLengthPerEdge) % weightEdgeTextureSize;
			unsigned int weightY = (curEdgeID * weightDataLengthPerEdge) / weightEdgeTextureSize;

//			accEdgesSources->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->internalLightEdges[j]->source->id, 0, 0, 0));
//			accEdgesTargets->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->internalLightEdges[j]->target->id, 0, 0, 0));
//			accEdgesWeightsR->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->internalLightEdges[j]->weight.x());
//			accEdgesWeightsG->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->internalLightEdges[j]->weight.y());
//			accEdgesWeightsB->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->internalLightEdges[j]->weight.z());

			accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeLightMaps[i]->staticEdges[j]->source->id, 0, 0, 0));
			nodeX = (nodeX + 1) % nodeEdgeTextureSize;
			if(nodeX == 0) nodeY++;
			accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeLightMaps[i]->staticEdges[j]->target->id, 0, 0, 0));

			accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeLightMaps[i]->staticEdges[j]->weight.x());
			weightX = (weightX + 1) % weightEdgeTextureSize;
			if(weightX == 0) weightY++;
			accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeLightMaps[i]->staticEdges[j]->weight.y());
			weightX = (weightX + 1) % weightEdgeTextureSize;
			if(weightX == 0) weightY++;
			accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeLightMaps[i]->staticEdges[j]->weight.z());

			curEdgeID++;
		}
		for(unsigned int j = 0; j < lightNodeLightMaps[i]->dynamicEdges.size(); j++){
			LightNode *source = lightNodeLightMaps[i]->dynamicEdges[j]->source;
			LightNode *target = lightNodeLightMaps[i]->dynamicEdges[j]->target;

			unsigned int nodeX = (curEdgeID * nodeDataLengthPerEdge) % nodeEdgeTextureSize;
			unsigned int nodeY = (curEdgeID * nodeDataLengthPerEdge) / nodeEdgeTextureSize;
			unsigned int weightX = (curEdgeID * weightDataLengthPerEdge) % weightEdgeTextureSize;
			unsigned int weightY = (curEdgeID * weightDataLengthPerEdge) / weightEdgeTextureSize;

//			accEdgesSources->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->internalLightEdges[j]->source->id, 0, 0, 0));
//			accEdgesTargets->writeColor(nodeX, nodeY, Util::Color4f(lightNodeMaps[i]->internalLightEdges[j]->target->id, 0, 0, 0));
//			accEdgesWeightsR->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->internalLightEdges[j]->weight.x());
//			accEdgesWeightsG->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->internalLightEdges[j]->weight.y());
//			accEdgesWeightsB->writeSingleValueFloat(nodeX, nodeY, lightNodeMaps[i]->internalLightEdges[j]->weight.z());

			accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeLightMaps[i]->dynamicEdges[j]->source->id, 0, 0, 0));
			nodeX = (nodeX + 1) % nodeEdgeTextureSize;
			if(nodeX == 0) nodeY++;
			accEdgesNodes->writeColor(nodeX, nodeY, Util::Color4f(lightNodeLightMaps[i]->dynamicEdges[j]->target->id, 0, 0, 0));

			accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeLightMaps[i]->dynamicEdges[j]->weight.x());
			weightX = (weightX + 1) % weightEdgeTextureSize;
			if(weightX == 0) weightY++;
			accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeLightMaps[i]->dynamicEdges[j]->weight.y());
			weightX = (weightX + 1) % weightEdgeTextureSize;
			if(weightX == 0) weightY++;
			accEdgesWeights->writeSingleValueFloat(weightX, weightY, lightNodeLightMaps[i]->dynamicEdges[j]->weight.z());

			curEdgeID++;
		}
	}
}

void LightNodeManager::propagateLight(){
	std::cout << "propagating light" << std::endl;

	//copy the light nodes
	copyTexture(nodeTextureStatic.get(), nodeTextureRendering[0].get());
	copyTexture(nodeTextureStatic.get(), nodeTextureRendering[1].get());
//	fillTexture(nodeTextureComplete.get(), Util::Color4f(0, 0, 0, 0));

//	copyTexture(nodeTextureStaticR.get(), nodeTextureTemporaryR.get());
//	copyTexture(nodeTextureStaticG.get(), nodeTextureTemporaryG.get());
//	copyTexture(nodeTextureStaticB.get(), nodeTextureTemporaryB.get());
//	fillTexture(nodeTextureCompleteR.get(), Util::Color4f(0, 0, 0, 0));
//	fillTexture(nodeTextureCompleteG.get(), Util::Color4f(0, 0, 0, 0));
//	fillTexture(nodeTextureCompleteB.get(), Util::Color4f(0, 0, 0, 0));

	propagateLightShader.get()->setUniform(*renderingContext, Rendering::Uniform("numEdges", (int32_t)curNumEdges));
	propagateLightShader.get()->setUniform(*renderingContext, Rendering::Uniform("outputTextureSize", (int32_t)tmpTexEdgeSize->getWidth()));
	propagateLightShader.get()->setUniform(*renderingContext, Rendering::Uniform("edgeNodesSize", (int32_t)edgeTextureNodes->getWidth()));
	propagateLightShader.get()->setUniform(*renderingContext, Rendering::Uniform("edgeWeightsSize", (int32_t)edgeTextureWeights->getWidth()));
	propagateLightShader.get()->setUniform(*renderingContext, Rendering::Uniform("nodeColorsSize", (int32_t)nodeTextureStatic->getWidth()));

	renderingContext->pushBoundImage(0);
	renderingContext->pushBoundImage(1);
	renderingContext->pushBoundImage(2);
	renderingContext->pushBoundImage(3);

	//write into a texture
	TextureProcessor textureProcessor;
	textureProcessor.setRenderingContext(renderingContext);
	textureProcessor.setInputTexture(tmpTexSmallest.get());
	textureProcessor.setOutputTexture(tmpTexEdgeSize.get());
	textureProcessor.setShader(propagateLightShader.get());

	curNodeTextureRenderingIndex = 0;
	unsigned int curCycle = 0;
	do {
		renderingContext->setBoundImage(0, Rendering::ImageBindParameters(edgeTextureNodes.get()));													//setting the node-texture
		renderingContext->setBoundImage(1, Rendering::ImageBindParameters(edgeTextureWeights.get()));												//setting the weights-texture
		renderingContext->setBoundImage(2, Rendering::ImageBindParameters(nodeTextureRendering[curNodeTextureRenderingIndex].get()));				//setting the source-texture
		renderingContext->setBoundImage(3, Rendering::ImageBindParameters(nodeTextureRendering[(curNodeTextureRenderingIndex + 1) % 2].get()));		//setting the target-texture

		textureProcessor.execute();

		curNodeTextureRenderingIndex = (curNodeTextureRenderingIndex + 1) % 2;

		curCycle++;
	} while(curCycle < NUMBER_LIGHT_PROPAGATION_CYCLES);

	renderingContext->popBoundImage(0);
	renderingContext->popBoundImage(1);
	renderingContext->popBoundImage(2);
	renderingContext->popBoundImage(3);


//	unsigned int counter = 0, counter2 = 0, counter3 = 0;
//	nodeTextureRendering[curNodeTextureRenderingIndex]->downloadGLTexture(*renderingContext);
////	fillTexture(nodeTextureRendering[curNodeTextureRenderingIndex].get(), Util::Color4f(1, 0, 0, 0));
//	Util::Reference<Util::PixelAccessor> acc2 = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *nodeTextureRendering[curNodeTextureRenderingIndex].get());
//	for(unsigned int i = 1; i < nodeTextureRendering[curNodeTextureRenderingIndex]->getWidth() * nodeTextureRendering[curNodeTextureRenderingIndex]->getHeight(); i++){
//		unsigned int x = i % nodeTextureRendering[curNodeTextureRenderingIndex]->getWidth();
//		unsigned int y = i / nodeTextureRendering[curNodeTextureRenderingIndex]->getWidth();
//		unsigned int xb = (i-1) % nodeTextureRendering[curNodeTextureRenderingIndex]->getWidth();
//		unsigned int yb = (i-1) / nodeTextureRendering[curNodeTextureRenderingIndex]->getWidth();
//
//		uint32_t valueBefore = acc2->readColor4f(xb, yb).r();//readSingleValueByte(xb, yb);
//		uint32_t value = acc2->readColor4f(x, y).r();//readSingleValueByte(x, y);
//		if(valueBefore != value || i % 1000 == 0 || i == 1){
//			std::cout << i << ": " << (unsigned int)value << " but " << (i-1) << ": " << (unsigned int)valueBefore << std::endl;
//			if(counter++ >= 100) break;
////			if(i >= 10) break;
//		}
////		if(value == 2){
////			counter++;
////		} else if(value == 4){
////			counter2++;
////		} else {
////			counter3++;
////		}
//	}
//	std::cout << "Counter: " << counter << " Counter2: " << counter2 << " Counter3: " << counter3 << std::endl;
}

void LightNodeManager::copyTexture(Rendering::Texture *source, Rendering::Texture *target){
	Util::Reference<Util::PixelAccessor> accSource = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *source);
	Util::Reference<Util::PixelAccessor> accTarget = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *target);
	for(unsigned int y = 0; y < target->getHeight(); y++){
		for(unsigned int x = 0; x < target->getWidth(); x++){
			accTarget->writeColor(x, y, accSource->readColor4f(x, y));
		}
	}
	target->dataChanged();
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
			acc.get()->writeColor(x, y, Util::Color4ub(value, 0, 0, 0));
		}
	}
	texture->dataChanged();
}

void LightNodeManager::fillTextureFloat(Rendering::Texture *texture, float value){
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *texture);
	for(unsigned int y = 0; y < texture->getHeight(); y++){
		for(unsigned int x = 0; x < texture->getWidth(); x++){
			acc.get()->writeSingleValueFloat(x, y, value);
		}
	}
	texture->dataChanged();
}

void LightNodeManager::createWorldBBCameras(MinSG::Node* node, Util::Reference<MinSG::CameraNodeOrtho> enclosingCameras[3]){
	const Geometry::Vec3 worldDirections[3] = {Geometry::Vec3(1, 0, 0), Geometry::Vec3(0, 1, 0), Geometry::Vec3(0, 0, 1)};
//	float maxExtendHalf = lightingArea.extend * 0.5f;
	float maxExtendHalf = node->getWorldBB().getExtentMax() * 0.5f;
//	float diameterHalf = lightRootNode->getWorldBB().getDiameter() * 0.5;

	for(unsigned int i = 0; i < 3; i++){
//		enclosingCameras[i]->setWorldPosition(lightingArea.center - worldDirections[i] * (maxExtendHalf + 1));
		enclosingCameras[i]->setWorldPosition(node->getWorldBB().getCenter() - worldDirections[i] * (maxExtendHalf + 1));
//		camera.setWorldPosition(rootCenter - worldDir * (diameterHalf + 1));
//		camera.setWorldPosition(rootCenter - (worldDir.getNormalized()) * (maxExtend + 1));
//		enclosingCameras[i].get()->rotateToWorldDir(worldDirections[i]);
		MinSG::Transformations::rotateToWorldDir(*enclosingCameras[i].get(), worldDirections[i]);

//		var frustum = Geometry.calcEnclosingOrthoFrustum(lightRootNode.getBB(), camera.getWorldMatrix().inverse() * lightRootNode.getWorldMatrix());
//		camera.setNearFar(frustum.getNear() * 0.99, frustum.getFar() * 1.01);

//		outln("fnear="+frustum.getNear()+" ffar="+frustum.getFar()+" ext="+maxExtend);

#ifdef TP_ACTIVATE_DEBUG
//		//DEBUG
//		Geometry::Vec3 up;
//		if(i == 1){
//			up = Geometry::Vec3(1, 0, 0);
//		} else {
//			up = Geometry::Vec3(0, 1, 0);
//		}
//		Geometry::Vec3 right = worldDirections[i].cross(up);
////		var offset = worldDir * frustum.getNear() * 0.99;
////		var offsetFar = worldDir * frustum.getFar() * 1.01;
//		Geometry::Vec3 offset = worldDirections[i];
////		Geometry::Vec3 offsetFar = worldDirections[i] * (lightingArea.extend + 1);
//		Geometry::Vec3 offsetFar = worldDirections[i] * (node->getWorldBB().getExtentMax() + 1);
////		outln("right: "+right+" offset: "+offset+" offsetFar: "+offsetFar);
//		float cl;
//		float cr;
//		float ct;
//		float cb;
//		//DEBUG END
#endif //TP_ACTIVATE_DEBUG

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

//		enclosingCameras[i].get()->setNearFar(-1, -lightingArea.extend - 1);
		enclosingCameras[i].get()->setNearFar(-1, -node->getWorldBB().getExtentMax() - 1);
//		camera.setNearFar(maxExtend + 1, 1);
//		camera.setNearFar(1, diameterHalf*2 + 1);
		enclosingCameras[i].get()->setClippingPlanes(-maxExtendHalf, maxExtendHalf, -maxExtendHalf, maxExtendHalf);

#ifdef TP_ACTIVATE_DEBUG
//		//DEBUG
//		cl = -maxExtendHalf;
//		cr = maxExtendHalf;
//		ct = maxExtendHalf;
//		cb = -maxExtendHalf;
//
//		Geometry::Vec3 debPos = enclosingCameras[i].get()->getWorldPosition();
//		debug.addDebugLine(debPos + offset + right * cl + up * cb, debPos + offset + right * cr + up * cb);
//		debug.addDebugLine(debPos + offset + right * cl + up * cb, debPos + offset + right * cl + up * ct);
//		debug.addDebugLine(debPos + offset + right * cr + up * cb, debPos + offset + right * cr + up * ct);
//		debug.addDebugLine(debPos + offset + right * cl + up * ct, debPos + offset + right * cr + up * ct);
//		debug.addDebugLine(debPos + offsetFar + right * cl + up * cb, debPos + offsetFar + right * cr + up * cb);
//		debug.addDebugLine(debPos + offsetFar + right * cl + up * cb, debPos + offsetFar + right * cl + up * ct);
//		debug.addDebugLine(debPos + offsetFar + right * cr + up * cb, debPos + offsetFar + right * cr + up * ct);
//		debug.addDebugLine(debPos + offsetFar + right * cl + up * ct, debPos + offsetFar + right * cr + up * ct);
////		outln("left: "+cl+" right: "+cr+" top: "+ct+" bottom: "+cb);
//
//		debug.addDebugLine(enclosingCameras[i].get()->getWorldPosition(), enclosingCameras[i].get()->getWorldPosition() + offset, Util::Color4f(0.0,0.0,1.0,1.0), Util::Color4f(1.0,0.0,0.0,1.0));
//		debug.addDebugLine(enclosingCameras[i].get()->getWorldPosition() + offset, enclosingCameras[i].get()->getWorldPosition() + offsetFar, Util::Color4f(0.0,1.0,0.0,1.0), Util::Color4f(1.0,0.0,0.0,1.0));
//		//DEBUG END
#endif //TP_ACTIVATE_DEBUG
	}
//	debug.addDebugLineCol2(cameras[0].getWorldPosition(), cameras[0].getWorldPosition() + worldDirections[0], new Util.Color4f(0.0,0.0,1.0,1.0), new Util.Color4f(1.0,0.0,0.0,1.0));
//	debug.addDebugLineCol2(cameras[0].getWorldPosition() + worldDirections[0], cameras[0].getWorldPosition() + worldDirections[0] * (maxExtend + 1), new Util.Color4f(0.0,1.0,0.0,1.0), new Util.Color4f(1.0,0.0,0.0,1.0));

//	debug.addDebugLineCol2(new Geometry.Vec3(0, 0, 0), new Geometry.Vec3(1, 0, 0), new Util.Color4f(1, 0, 0, 1), new Util.Color4f(1, 0, 0, 1));
//	debug.addDebugLineCol2(new Geometry.Vec3(0, 0, 0), new Geometry.Vec3(0, 1, 0), new Util.Color4f(0, 1, 0, 1), new Util.Color4f(0, 1, 0, 1));
//	debug.addDebugLineCol2(new Geometry.Vec3(0, 0, 0), new Geometry.Vec3(0, 0, 1), new Util.Color4f(0, 0, 1, 1), new Util.Color4f(0, 0, 1, 1));
}

void LightNodeManager::buildVoxelOctree(Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, unsigned int atomicCounterOffset, Rendering::Texture* octreeLocks, Util::Reference<MinSG::CameraNodeOrtho> enclosingCameras[3], unsigned int maxOctreeDepth, MinSG::Node* renderNode, bool drawStatic, bool drawDynamic, bool useStartDimensions){
	renderingContext->pushAndSetBoundImage(0, Rendering::ImageBindParameters(octreeTexture));
	fillTexture(atomicCounter, 0);
	fillTexture(octreeLocks, 0);
	renderingContext->pushAndSetBoundImage(1, Rendering::ImageBindParameters(octreeLocks));
#ifdef USE_ATOMIC_COUNTER
	renderingContext->pushAndSetAtomicCounterTextureBuffer(0, atomicCounter);
#else
	renderingContext->pushAndSetBoundImage(2, Rendering::ImageBindParameters(atomicCounter));
#endif // USE_ATOMIC_COUNTER

	if(useStartDimensions){
		voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("rootMidPos", lightingArea.center));
		voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("quarterSizeOfRootNode", lightingArea.extend * 0.25f));
	} else {
		voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("rootMidPos", renderNode->getWorldBB().getCenter()));
		voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("quarterSizeOfRootNode", renderNode->getWorldBB().getExtentMax() * 0.25f));
	}
	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("atomicCounterOffset", (int32_t)atomicCounterOffset));
	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("textureWidth", (int32_t)octreeTexture->getWidth()));
	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("lockTextureWidth", (int32_t)octreeLocks->getWidth()));
	voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("maxTreeDepth", (int32_t)maxOctreeDepth));

//	std::cout << "Pushing camera" << std::endl;

	//activate new camera
	frameContext->pushCamera();
	//activate new culling
	renderingContext->pushCullFace();
	renderingContext->pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::LEQUAL));
	for(unsigned int i = 0; i < 3; i++){
		frameContext->setCamera(enclosingCameras[i].get());
//		voxelOctreeShaderCreate.get()->setUniform(*renderingContext, Rendering::Uniform("inverseMat", enclosingCameras[i]->getMatrix().inverse()));

		//write into a texture
		TextureProcessor textureProcessor;
		textureProcessor.setRenderingContext(renderingContext);
		textureProcessor.setOutputTexture(tmpTexVoxelOctreeSize.get());
		textureProcessor.setDepthTexture(tmpDepthTexSmallest.get());
		textureProcessor.setShader(voxelOctreeShaderCreate.get());

//		std::cout << "before rendering " << i << std::endl;

		textureProcessor.begin();

//		renderingContext->clearDepth(1.0f);
//
//		renderingContext->clearScreen(Util::Color4f(1, 1, 1, 1));

//		std::cout << "after clear screen " << i << std::endl;

		renderingContext->setCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters()));

//		std::cout << "after cull face " << i << std::endl;

//		renderAllNodes(renderNode);
		renderAllNodes(drawStatic, drawDynamic);
//		renderNodeSingleCall(renderNode);

//		renderingContext->setCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters::CULL_BACK));
//		renderAllNodes(lightRootNode.get());
//
//		std::cout << "between rendering " << i << std::endl;
//
//		renderingContext->setCullFace(Rendering::CullFaceParameters(Rendering::CullFaceParameters::CULL_FRONT));
//		renderAllNodes(lightRootNode.get());

		textureProcessor.end();

//		std::cout << "after rendering " << i << std::endl;
	}

#ifdef TP_ACTIVATE_DEBUG
	//DEBUG output of the unneeded data as png
//	tmpTexVoxelOctreeSize.get()->downloadGLTexture(*renderingContext);
//	Rendering::Serialization::saveTexture(*renderingContext, tmpTexVoxelOctreeSize.get(), Util::FileName("screens/voxelOctreeTex.png"));
////	Rendering::TextureUtils::drawTextureToScreen(*renderingContext, Geometry::Rect_i(0, 0, textureSize, textureSize), *tmpTexVoxelOctreeSize.get(), Geometry::Rect_f(0.0f, 0.0f, 1.0f, 1.0f));
	//DEBUG END
#endif //TP_ACTIVATE_DEBUG

//	Util::Reference<Util::PixelAccessor> acc2 = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *octreeTexture);
//	std::cout << "Number Nodes: " << acc2->getWidth() * acc2->getHeight() / VOXEL_OCTREE_SIZE_PER_NODE << std::endl;
//	for(unsigned int i = 0; i < acc2->getWidth() * acc2->getHeight() / VOXEL_OCTREE_SIZE_PER_NODE; i++){
//		unsigned int x = i % acc2->getWidth();
//		unsigned int y = i / acc2->getWidth();
//		acc2->writeColor(x, y, Util::Color4f(i + 1, 0, 0, 0));
//	}

//	std::cout << "reset values" << std::endl;

	//reset depth test
	renderingContext->popDepthBuffer();
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

void LightNodeManager::renderNodeSingleCall(MinSG::Node* node){
	MinSG::RenderParam renderParam;
	renderParam.setFlags(MinSG::USE_WORLD_MATRIX | MinSG::FRUSTUM_CULLING);
	frameContext->displayNode(node, renderParam);
}

void LightNodeManager::renderAllNodes(MinSG::Node* node){
	NodeRenderVisitor renderVisitor;
	renderVisitor.init(frameContext);
	node->traverse(renderVisitor);
}

void LightNodeManager::renderAllNodes(bool staticNodes, bool dynamicNodes){
	MinSG::RenderParam renderParam;
	renderParam.setFlags(MinSG::USE_WORLD_MATRIX | MinSG::FRUSTUM_CULLING);
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
		if((lightNodeMaps[i]->staticNode && staticNodes) || (!lightNodeMaps[i]->staticNode && dynamicNodes)){
//			lightNodeMaps[i]->geometryNode->display(*frameContext, renderParam);
			frameContext->displayNode(lightNodeMaps[i]->geometryNode, renderParam);
		}
	}
}

void LightNodeManager::removeAllStaticLightMapConnections(){
	for(unsigned int i = 0; i < lightNodeLightMaps.size(); i++){
		for(unsigned int j = 0; j < lightNodeLightMaps[i]->staticEdges.size(); j++){
			delete lightNodeLightMaps[i]->staticEdges[j];
		}
		lightNodeLightMaps[i]->staticEdges.clear();
	}
}

void LightNodeManager::removeAllDynamicLightMapConnections(){
	for(unsigned int i = 0; i < lightNodeLightMaps.size(); i++){
		for(unsigned int j = 0; j < lightNodeLightMaps[i]->dynamicEdges.size(); j++){
			delete lightNodeLightMaps[i]->dynamicEdges[j];
		}
		lightNodeLightMaps[i]->dynamicEdges.clear();
	}
}

void LightNodeManager::removeStaticLightNodeMapConnection(LightNodeMap* lightNodeMap, LightNodeMapConnection* lightNodeMapConnection){
	for(unsigned int i = 0; i < lightNodeMap->externalLightEdgesStatic.size(); i++){
		if(lightNodeMap->externalLightEdgesStatic[i] == lightNodeMapConnection){
			lightNodeMap->externalLightEdgesStatic.erase(lightNodeMap->externalLightEdgesStatic.begin()+i);
			break;
		}
	}
}

void LightNodeManager::removeAllStaticMapConnections(){
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
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
		lightNodeMaps[i]->externalLightEdgesStatic.clear();
	}
}

void LightNodeManager::removeAllDynamicMapConnections(){
	for(unsigned int i = 0; i < lightNodeMaps.size(); i++){
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
		lightNodeMaps[i]->externalLightEdgesDynamic.clear();
	}
}

}
}

#endif /* MINSG_EXT_THESISPETER */
