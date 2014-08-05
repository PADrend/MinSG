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
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/LightNode.h>
#include <Util/Graphics/PixelAccessor.h>
#include <MinSG/Core/FrameContext.h>
#include <random>

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

NodeCreaterVisitor::NodeCreaterVisitor(){
	nodeIndex = 0;
}

NodeCreaterVisitor::status NodeCreaterVisitor::leave(MinSG::Node* node){
	if(typeid(node) == typeid(MinSG::GeometryNode)){
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
	} else if(typeid(node) == typeid(MinSG::LightNode)){
		//TODO: implement light nodes
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
	if(typeid(node) == typeid(MinSG::GeometryNode)){
		frameContext->displayNode(node, renderParam);
	}

	return CONTINUE_TRAVERSAL;
}

LightNodeManager::LightNodeManager(){
	for(unsigned int i = 0; i < 3; i++) sceneEnclosingCameras[i] = 0;
	debug = 0;
}

LightNodeManager::~LightNodeManager(){
	cleanUp();
}

void LightNodeManager::setSceneRootNode(MinSG::Node *sceneRootNode){
	this->sceneRootNode = sceneRootNode;
	debug->setSceneRootNode(sceneRootNode);
}

void LightNodeManager::setRenderingContext(Rendering::RenderingContext& renderingContext){
	this->renderingContext = &renderingContext;
}

void LightNodeManager::setFrameContext(MinSG::FrameContext& frameContext){
	this->frameContext = &frameContext;
}

//here begins the fun
void LightNodeManager::activateLighting(MinSG::Node *sceneRootNode, MinSG::Node* lightRootNode, Rendering::RenderingContext& renderingContext, MinSG::FrameContext& frameContext){
	if(debug == 0) debug = new DebugObjects();
	else debug->clearDebug();
	//some needed variables (for rendering etc.)
    setSceneRootNode(sceneRootNode);
    setLightRootNode(lightRootNode);
    setRenderingContext(renderingContext);
    setFrameContext(frameContext);

	//create the
    createWorldBBCameras();

//	createLightNodes(lightRootNode);

	debug->buildDebugLineNode();
}

void LightNodeManager::createLightNodes(){
	NodeCreaterVisitor createNodes;
	createNodes.nodeIndex = 0;
    createNodes.lightNodeMaps = &lightNodeMaps;
    lightRootNode->traverse(createNodes);
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

void LightNodeManager::cleanUp(){
	for(unsigned int i = 0; i < 3; i++){
		if(sceneEnclosingCameras[i] != 0) delete sceneEnclosingCameras[i];
	}
	if(debug != 0){
		delete debug;
		debug = 0;
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

	//lookup shader
	Rendering::Shader* shader = Rendering::Shader::loadShader(Util::FileName("./extPlugins/ThesisPeter/voxelOctreeRead.vs"), Util::FileName("./extPlugins/ThesisPeter/voxelOctreeRead.fs"), Rendering::Shader::USE_UNIFORMS | Rendering::Shader::USE_GL);

	//activate the shader and check the edges
	TextureProcessor textureProcessor;
	textureProcessor.setRenderingContext(this->renderingContext);
    textureProcessor.setInputTexture(edgeInput.get());
    textureProcessor.setOutputTexture(edgeOutput.get());
	textureProcessor.setShader(shader);
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
//	acc.get()->fill(0, 0, texture->getWidth(), texture->getHeight(), color);
}

void LightNodeManager::fillTexture(Rendering::Texture *texture, uint8_t value){
	Util::Reference<Util::PixelAccessor> acc = Rendering::TextureUtils::createColorPixelAccessor(*renderingContext, *texture);
	for(unsigned int y = 0; y < texture->getHeight(); y++){
		for(unsigned int x = 0; x < texture->getWidth(); x++){
			acc.get()->writeColor(x, y, value);
		}
	}
}

void LightNodeManager::createWorldBBCameras(){
	const Geometry::Vec3 worldDirections[3] = {Geometry::Vec3(1, 0, 0), Geometry::Vec3(0, 1, 0), Geometry::Vec3(0, 0, 1)};
	float maxExtend = lightRootNode->getWorldBB().getExtentMax();
	float maxExtendHalf = maxExtend * 0.5f;
//	float diameterHalf = lightRootNode->getWorldBB().getDiameter() * 0.5;
	lightRootCenter = lightRootNode->getWorldBB().getCenter();

	for(unsigned int i = 0; i < 3; i++){
		if(sceneEnclosingCameras[i] == 0) sceneEnclosingCameras[i] = new MinSG::CameraNodeOrtho();
		sceneEnclosingCameras[i]->setWorldPosition(lightRootCenter - worldDirections[i] * (maxExtendHalf + 1));
//		camera.setWorldPosition(rootCenter - worldDir * (diameterHalf + 1));
//		camera.setWorldPosition(rootCenter - (worldDir.getNormalized()) * (maxExtend + 1));
		sceneEnclosingCameras[i]->rotateToWorldDir(worldDirections[i]);

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

		sceneEnclosingCameras[i]->setNearFar(-1, -maxExtend - 1);
//		camera.setNearFar(maxExtend + 1, 1);
//		camera.setNearFar(1, diameterHalf*2 + 1);
		sceneEnclosingCameras[i]->setClippingPlanes(-maxExtendHalf, maxExtendHalf, -maxExtendHalf, maxExtendHalf);
		cl = -maxExtendHalf;
		cr = maxExtendHalf;
		ct = maxExtendHalf;
		cb = -maxExtendHalf;

		Geometry::Vec3 debPos = sceneEnclosingCameras[i]->getWorldPosition();
		debug->addDebugLine(debPos + offset + right * cl + up * cb, debPos + offset + right * cr + up * cb);
		debug->addDebugLine(debPos + offset + right * cl + up * cb, debPos + offset + right * cl + up * ct);
		debug->addDebugLine(debPos + offset + right * cr + up * cb, debPos + offset + right * cr + up * ct);
		debug->addDebugLine(debPos + offset + right * cl + up * ct, debPos + offset + right * cr + up * ct);
		debug->addDebugLine(debPos + offsetFar + right * cl + up * cb, debPos + offsetFar + right * cr + up * cb);
		debug->addDebugLine(debPos + offsetFar + right * cl + up * cb, debPos + offsetFar + right * cl + up * ct);
		debug->addDebugLine(debPos + offsetFar + right * cr + up * cb, debPos + offsetFar + right * cr + up * ct);
		debug->addDebugLine(debPos + offsetFar + right * cl + up * ct, debPos + offsetFar + right * cr + up * ct);
//		outln("left: "+cl+" right: "+cr+" top: "+ct+" bottom: "+cb);

		debug->addDebugLine(sceneEnclosingCameras[i]->getWorldPosition(), sceneEnclosingCameras[i]->getWorldPosition() + offset, Util::Color4f(0.0,0.0,1.0,1.0), Util::Color4f(1.0,0.0,0.0,1.0));
		debug->addDebugLine(sceneEnclosingCameras[i]->getWorldPosition() + offset, sceneEnclosingCameras[i]->getWorldPosition() + offsetFar, Util::Color4f(0.0,1.0,0.0,1.0), Util::Color4f(1.0,0.0,0.0,1.0));
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

	renderingContext->pushAndSetBoundImage(0, Rendering::ImageBindParameters(octreeTexture));
	renderingContext->pushAndSetBoundImage(1, Rendering::ImageBindParameters(atomicCounter));

	voxelOctreeShaderCreate->setUniform(*renderingContext, Rendering::Uniform("sizeOfRootNode", lightRootNode->getWorldBB().getExtentMax() * 0.25f));
	voxelOctreeShaderCreate->setUniform(*renderingContext, Rendering::Uniform("textureWidth", (int32_t)VOXEL_OCTREE_TEXTURE_SIZE));

	frameContext->pushCamera();
	for(unsigned int i = 0; i < 3; i++){
		frameContext->setCamera(sceneEnclosingCameras[i]);
//		frameContext.setCamera(PADrend.getActiveCamera());

		//write into a texture
		TextureProcessor textureProcessor;
		textureProcessor.setOutputTexture(outputTex.get());
		textureProcessor.setShader(voxelOctreeShaderCreate);
		textureProcessor.begin();
		renderingContext->clearScreen(Util::Color4f(1, 1, 1, 1));

		renderAllNodes(lightRootNode);

		textureProcessor.end();
	}

//	Rendering::showDebugTexture(outputTex);

	//reset camera
	frameContext->popCamera();
}

void LightNodeManager::renderAllNodes(MinSG::Node* node){
	NodeRenderVisitor renderVisitor;
	renderVisitor.init(frameContext);
	node->traverse(renderVisitor);
}

}
}

#endif /* MINSG_EXT_THESISPETER */
