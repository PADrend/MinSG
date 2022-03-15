/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StdNodeVisitors.h"
#include "../Core/Nodes/GeometryNode.h"
#include "../Core/Nodes/GroupNode.h"
#include "../Core/FrameContext.h"
#include <Geometry/Box.h>
#include <Geometry/BoxIntersection.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/RayBoxIntersection.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/OcclusionQuery.h>
#include <Util/Graphics/Color.h>
#include <Util/Macros.h>

#include <limits>

namespace MinSG {

// ----------------------------------------------------------------------------------------------------
template<>
void forEachNodeTopDown<Node>(Node * root, const std::function<void (Node *)>& func) { // Specialization for Node
	if(root == nullptr) {
		return;
	}
	struct Visitor : public NodeVisitor {
		const std::function<void (Node *)>& m_func;
		Visitor(const std::function<void (Node *)> & p_func) : m_func(p_func) {
		}
		virtual ~Visitor() = default;
		NodeVisitor::status enter(Node * node) override {
			// Use enter() to perform a pre-order tree walk
			m_func(node);
			return CONTINUE_TRAVERSAL;
		}
	} visitor(func);
	root->traverse(visitor);
}
template<>
void traverseTopDown<Node>(Node * root, std::function<NodeVisitor::status (Node *)> func) {
	if(root) {
		struct Visitor : public NodeVisitor {
			std::function<NodeVisitor::status (Node *)> m_func;
			Visitor(std::function<NodeVisitor::status (Node *)> p_func) : m_func(std::move(p_func)) {}
			virtual ~Visitor() = default;
		
			NodeVisitor::status enter(Node * node) override {	// Use enter() to perform a pre-order tree walk
				return m_func(node);
			}
		} visitor(func);
		root->traverse(visitor);
	}
}


// ----------------------------------------------------------------------------------------------------

std::deque<Node *> collectClosedNodes(Node * root) {
	std::deque<Node *> nodes;
	
	traverseTopDown(root,[&nodes](Node* node){
		if(node->isClosed()) {
			nodes.push_back(node);
			return NodeVisitor::BREAK_TRAVERSAL;
		}
		return NodeVisitor::CONTINUE_TRAVERSAL;
	});
	return nodes;
}

std::deque<Node *> collectClosedNodesAtPosition(Node * root, const Geometry::Vec3 & position) {
	std::deque<Node *> nodes;
	traverseTopDown(root,[&nodes,&position](Node* node) {
		if(!node->getWorldBB().contains(position)) {
			return NodeVisitor::BREAK_TRAVERSAL;
		} else if(!node->isClosed()) {
			return NodeVisitor::CONTINUE_TRAVERSAL;
		}
		const Geometry::Matrix4x4 iTransMat = node->getWorldTransformationMatrix().inverse();
		if(node->getBB().contains(iTransMat.transformPosition(position))) {
			nodes.push_back(node);
		}
		return NodeVisitor::CONTINUE_TRAVERSAL;
	});
	return nodes;
}

std::deque<Node *> collectClosedNodesIntersectingBox(Node * root, const Geometry::Box & box) {
	std::deque<Node *> nodes;
	struct Vis : public NodeVisitor {
		Vis(const Geometry::Box & _b, std::deque<Node *> & _nodes) :
			box(_b), nodes(_nodes) {
		}
		virtual ~Vis() {
		}

		const Geometry::Box & box;
		std::deque<Node *> & nodes;

		// ---|> NodeVisitor
		NodeVisitor::status enter(Node * node) override {
			if(!Geometry::Intersection::isBoxIntersectingBox(node->getWorldBB(), box)) {
				return BREAK_TRAVERSAL;
			} else if(!node->isClosed()) {
				return CONTINUE_TRAVERSAL;
			}

			nodes.push_back(node);
			return CONTINUE_TRAVERSAL;
		}
	} visitor(box, nodes);
	root->traverse(visitor);
	return nodes;
}

std::deque<GeometryNode *> collectGeoNodesAtPosition(Node * root, const Geometry::Vec3 & position) {
	std::deque<GeometryNode *> geoNodes;
	struct Vis : public NodeVisitor {
		Vis(const Geometry::Vec3 & _p, std::deque<GeometryNode *> & _geoNodes) :
			position(_p), geoNodes(_geoNodes) {
		}
		virtual ~Vis() {
		}

		const Geometry::Vec3 & position;
		std::deque<GeometryNode *> & geoNodes;

		// ---|> NodeVisitor
		NodeVisitor::status enter(Node * node) override {
			if(!node->getWorldBB().contains(position)) {
				return NodeVisitor::BREAK_TRAVERSAL;
			}
			GeometryNode * geoNode = dynamic_cast<GeometryNode *>(node);
			if(geoNode == nullptr) {
				return CONTINUE_TRAVERSAL;
			}
			Geometry::Matrix4x4 iTransMat = geoNode->getWorldTransformationMatrix().inverse();
			if(geoNode->getBB().contains(iTransMat.transformPosition(position))) {
				geoNodes.push_back(geoNode);
			}
			return CONTINUE_TRAVERSAL;
		}
	} visitor(position, geoNodes);
	root->traverse(visitor);
	return geoNodes;
}

std::deque<GeometryNode *> collectGeoNodesIntersectingBox(Node * root, const Geometry::Box & box) {
	std::deque<GeometryNode *> geoNodes;
	struct Vis : public NodeVisitor {
		Vis(const Geometry::Box & _b, std::deque<GeometryNode *> & _geoNodes) :
			box(_b), geoNodes(_geoNodes) {
		}
		virtual ~Vis() {
		}

		const Geometry::Box & box;
		std::deque<GeometryNode *> & geoNodes;

		// ---|> NodeVisitor
		NodeVisitor::status enter(Node * node) override {
			if(!Geometry::Intersection::isBoxIntersectingBox(node->getWorldBB(), box)) {
				return BREAK_TRAVERSAL;
			}
			GeometryNode * geoNode = dynamic_cast<GeometryNode *>(node);
			if(geoNode != nullptr) {
				geoNodes.push_back(geoNode);
			}
			return CONTINUE_TRAVERSAL;
		}
	} visitor(box, geoNodes);
	root->traverse(visitor);
	return geoNodes;
}

std::deque<GeometryNode *> collectGeoNodesIntersectingSphere(Node * root, const Geometry::Vec3 & pos, float radius) {
	std::deque<GeometryNode *> geoNodes;
	struct Vis : public NodeVisitor {
		Vis(const Geometry::Vec3 & _pos, float _radius, std::deque<GeometryNode *> & _geoNodes) : pos(_pos), radiusSquared(_radius * _radius), geoNodes(_geoNodes) {
		}
		virtual ~Vis() {
		}

		const Geometry::Vec3 & pos;
		const float radiusSquared;
		std::deque<GeometryNode *> & geoNodes;

		// ---|> NodeVisitor
		NodeVisitor::status enter(Node * node) override {
			if(node->getWorldBB().getDistanceSquared(pos) > radiusSquared) {
				return BREAK_TRAVERSAL;
			}
			GeometryNode * geoNode = dynamic_cast<GeometryNode *>(node);
			if(geoNode != nullptr) {
				geoNodes.push_back(geoNode);
			}
			return CONTINUE_TRAVERSAL;
		}
	} visitor(pos, radius, geoNodes);
	root->traverse(visitor);
	return geoNodes;
}

std::deque<GeometryNode *> collectGeoNodesIntersectingRay(Node * root, const Geometry::Vec3 & pos, const Geometry::Vec3 & dir) {
	Geometry::Ray3f ray(pos, dir);
	std::deque<GeometryNode *> geoNodes;
	Geometry::Intersection::Slope<float> slope(ray);
	struct Vis : public MinSG::NodeVisitor {
		Vis(const Geometry::Intersection::Slope<float> & _s, std::deque<MinSG::GeometryNode *> & _geoNodes) :
			slope(_s), geoNodes(_geoNodes) {
		}
		virtual ~Vis() {
		}

		const Geometry::Intersection::Slope<float> & slope;
		std::deque<MinSG::GeometryNode *> & geoNodes;

		// ---|> NodeVisitor
		NodeVisitor::status enter(MinSG::Node * node) override {
			if(!slope.isRayIntersectingBox(node->getWorldBB())) {
				return BREAK_TRAVERSAL;
			}
			MinSG::GeometryNode * geoNode = dynamic_cast<MinSG::GeometryNode *>(node);
			if(geoNode != nullptr) {
				geoNodes.push_back(geoNode);
			}
			return CONTINUE_TRAVERSAL;
		}
	} visitor(slope, geoNodes);
	root->traverse(visitor);
	return geoNodes;
}

std::deque<Node *> collectInstances(Node * root, const Node * prototype) {
	std::deque<Node *> nodes;
	forEachNodeTopDown(root,[&nodes,&prototype](Node* node){
		if(node->isInstance() && node->getPrototype() == prototype) 
			nodes.push_back(node);
	});
	return nodes;
}

std::deque<Node *> collectNodesWithAttribute(Node * root, Util::StringIdentifier attrName) {
	std::deque<Node *> nodes;
	forEachNodeTopDown(root,[&nodes,&attrName](Node* node){
		if(node->getAttribute(attrName) != nullptr) 
			nodes.push_back(node);
	});
	return nodes;
}

std::deque<Node *> collectNodesReferencingAttribute(Node * root, Util::StringIdentifier attrName) {
	std::deque<Node *> nodes;
	forEachNodeTopDown(root,[&nodes,&attrName](Node* node){
		if(node->findAttribute(attrName) != nullptr) 
			nodes.push_back(node);
	});
	return nodes;
}


std::deque<Node *> collectNextNodesReferencingAttribute(Node * root, Util::StringIdentifier attrName){
	std::deque<Node *> nodes;
	traverseTopDown(root,[&root,&nodes,&attrName](Node* node){
		if(node->findAttribute(attrName) && node!=root) {
			nodes.push_back(node);
			return NodeVisitor::BREAK_TRAVERSAL;
		}
		return NodeVisitor::CONTINUE_TRAVERSAL;
	});
	return nodes;


}

std::vector<GeometryNode *> collectVisibleNodes(Node * root, FrameContext & context, float maxDistance /*=-1*/,bool fillDepthBuffer/*=false*/,renderingLayerMask_t layers/*=1*/) {
	std::vector<GeometryNode *> visibleObjects;
	if(root == nullptr) {
		return visibleObjects;
	}

	Geometry::Frustum frustum = context.getCamera()->getFrustum();
	if(maxDistance >= 0) {
		frustum.setFrustum(frustum.getLeft(), frustum.getRight(), frustum.getBottom(), frustum.getTop(), frustum.getNear(), maxDistance, frustum.isOrthogonal());
	}

	const auto objectsInVFList = collectNodesInFrustum<GeometryNode>(root, frustum);
	const size_t numQueries = objectsInVFList.size();
	if(numQueries == 0) {
		return visibleObjects;
	}

	RenderParam param(USE_WORLD_MATRIX | NO_STATES);
	param.setRenderingLayers(layers);
	
	if(fillDepthBuffer){
		// first pass
		context.getRenderingContext().clearScreen(Util::Color4f(0,0,0,0));
		for(const auto & geoNode : objectsInVFList) {
			context.displayNode(geoNode, param);
		}
	}

	context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::EQUAL));

	std::vector<Rendering::OcclusionQuery> queries(numQueries);

	size_t i = 0;
	Rendering::OcclusionQuery::enableTestMode(context.getRenderingContext());
	for(const auto & geoNode : objectsInVFList) {
		queries[i].begin(context.getRenderingContext());
		context.displayNode(geoNode, param);
		queries[i].end(context.getRenderingContext());
		++i;
	}
	Rendering::OcclusionQuery::disableTestMode(context.getRenderingContext());
	i = 0;
	for(const auto & geoNode : objectsInVFList) {
		if(queries[i].getResult(context.getRenderingContext()) > 0) {
			visibleObjects.push_back(geoNode);
		}
		++i;
	}
	context.getRenderingContext().popDepthBuffer();

	return visibleObjects;
}
// ---------------------

