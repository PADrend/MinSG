/*
	This file is part of the MinSG library extension ThesisPeter.
	Copyright (C) 2014 Peter Stilow

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_THESISPETER

#ifndef MINSG_THESISPETER_LIGHTNODEMANAGER_H_
#define MINSG_THESISPETER_LIGHTNODEMANAGER_H_

#include <vector>
#include <Geometry/Vec3.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Core/Nodes/CameraNodeOrtho.h>
#include <MinSG/Core/NodeVisitor.h>
#include <MinSG/Core/NodeAttributeModifier.h>
#include <MinSG/Core/FrameContext.h>
#include <MinSG/Core/RenderParam.h>
#include <MinSG/Core/FrameContext.h>
#include <MinSG/Core/States/ShaderState.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Timer.h>
#include "DebugObjects.h"
#include "TextureProcessor.h"
#include "ImageState.h"

namespace Rendering {
class Mesh;
class Texture;

/*! LightNodeIndexAttributeAccessor ---|> VertexAttributeAccessor
	Abstract accessor for light node indices. */
class LightNodeIndexAttributeAccessor : public VertexAttributeAccessor {
protected:
	LightNodeIndexAttributeAccessor(MeshVertexData & _vData,const VertexAttribute & _attribute) :
			VertexAttributeAccessor(_vData,_attribute){}

	static const std::string noAttrErrorMsg;
	static const std::string unimplementedFormatMsg;

	static const VertexAttribute & assertAttribute(MeshVertexData & _vData, const Util::StringIdentifier name) {
		const VertexAttribute & attr = _vData.getVertexDescription().getAttribute(name);
		if(attr.empty())
			throw std::invalid_argument(noAttrErrorMsg + name.toString() + '\'');
		return attr;
	}
public:
	/*! (static factory)
		Create a LightNodeIndexAttributeAccessor for the given MeshVertexData's attribute having the given name.
		If no Accessor can be created, an std::invalid_argument exception is thrown. */
	static Util::Reference<LightNodeIndexAttributeAccessor> create(MeshVertexData & _vData,Util::StringIdentifier name){
		const VertexAttribute & attr = assertAttribute(_vData, name);
		if(attr.getNumValues() == 1 /*&& attr.getDataType() == GL_FLOAT*/) {
			return new LightNodeIndexAttributeAccessor(_vData, attr);
		} else {
			throw std::invalid_argument(unimplementedFormatMsg + name.toString() + '\'');
		}
	}

	virtual ~LightNodeIndexAttributeAccessor(){}

	const unsigned int getLightNodeIndex(uint32_t index) const {
		assertRange(index);
		const float* v = _ptr<const float>(index);
		return *v;
	}

	void setLightNodeIndex(uint32_t index, const float value){
		assertRange(index);
		float* v = _ptr<float>(index);
		*v = value;
	}
};
}

namespace MinSG {
class Node;
class GeometryNode;
class LightNode;
class PathNode;

namespace ThesisPeter {

struct FilterEdgeState {
	int childID;
	int nodeOffset;
	Geometry::Vec3 t0, t1, tm;
};

struct LightNode {
	Geometry::Vec3 osPosition;	//object space position, used for dynamic objects for recalculation of the position
	Geometry::Vec3 position;
	Geometry::Vec3 osNormal;	//object space normal, used for dynamic objects for recalculation of the positions
	Geometry::Vec3 normal;
	Util::Color4f color;
	unsigned int id;			//used to identify the position inside the texture on GPU
};

struct LightEdge {
	LightNode* source;
	LightNode* target;
	Geometry::Vec3 weight;
};

struct LightNodeMap;

struct LightNodeMapConnection {
	std::vector<LightEdge*> edges;
	LightNodeMap* map1;
	LightNodeMap* map2;
};

struct LightNodeMap {
	std::vector<LightNode*> lightNodes;
	std::vector<LightEdge*> internalLightEdges;
	std::vector<LightNodeMapConnection*> externalLightEdgesStatic;
	std::vector<LightNodeMapConnection*> externalLightEdgesDynamic;
	MinSG::GeometryNode* geometryNode;
	bool staticNode;
};

struct LightNodeLightMap {
	LightNode light;
	MinSG::LightNode* lightNode;
	std::vector<LightEdge*> staticEdges;
	std::vector<LightEdge*> dynamicEdges;
	bool staticNode;
};

struct LightingArea {
	float extend;
	Geometry::Vec3 center;
};

class LightInfoAttribute : public Util::GenericAttribute {
public:
	unsigned int lightNodeID;
	bool staticNode;

