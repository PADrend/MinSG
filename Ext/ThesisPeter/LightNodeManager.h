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
#include <MinSG/Core/NodeVisitor.h>
#include <MinSG/Core/NodeAttributeModifier.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Util/Graphics/ColorLibrary.h>

namespace Rendering {
class Mesh;

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

struct LightNode {
	Geometry::Vec3 position;
	Geometry::Vec3 normal;
	Util::Color4f color;
};

struct LightNodeMap {
	std::vector<LightNode*> lightNodes;
	MinSG::GeometryNode* geometryNode;
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

class LightNodeManager {
	PROVIDES_TYPE_NAME(LightNodeManager)
public:
	LightNodeManager();
	void createLightNodes(MinSG::Node *rootNode);
	static void createLightNodes(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);
	static void mapLightNodesToObject(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);
	static void addAttribute(Rendering::Mesh *mesh);
//	static void createLightEdges(lightNodes);

private:
	void setLightRootNode(MinSG::Node *rootNode);
	static void createLightNodesPerVertexRandom(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes, float randomVal);
	static void mapLightNodesToObjectClosest(MinSG::GeometryNode* node, std::vector<LightNode*>* lightNodes);

	MinSG::Node* lightRootNode;
	std::vector<LightNodeMap*> lightNodeMaps;

	static const Util::StringIdentifier lightNodeIDIdent;
};

}
}

#endif /* MINSG_THESISPETER_LIGHTNODEMANAGER_H_ */

#endif /* MINSG_EXT_THESISPETER */