uint32_t countTriangles(Node * root) {
	uint32_t triangleCount = 0;
	forEachNodeTopDown<GeometryNode>(root,[&triangleCount](GeometryNode* geoNode){
		triangleCount += geoNode->getTriangleCount();
	});
	return triangleCount;	
}

uint32_t countTrianglesInFrustum(Node * root, const Geometry::Frustum & frustum) {
	const auto geoNodes = collectNodesInFrustum<GeometryNode>(root, frustum);
	uint32_t triangleCount = 0;
	for(const auto & geoNode : geoNodes) {
		triangleCount += geoNode->getTriangleCount();
	}
	return triangleCount;
}

// ----------------------------------------------------------------------------------------------------

std::deque<Node *> getChildNodes(Node * node) {
	std::deque<Node *> children;
	traverseTopDown(node,[&children,&node](Node* node2){
		if(node2 == node) 
			return NodeVisitor::CONTINUE_TRAVERSAL;
		children.push_back(node2);
		return NodeVisitor::BREAK_TRAVERSAL;
	});
	return children;
}

std::vector<uint32_t> countNodesInLevels(Node * rootNode) {
	if(rootNode == nullptr) {
		return std::vector<uint32_t>();
	}

	struct Visitor : public NodeVisitor {
		Visitor() : levelCounts(), level(0) {
		}
		virtual ~Visitor() {
		}

		std::vector<uint32_t> levelCounts;
		uint32_t level;

		NodeVisitor::status enter(Node * node) override {
			if(levelCounts.size() <= level) {
				levelCounts.resize(level + 1, 0);
			}
			++levelCounts[level];

			auto * groupNode = dynamic_cast<GroupNode *>(node);
			if(groupNode != nullptr) {
				++level;
			}
			return CONTINUE_TRAVERSAL;
		}
		NodeVisitor::status leave(Node * node) override {
			auto * groupNode = dynamic_cast<GroupNode *>(node);
			if(groupNode != nullptr) {
				--level;
			}
			return CONTINUE_TRAVERSAL;
		}
	} visitor;
	rootNode->traverse(visitor);
	return visitor.levelCounts;
}