	LightInfoAttribute* clone() const override {
		LightInfoAttribute* lightAttr = new LightInfoAttribute();
		lightAttr->lightNodeID = lightNodeID;
		lightAttr->staticNode = staticNode;
		return lightAttr;
	}
};

class NodeCreaterVisitor : public MinSG::NodeVisitor {
public:
	NodeCreaterVisitor();
	static unsigned int nodeIndex;
	std::vector<LightNodeMap*>* lightNodeMaps;
	std::vector<LightNodeLightMap*>* lightNodeLightMaps;
	std::vector<MinSG::PathNode*>* pathNodes;
	bool useGeometryNodes, useLightNodes, loadPaths;

private:
	static const Util::StringIdentifier staticNodeIdent;

	status leave(MinSG::Node* node);
};

class NodeRenderVisitor : public MinSG::NodeVisitor {
public:
	NodeRenderVisitor();

	void init(MinSG::FrameContext* frameContext);

private:
	status leave(MinSG::Node* node);

	MinSG::FrameContext* frameContext;
	MinSG::RenderParam renderParam;
};

class LightNodeManager {
	PROVIDES_TYPE_NAME(LightNodeManager)
public:
	LightNodeManager();
	virtual ~LightNodeManager();
	void test(MinSG::FrameContext& frameContext, Util::Reference<MinSG::Node> sceneRootNode);
	void startTesting();
	void stopTesting();
	void resumeTesting();
	void onRender();
	void setCameraNode(Util::Reference<MinSG::Node> cameraNode);
	void setSceneRootNode(Util::Reference<MinSG::Node> sceneRootNode);
	void setRenderingContext(Rendering::RenderingContext& renderingContext);
	void setFrameContext(MinSG::FrameContext& frameContext);
	unsigned int addTreeToDebug(Geometry::Vec3 parentPos, float parentSize, unsigned int depth, unsigned int curID, Util::PixelAccessor* pixelAccessor, unsigned int maxDepth);
	void activateLighting(Util::Reference<MinSG::Node> sceneRootNode, Util::Reference<MinSG::Node> lightRootNode, Rendering::RenderingContext& renderingContext, MinSG::FrameContext& frameContext, Util::Reference<MinSG::Node> cameraNode);
	void createLightNodes();
	static void createLightNodes(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);
	static void mapLightNodesToObject(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);
	void createLightEdges(Rendering::Texture* atomicCounter);
	void createLightEdgesStaticFromLights(Rendering::Texture* atomicCounter);
	void createLightEdgesDynamicFromLights(Rendering::Texture* atomicCounter);
	void createLightEdgesDynamicFromDynamicLights(Rendering::Texture* atomicCounter);
	void createLightEdgesInternal(Rendering::Texture* atomicCounter);
	void createLightEdgesExternalStatic(Rendering::Texture* atomicCounter);
	void createLightEdgesExternalDynamic(Rendering::Texture* atomicCounter);
	void cleanUpDebug();
	void cleanUp();
	void setShowEdges(bool showEdges);
	void setShowOctree(bool showOctree);

	static const unsigned int NUMBER_LIGHT_PROPAGATION_CYCLES;	//the number of light propagation cycles, which directly corresponds to the number of light "reflection" between objects
	static const float PERCENT_NODES;							//the percent value for how many nodes shall be taken, also taken for random algorithm
	static const float MAX_EDGE_LENGTH;							//maximal edge length between 2 nodes
	static const float MIN_EDGE_WEIGHT;							//minimal edge weight between 2 nodes
	static const float MAX_EDGE_LENGTH_LIGHT;					//maximal edge length between a node an a light node
	static const float MIN_EDGE_WEIGHT_LIGHT;					//minimal edge weight between a node an a light node
	static const unsigned int VOXEL_OCTREE_DEPTH;				//more depth = more precision = more memory usage
	static const unsigned int VOXEL_OCTREE_TEXTURE_SIZE;		//max = 16384;	size*size = 18874368 if tree completely filled with depth 7
	static const unsigned int VOXEL_OCTREE_SIZE_PER_NODE;		//must be set to the same value as the shader definition!!!
	static bool SHOW_EDGES;										//if active, edges are being shown
	static bool SHOW_OCTREE;									//if active, the octree is being shown
	static float LIGHT_STRENGTH;								//used for propagation of the light, start value for light sources
	static float LIGHT_STRENGTH_FACTOR;							//a factor, which is multiplied with the global illumination to "convert" it from HDR to LDR
	static float NODE_MAPPING_DISTANCE_FACTOR;					//a factor used for taking the normal of a surface into account, if the distance is not too big
	static float NODE_POSITION_OFFSET;							//an offset, that pushes the light nodes along their normal, to be more on the edge inside the voxelOctreeCell
	static const unsigned int VOXEL_OCTREE_TEXTURE_INTERNAL_SIZE;	//max = 16384;	size*size = 18874368 if tree completely filled with depth 7

	static unsigned int globalNodeCounter;						//used to give the nodes unique id's

