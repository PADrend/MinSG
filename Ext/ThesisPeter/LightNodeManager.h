/*
	This file is part of the MinSG library extension ThesisPeter.
	Copyright (C) 2014 Peter Stilow

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#define MINSG_EXT_THESISPETER
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
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/PixelAccessor.h>
#include "DebugObjects.h"
#include "TextureProcessor.h"

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
			const unsigned int* v = _ptr<const unsigned int>(index);
			return *v;
		}

		void setLightNodeIndex(uint32_t index, const unsigned int value){
			assertRange(index);
			unsigned int* v = _ptr<unsigned int>(index);
			*v = value;
		}
};
}

namespace MinSG {
class Node;
class GeometryNode;

namespace ThesisPeter {

struct FilterEdgeState {
	int childID;
	int nodeOffset;
	Geometry::Vec3 t0, t1, tm;
};

struct LightNode {
	Geometry::Vec3 position;
	Geometry::Vec3 normal;
	Util::Color4f color;
};

struct LightEdge {
	LightNode* source;
	LightNode* target;
	float weight;
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
	void setSceneRootNode(Util::Reference<MinSG::Node> sceneRootNode);
	void setRenderingContext(Rendering::RenderingContext& renderingContext);
	void setFrameContext(MinSG::FrameContext& frameContext);
	unsigned int addTreeToDebug(Geometry::Vec3 parentPos, float parentSize, unsigned int depth, unsigned int curID, Util::PixelAccessor* pixelAccessor);
	void activateLighting(Util::Reference<MinSG::Node> sceneRootNode, Util::Reference<MinSG::Node> lightRootNode, Rendering::RenderingContext& renderingContext, MinSG::FrameContext& frameContext);
	void createLightNodes();
	static void createLightNodes(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);
	static void mapLightNodesToObject(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);
	void createLightEdges(Rendering::Texture* atomicCounter);
	void cleanUpDebug();
	void cleanUp();

	static const float MAX_EDGE_LENGTH;
	static const unsigned int VOXEL_OCTREE_DEPTH;			//more depth = more precision = more memory usage
	static const unsigned int VOXEL_OCTREE_TEXTURE_SIZE;	//max = 16384;	size*size = 18874368 if tree completely filled with depth 7
	static const unsigned int VOXEL_OCTREE_SIZE_PER_NODE;	//must be set to the same value as the shader definition!!!

private:
	static unsigned int nextPowOf2(unsigned int number);
	static void getTexCoords(unsigned int index, unsigned int texWidth, Geometry::Vec2i* texCoords);
	void setLightRootNode(Util::Reference<MinSG::Node> rootNode);
	static void createLightNodesPerVertexPercent(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes, float percentage);
	static void createLightNodesPerVertexRandom(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes, float randomVal);
	static void mapLightNodesToObjectClosest(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);
	static bool isVisible(LightNode* source, LightNode* target);
	void addLightEdge(LightNode* source, LightNode* target, std::vector<LightEdge*>* lightEdges);

	void filterIncorrectEdges(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter);
	void filterIncorrectEdgesAsTexture(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter);

	void filterIncorrectEdgesAsTextureCPU(std::vector<LightEdge*> *edges, Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter);
	int checkLine(Geometry::Vec3 octreeMin, Geometry::Vec3 octreeMax, Geometry::Vec3 rayOrigin, Geometry::Vec3 rayDirection, Util::Reference<Util::PixelAccessor> octreeAcc);
	int testIntersection(Geometry::Vec3 t0, Geometry::Vec3 t1, Geometry::Vec3 lineStart, Geometry::Vec3 origLineStart, Geometry::Vec3 lineDir, Util::Reference<Util::PixelAccessor> octreeAcc);
	void firstNodeRoot(Geometry::Vec3 t0, Geometry::Vec3 t1, Geometry::Vec3 lineStart, FilterEdgeState* nodeStates, int* curDepth, Util::Reference<Util::PixelAccessor> octreeAcc);
	int nextNode(Geometry::Vec3 tm, int next1, int next2, int next3);
	int firstNode(Geometry::Vec3 t0, Geometry::Vec3 tm);
	void setStartEndNodes(Geometry::Vec3 posStart, Geometry::Vec3 posEnd, Util::Reference<Util::PixelAccessor> octreeAcc);
	int getNodeID(Geometry::Vec3 pos, Util::Reference<Util::PixelAccessor> octreeAcc);
	void getNodeIndexVoxelOctree(int nodeID, Geometry::Vec2i* index);
	void getNewMidPos(Geometry::Vec3 oldMidPos, int childOffset, int curDepth, Geometry::Vec3* newMidPos);
	int getChildOffset(Geometry::Vec3 midPos, Geometry::Vec3 targetPos);
	int startNodeID, endNodeID;
	int octreeLookupDifference;
	float quarterSizeOfRootNode;
	unsigned int voxelOctreeTextureSize;
	Geometry::Vec3 rootMidPos;

	void fillTexture(Rendering::Texture *texture, Util::Color4f color);
	void fillTexture(Rendering::Texture *texture, uint8_t value);
	void fillTextureFloat(Rendering::Texture *texture, float value);
	void createWorldBBCameras();
	void buildVoxelOctree(Rendering::Texture* octreeTexture, Rendering::Texture* atomicCounter, Rendering::Texture* octreeLocks);
	void renderAllNodes(MinSG::Node* node);

	Util::Reference<MinSG::Node> lightRootNode;
	Util::Reference<MinSG::Node> sceneRootNode;
	Rendering::RenderingContext* renderingContext;
	MinSG::FrameContext* frameContext;
	std::vector<LightNodeMap*> lightNodeMaps;
	std::vector<LightEdge*> lightEdges;
	Util::Reference<MinSG::CameraNodeOrtho> sceneEnclosingCameras[3];
	Geometry::Vec3 lightRootCenter;
	Util::Reference<Rendering::Texture> voxelOctreeTextureStatic;
	Util::Reference<Rendering::Texture> voxelOctreeLocksStatic;
	Util::Reference<Rendering::Texture> atomicCounter;
	Util::Reference<Rendering::Shader> voxelOctreeShaderCreate;
	Util::Reference<Rendering::Shader> voxelOctreeShaderRead;

	static const Util::StringIdentifier lightNodeIDIdent;
	DebugObjects* debug;
};

}
}

#endif /* MINSG_THESISPETER_LIGHTNODEMANAGER_H_ */

#endif /* MINSG_EXT_THESISPETER */