// ----------------------------------------------------------------------------------------------------

//! moves all transformations into closed nodes, subtrees of closed nodes are not processed.
void moveTransformationsIntoClosedNodes(Node * root) {
	if(root != nullptr){
		struct Visitor : public NodeVisitor {

			Node * traversalRoot;
			Visitor(Node * _root):traversalRoot(_root){}
			virtual ~Visitor(){}

			NodeVisitor::status enter(Node * node) override {
				if(node==traversalRoot){ // root node?
					GroupNode * gn = dynamic_cast<GroupNode*>(node);
					if(gn==nullptr || gn->countChildren()==0){ // only a single node? -> do nothing.
						return EXIT_TRAVERSAL;
					}
				}else if(node->hasParent()){
					Geometry::Matrix4x4f m = node->getParent()->getRelTransformationMatrix() * node->getRelTransformationMatrix();
					if(m.convertsSafelyToSRT())
						node->setRelTransformation(m._toSRT());
					else
						node->setRelTransformation(m);
				}
				if(node->isClosed())
					return BREAK_TRAVERSAL;
				return CONTINUE_TRAVERSAL;
			}

			NodeVisitor::status leave(Node * node) override {
				if(!node->isClosed())
					node->resetRelTransformation();
				return CONTINUE_TRAVERSAL;
			}

		} visitor(root);
		root->traverse(visitor);
	}
}