	void setLightStrengthFactor(float lightStrenthFactor);

	//tracking objects in the scene
	void onNodeTransformed(Node* node);

	void addDynamicObject(Util::Reference<MinSG::Node> node);
	void addDynamicLight(Util::Reference<MinSG::Node> node);

private:
	static unsigned int nextPowOf2(unsigned int number);
	static void getTexCoords(unsigned int index, unsigned int texWidth, Geometry::Vec2i* texCoords);
	void setLightRootNode(Util::Reference<MinSG::Node> rootNode);
	static void refreshLightNodeParameters(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);
	static void refreshLightNodeLightParameters(MinSG::LightNode* node, LightNode* lightNode);
	static void createLightNodesPerVertexPercent(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes, float percentage);
	static void createLightNodesPerVertexRandom(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes, float randomVal);
	static void mapLightNodesToObjectClosest(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);
	static bool isVisible(LightNode* source, LightNode* target);
	void addLightEdge(LightNode* source, LightNode* target, std::vector<LightEdge*>* lightEdges, float maxEdgeLength, float minEdgeWeight, bool checkVisibility, bool useNormal);
	bool isDistanceLess(MinSG::Node* n1, MinSG::Node* n2, float distance);

	void filterIncorrectEdges(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, Util::Reference<MinSG::CameraNodeOrtho> enclosingCameras[3], unsigned int maxOctreeDepth, MinSG::Node* node, bool useStartDimensions = false);
	void filterIncorrectEdgesAsObjects(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Util::Reference<MinSG::CameraNodeOrtho> enclosingCameras[3], unsigned int maxOctreeDepth, MinSG::Node* node, bool useStartDimensions);

	void filterIncorrectEdgesAsTexture(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, MinSG::Node* node);
	void filterIncorrectEdgesAsTextureCPU(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, MinSG::Node* node);
	int checkLine(Geometry::Vec3 octreeMin, Geometry::Vec3 octreeMax, Geometry::Vec3 rayOrigin, Geometry::Vec3 rayDirection, Util::Reference<Util::PixelAccessor> octreeAcc);
	int testIntersection(Geometry::Vec3 t0, Geometry::Vec3 t1, Geometry::Vec3 lineStart, Geometry::Vec3 origLineStart, Geometry::Vec3 lineDir, Util::Reference<Util::PixelAccessor> octreeAcc);
	void firstNodeRoot(Geometry::Vec3 t0, Geometry::Vec3 t1, Geometry::Vec3 lineStart, FilterEdgeState* nodeStates, int* curDepth, Util::Reference<Util::PixelAccessor> octreeAcc);
	int nextNode(Geometry::Vec3 tm, int next1, int next2, int next3);
	int firstNode(Geometry::Vec3 t0, Geometry::Vec3 tm);
	void setStartEndNodes(Geometry::Vec3 posStart, Geometry::Vec3 posEnd, Util::Reference<Util::PixelAccessor> octreeAcc);
	int getNodeID(Geometry::Vec3 pos, Util::Reference<Util::PixelAccessor> octreeAcc);
	void getNewMidPos(Geometry::Vec3 oldMidPos, int childOffset, int curDepth, Geometry::Vec3* newMidPos);
	int getChildOffset(Geometry::Vec3 midPos, Geometry::Vec3 targetPos);
	void getNodeIndexVoxelOctree(int nodeID, Geometry::Vec2i* index);
	int startNodeID, endNodeID;
	int octreeLookupDifference;
	float quarterSizeOfRootNode;
	unsigned int voxelOctreeTextureSize;

	void createNodeTextures();
	void createEdgeTextures();
	void createEdgeTexturesInternal();
	void createEdgeTexturesExternalStatic();
	void createEdgeTexturesExternalDynamic();
	void propagateLight();

	void copyTexture(Rendering::Texture *source, Rendering::Texture *target);
	void fillTexture(Rendering::Texture *texture, Util::Color4f color);
	void fillTexture(Rendering::Texture *texture, uint8_t value);
	void fillTextureFloat(Rendering::Texture *texture, float value);
	void createWorldBBCameras(MinSG::Node* node, Util::Reference<MinSG::CameraNodeOrtho> enclosingCameras[3]);
	void buildVoxelOctree(Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, unsigned int atomicCounterOffset, Rendering::Texture* octreeLocks, Util::Reference<MinSG::CameraNodeOrtho> enclosingCameras[3], unsigned int maxOctreeDepth, MinSG::Node* renderNode, bool drawStatic, bool drawDynamic, bool useStartDimensions = false);
	void renderNodeSingleCall(MinSG::Node* node);
	void renderAllNodes(MinSG::Node* node);
	void renderAllNodes(bool staticNodes, bool dynamicNodes);