//! moves all transformations into leaves
void moveTransformationsIntoLeaves(Node * root) {
	if(root != nullptr){
		struct Visitor : public NodeVisitor {
			Node * traversalRoot;
			Visitor(Node * _root):traversalRoot(_root){}
			virtual ~Visitor(){}

			NodeVisitor::status enter(Node * node) override {
				if(node==traversalRoot){ // root node?
					GroupNode * gn = dynamic_cast<GroupNode*>(node);
					if(gn==nullptr || gn->countChildren()==0){ // only a single node? -> do nothing.
						return EXIT_TRAVERSAL;
					}
				}else if(node->hasParent()){
					const Geometry::Matrix4x4f m(node->getParent()->getRelTransformationMatrix() * node->getRelTransformationMatrix());
					if(m.convertsSafelyToSRT())
						node->setRelTransformation(m._toSRT());
					else
						node->setRelTransformation(m);
				}
				return CONTINUE_TRAVERSAL;
			}

			NodeVisitor::status leave(Node * node) override {
				GroupNode * gn = dynamic_cast<GroupNode*>(node);
				if(gn && gn->countChildren() != 0)
					node->resetRelTransformation();
				return CONTINUE_TRAVERSAL;
			}

		} visitor(root);
		root->traverse(visitor);
	}
}


//! moves all statis into closed nodes, subtrees of closed nodes are not processed.
void moveStatesIntoClosedNodes(Node * root){
	if(root != nullptr){
		struct Visitor : public NodeVisitor {

			Visitor(Node* _root):root(_root){}
			virtual ~Visitor(){}

			NodeVisitor::status enter(Node * node) override {
				if(node!=root){
					for(const auto & state : node->getParent()->getStates()) {
						node->removeState(state); // remove if present
						node->addState(state); // add always
					}
				}
				if(node->isClosed())
					return BREAK_TRAVERSAL;
				return CONTINUE_TRAVERSAL;
			}

			NodeVisitor::status leave(Node * node) override {
				if(!node->isClosed())
					node->removeStates();
				return CONTINUE_TRAVERSAL;
			}

			Node * root;

		} visitor(root);
		root->traverse(visitor);
	}
}


//! moves all statis into leaves
void moveStatesIntoLeaves(Node * root){
	struct Visitor : public NodeVisitor {

		Visitor(Node* _root):root(_root){}
		virtual ~Visitor(){}

		NodeVisitor::status enter(Node * node) override {
			if(node!=root){
				for(const auto & state : node->getParent()->getStates()) {
					node->removeState(state); // remove if present
					node->addState(state); // add always
				}
			}
			GroupNode * gn = dynamic_cast<GroupNode*>(node);
			if(!gn || gn->countChildren() == 0)
				return BREAK_TRAVERSAL;
			return CONTINUE_TRAVERSAL;
		}

		NodeVisitor::status leave(Node * node) override {
			GroupNode * gn = dynamic_cast<GroupNode*>(node);
			if(gn && gn->countChildren() != 0)
				node->removeStates();
			return CONTINUE_TRAVERSAL;
		}

		Node * root;

	} visitor(root);
	root->traverse(visitor);
}

// ----------------------------------------------------------------------------------------------------


uint32_t getNodeLevel(Node * rootNode, Node * node) {
	if(rootNode == nullptr || node == nullptr) {
		return std::numeric_limits<uint32_t>::max();
	}

	struct Visitor : public NodeVisitor {
		Visitor(Node* node) : level(0), nodeLevel(std::numeric_limits<uint32_t>::max()), node(node) { }
		virtual ~Visitor() { }
		uint32_t level;
		uint32_t nodeLevel;
		Node* node;

		NodeVisitor::status enter(Node * _node) override {
			if(node == _node) {
				nodeLevel = level;
				return EXIT_TRAVERSAL;
			}
			++level;
			return CONTINUE_TRAVERSAL;
		}
		NodeVisitor::status leave(Node * _node) override {
			--level;
			return CONTINUE_TRAVERSAL;
		}
	} visitor(node);
	rootNode->traverse(visitor);
	return visitor.nodeLevel;
}


uint32_t getTreeDepth(Node * rootNode) {
	if(rootNode == nullptr) {
		return 0;
	}

	struct Visitor : public NodeVisitor {
		Visitor() : level(0), maxLevel(0) { }
		virtual ~Visitor() { }
		uint32_t level;
		uint32_t maxLevel;

		NodeVisitor::status enter(Node * _node) override {
			maxLevel = std::max(level, maxLevel);
			++level;
			return CONTINUE_TRAVERSAL;
		}
		NodeVisitor::status leave(Node * _node) override {
			--level;
			return CONTINUE_TRAVERSAL;
		}
	} visitor;
	rootNode->traverse(visitor);
	return visitor.maxLevel;
}

// --------------------


}