	void removeAllStaticLightMapConnections();
	void removeAllDynamicLightMapConnections();
	void removeStaticLightNodeMapConnection(LightNodeMap* lightNodeMap, LightNodeMapConnection* lightNodeMapConnection);
	void removeAllStaticMapConnections();
	void removeAllDynamicMapConnections();

	void objectSwitchFromStaticToDynamic();
	void objectSwitchFromDynamicToDynamic();
	void lightSwitchFromStaticToDynamic();
	void lightSwitchFromDynamicToDynamic();
	void refreshDebugging();

	Rendering::RenderingContext* renderingContext;
	MinSG::FrameContext* frameContext;
	std::vector<LightNodeLightMap*> lightNodeLightMaps;
	std::vector<LightNodeMap*> lightNodeMaps;
	std::vector<LightEdge*> lightEdges;
	Util::Reference<MinSG::CameraNodeOrtho> sceneEnclosingCameras[3];
	Util::Reference<Rendering::Texture> tmpTexVoxelOctreeSize;			//used to "render" the octree (at creation) and the edges into the octree (for collision)
	Util::Reference<Rendering::Texture> voxelOctreeTextureStatic;
	unsigned int numberStaticOctreeNodes;								//the number of voxelOctreeNodes of the static texture, to assign the atomic counter
	Util::Reference<Rendering::Texture> voxelOctreeLocks;
	Util::Reference<Rendering::Texture> atomicCounter;
	Util::Reference<Rendering::Texture> voxelOctreeTextureComplete;

	static unsigned int NODE_DATA_LENGTH_PER_EDGE;
	static unsigned int WEIGHT_DATA_LENGTH_PER_EDGE;
	unsigned int curNumEdgesInternal;
	unsigned int curNumEdgesExternalStatic;
	unsigned int curNumEdgesExternalDynamic;
	unsigned int curNodeTextureRenderingIndex;
	Util::Reference<Rendering::Texture> tmpTexSmallest;
	Util::Reference<Rendering::Texture> tmpDepthTexSmallest;
	Util::Reference<Rendering::Texture> tmpTexEdgeSizeInternal;
	Util::Reference<Rendering::Texture> edgeTextureNodesInternal;
	Util::Reference<Rendering::Texture> edgeTextureWeightsInternal;
	Util::Reference<Rendering::Texture> tmpTexEdgeSizeExternalStatic;
	Util::Reference<Rendering::Texture> edgeTextureNodesExternalStatic;
	Util::Reference<Rendering::Texture> edgeTextureWeightsExternalStatic;
	Util::Reference<Rendering::Texture> tmpTexEdgeSizeExternalDynamic;
	Util::Reference<Rendering::Texture> edgeTextureNodesExternalDynamic;
	Util::Reference<Rendering::Texture> edgeTextureWeightsExternalDynamic;
	Util::Reference<Rendering::Texture> nodeTextureStatic;
	Util::Reference<Rendering::Texture> nodeTextureRendering[2];

	Util::Reference<Rendering::Shader> voxelOctreeShaderCreate;
	Util::Reference<Rendering::Shader> voxelOctreeShaderReadTexture;
	Util::Reference<Rendering::Shader> voxelOctreeShaderReadObject;
	Util::Reference<Rendering::Shader> propagateLightShader;

	Util::Reference<ImageState> lightGraphTextureState;
	Util::Reference<ShaderState> lightGraphShader;
	bool graphShaderAssigned;			//tracks, if the lightGraphShader was already assigned, or not

	LightingArea lightingArea;			//The area, in which the objects are placed for lighting (calculated from the bounding boxes of the objects)
	Util::Reference<MinSG::Node> lightRootNode;
	Util::Reference<MinSG::Node> sceneRootNode;
	Util::Reference<MinSG::Node> cameraNode;

	static const Util::StringIdentifier lightNodeIDIdent;
	static DebugObjects debug;

	float globalMaxEdgeWeight, globalMinEdgeWeight;		//just for debugging

	//for the rendering loop
	static bool dynamicObjectChanged;
	static bool staticObjectChanged;
	static bool dynamicLightChanged;
	static bool staticLightChanged;

	static bool loadPaths;
	std::vector<MinSG::PathNode*> pathNodes;
	std::vector<MinSG::Node*> dynamicObjects;
	std::vector<MinSG::Node*> dynamicLights;
	unsigned int frameCounter;
	double timePoint;
	bool testActive;

	Util::Timer timer;
	double lastTime, tmpTime;
	double minTime, maxTime, sumTime;
	unsigned int dynamicEdgesMin, dynamicEdgesMax, dynamicEdgesFilteredMin, dynamicEdgesFilteredMax;

	std::ofstream* logFile, *logFile2, *logFile3;
};

}
}

#endif /* MINSG_THESISPETER_LIGHTNODEMANAGER_H_ */

#endif /* MINSG_EXT_THESISPETER */
