/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef STDNODEVISITORS_H
#define STDNODEVISITORS_H

#include "../Core/NodeVisitor.h"
#include "../Core/Nodes/GroupNode.h"
#include "../Core/Nodes/Node.h"
#include "../Core/Nodes/AbstractCameraNode.h"

#include <Geometry/Box.h>

#include <deque>
#include <functional>
#include <map>
#include <set>
#include <vector>

namespace MinSG {
class GeometryNode;
/** @addtogroup helper
 * @{
 */

//--------------------------------------------------------------------
// basic traversal functions

/**
 * @brief Execute a function top-down for nodes in a subtree
 *
 * Execute the given function @p func for all nodes of type @p _T in the subtree specified by its root node @p root.
 * A depth-first, pre-order tree walk is performed on the subtree.
 *
 * @tparam _T The type of the nodes for which the function will be executed
 * @param root The root node of the subtree that will be traversed
 * @param func The function that will be executed for each node
 * @author Benjamin Eikel
 * @date 2012-07-31
 */
template<typename _T=Node>
void forEachNodeTopDown(Node * root, const std::function<void (_T *)>& func) {
	if(root == nullptr) {
		return;
	}
	struct Visitor : public NodeVisitor {
		const std::function<void (_T *)>& m_func;
		Visitor(const std::function<void (_T *)> & p_func) : m_func(p_func) {
		}
		virtual ~Visitor() = default;
		NodeVisitor::status enter(Node * node) override {
			// Use enter() to perform a pre-order tree walk
			_T * castedNode = dynamic_cast<_T *>(node);
			if(castedNode != nullptr) {
				m_func(castedNode);
			}
			return CONTINUE_TRAVERSAL;
		}
	} visitor(func);
	root->traverse(visitor);
}
template<>
MINSGAPI void forEachNodeTopDown<Node>(Node * root, const std::function<void (Node *)>& func);
inline void forEachNodeTopDown(Node* root,const std::function<void (Node *)>& func){ // define alias as template substitution does not work in this case.
	forEachNodeTopDown<Node>(root, func);
}


/**
 * @brief Execute a function bottom-up for nodes in a subtree
 *
 * Execute the given function @p func for all nodes of type @p _T in the subtree specified by its root node @p root.
 * A depth-first, post-order tree walk is performed on the subtree.
 *
 * @tparam _T The type of the nodes for which the function will be executed
 * @param root The root node of the subtree that will be traversed
 * @param func The function that will be executed for each node
 * @author Benjamin Eikel
 * @date 2012-07-31
 */
template<typename _T=Node>
void forEachNodeBottomUp(Node * root, const std::function<void (_T *)>& func) {
	if(root == nullptr) {
		return;
	}
	struct Visitor : public NodeVisitor {
		const std::function<void (_T *)>& m_func;
		Visitor(const std::function<void (_T *)>& p_func) : m_func(p_func) {
		}
		virtual ~Visitor() = default;

		NodeVisitor::status leave(Node * node) override {
			// Use leave() to perform a post-order tree walk
			_T * castedNode = dynamic_cast<_T *>(node);
			if(castedNode != nullptr) {
				m_func(castedNode);
			}
			return CONTINUE_TRAVERSAL;
		}
	} visitor(func);
	root->traverse(visitor);
}
/**
 * @brief Execute a function top-down for nodes in a subtree
 *
 * Execute the given function @p func for all nodes of type @p _T in the subtree specified by its root node @p root.
 * A depth-first, pre-order tree walk is performed on the subtree.
 *
 * @tparam _T (optional) The type of the nodes for which the function will be executed
 * @param root The root node of the subtree that will be traversed
 * @param func The function that will be executed for each node (@see NodeVisitor::status for the result)
 */
template<typename _T=Node>
void traverseTopDown(Node * root, std::function<NodeVisitor::status (_T *)> func) {
	if(root) {
		struct Visitor : public NodeVisitor {
			std::function<NodeVisitor::status (_T *)> m_func;
			Visitor(std::function<NodeVisitor::status (_T *)> p_func) : m_func(p_func) {}
			virtual ~Visitor() = default;

			// Use enter() to perform a pre-order tree walk
			NodeVisitor::status enter(Node * node) override {
				_T * castedNode = dynamic_cast<_T *>(node);
				return castedNode ? m_func(castedNode) : CONTINUE_TRAVERSAL;
			}
		} visitor(func);
		root->traverse(visitor);
	}
}
template<>
MINSGAPI void traverseTopDown<Node>(Node * root, std::function<NodeVisitor::status (Node *)> func);
inline void traverseTopDown(Node* root,const std::function<NodeVisitor::status (Node *)>& func){ // define alias as template substitution does not work in this case.
	traverseTopDown<Node>(root, func);
}

// -----------------------------------------------------------------------------------------------

/**
 * collects all closed nodes reachable from the specified scene root to a vector
 * @param root  : root of the scene graph
 * @param nodes : vector to which the closed nodes are collected (pointers to the closed nodes)
 */
MINSGAPI std::deque<Node *> collectClosedNodes(Node * root);
MINSGAPI std::deque<Node *> collectClosedNodesAtPosition(Node * root, const Geometry::Vec3 & position);
MINSGAPI std::deque<Node *> collectClosedNodesIntersectingBox(Node * root, const Geometry::Box & box);

MINSGAPI std::deque<GeometryNode *> collectGeoNodesAtPosition(Node * root, const Geometry::Vec3 & position);
MINSGAPI std::deque<GeometryNode *> collectGeoNodesIntersectingBox(Node * root, const Geometry::Box & box);
MINSGAPI std::deque<GeometryNode *> collectGeoNodesIntersectingSphere(Node * root, const Geometry::Vec3 & pos, float radius);
MINSGAPI std::deque<GeometryNode *> collectGeoNodesIntersectingRay(Node * node, const Geometry::Vec3 & pos, const Geometry::Vec3 & dir);

MINSGAPI std::deque<Node *> collectInstances(Node * root, const Node * prototype);

//! Collect all nodes in subtree with root @a root of type @c _T.
template<typename _T=Node>
std::deque<_T *> collectNodes(const Node * root) {
	std::deque<_T *> nodes;
	// use const_cast as traverse is not const (for techincal reasons),but the visitor does not alter the given node).
	forEachNodeTopDown<_T>(const_cast<Node *>(root),[&nodes](_T* node){	nodes.push_back(node);	});
	return nodes;
}

//! Collect all nodes in the given subtree which have a node attribute with the given name (\see Node::getAttribute(...)).
MINSGAPI std::deque<Node *> collectNodesWithAttribute(Node * root, Util::StringIdentifier attrName);

//! Collect all nodes in the given subtree which reference a node attribute with the given name (\see Node::findAttribute(...)).
MINSGAPI std::deque<Node *> collectNodesReferencingAttribute(Node * root, Util::StringIdentifier attrName);

//! Collect nodes below root that reference the given attribute; the traversal of a subtree is stopped if the subtree root is collected. (\see collectNodesReferencingAttribute(...)).
MINSGAPI std::deque<Node *> collectNextNodesReferencingAttribute(Node * root, Util::StringIdentifier attrName);

//! Collect all nodes in subtree with root @a root of type @c _T, that intersect with the given frustum of camera @a cam.
template<typename _T>
std::deque<_T *> collectNodesInFrustum(Node * root, const Geometry::Frustum & frustum, bool includeIntersectingNodes=true) {
	std::deque<_T *> nodes;
	if (root != nullptr){
		struct Visitor : public NodeVisitor {
			const Geometry::Frustum frustum;
			std::deque<_T *> & nodes;
			bool includeIntersectingNodes;
			uint32_t insideFrustum;

			Visitor(Geometry::Frustum _frustum, std::deque<_T *> & _nodes, bool _includeIntersectingNodes) :
				frustum(std::move(_frustum)), nodes(_nodes),includeIntersectingNodes(_includeIntersectingNodes), insideFrustum(0) {}
			virtual ~Visitor() {}

			NodeVisitor::status enter(Node * node) override {
				if (!node->isActive()) {
					return BREAK_TRAVERSAL;
				}else if (insideFrustum > 0) {
					++insideFrustum;
				} else {
					const auto result = frustum.isBoxInFrustum(node->getWorldBB());
					if (result == Geometry::Frustum::intersection_t::INSIDE) {
						++insideFrustum;
					} else if (result == Geometry::Frustum::intersection_t::INTERSECT) {
						if (!includeIntersectingNodes) {
							return CONTINUE_TRAVERSAL;
						}
					} else {
						return BREAK_TRAVERSAL;
					}
				}
				_T * n = dynamic_cast<_T*> (node);
				if (n != nullptr) {
					nodes.push_back(n);
				}
				return CONTINUE_TRAVERSAL;
			}

			NodeVisitor::status leave(Node * /*node*/) override {
				if (insideFrustum > 0)
					--insideFrustum;
				return CONTINUE_TRAVERSAL;
			}
		}v(frustum, nodes,includeIntersectingNodes );
		root->traverse(v);
	}
	return nodes;
}

//! Get all states of a specific type used in the subtree.
template<class State_t>
std::set<State_t *> collectStates(Node * root) {
	std::set<State_t *> states;
	
	forEachNodeTopDown(root,[&states](Node * node){
		for(const auto & state : node->getStates()) {
			State_t * s = dynamic_cast<State_t *>(state);
			if(s)
				states.insert(s);
		}
	});
	return states;
}

/**
 * Collect all states of the type requested on a specific path.
 * The path begins at the given node and ends at the root node.
 * If a node on the path (excluding the given node, but including the root node)
 * contains appropriate states, they are added to the resulting array.
 *
 * @tparam State_t Type of the states to collect
 * @param node Start of the path; the end is the root node
 * @return Array containing the collected states (excluding states of the given node)
 * @author Benjamin Eikel
 * @date 2012-02-29
 */
template<class State_t>
std::deque<State_t *> collectStatesUpwards(Node * node) {
	std::deque<State_t *> states;
	if(node){
		for(node = node->getParent(); // Skip the given node
				node != nullptr; node = node->getParent()) {
			for(const auto & state : node->getStates()){
				State_t * specificState = dynamic_cast<State_t *>(state);
				if(specificState)
					states.push_back(specificState);
			}
		}
	}
	return states;
}

typedef uint8_t renderingLayerMask_t;	//! \see RenderingLayer.h
/**
 * Warning: may be slow!
 * Collects visible Objects in Frustum (with occ-extension)
 * @param root Root node
 * @param context Frame context to use (contains the used camera)
 * @param maxDistance (optional) only collect nodes up to a certain distance (not exact, used for speeding up queries)
 * @param fillDepthBuffer (optional) iff true, the screen is cleared and the potential visible nodes are rendered to the depth buffer.
 * @param layers (optional) Only test nodes with this rendering layers (default=1).
 * @return Container of visible objects
 * @note - Scene has to be rendered before execution (or fillDepthBuffer must be true)
 *       - Scene has to be rendered with flag Node::USE_WORLD_MATRIX
 *
 */
MINSGAPI std::vector<GeometryNode *> collectVisibleNodes(Node * root, FrameContext & context, float maxDistance = -1,bool fillDepthBuffer=false,renderingLayerMask_t layers=1);

// -----------------------------------------------------------------------------------------------

//! Sum up the number of triangles of all geometry nodes in the subtree starting with root.
MINSGAPI uint32_t countTriangles(Node * root);

/**
 * Sum up the number of triangles of the geometry nodes in the subtree starting with root,
 * where the bounding box intersects with the given frustum.
 *
 * @param root Root node of the subtree
 * @param frustum Frustum that is used for intersection tests
 * @return Number of triangles
 */
MINSGAPI uint32_t countTrianglesInFrustum(Node * root, const Geometry::Frustum & frustum);

// -----------------------------------------------------------------------------------------------

//!	@return Direct child nodes of the given @a node
MINSGAPI std::deque<Node *> getChildNodes(Node * node);

/**
 * Count the number of nodes in the levels of the scene graph. The root node has
 * level zero.
 *
 * @param rootNode Root node of a MinSG subgraph
 * @return Array containing the number of nodes in level @c i at index @c i
 */
MINSGAPI std::vector<uint32_t> countNodesInLevels(Node * rootNode);

// -----------------------------------------------------------------------------------------------

//! moves all transformations into closed nodes, subtrees of closed nodes are not processed.
MINSGAPI void moveTransformationsIntoClosedNodes(Node * root);

//! moves all transformations into leaves
MINSGAPI void moveTransformationsIntoLeaves(Node * root);

//! moves all statis into closed nodes, subtrees of closed nodes are not processed.
MINSGAPI void moveStatesIntoClosedNodes(Node * root);

//! moves all statis into leaves
MINSGAPI void moveStatesIntoLeaves(Node * root);

// -----------------------------------------------------------------------------------------------


/**
 * gets the level of a node relative to the given root node.
 * gives a warning if the node is not a child of the root node and returns the max. Integer
 *
 * @param rootNode Root node of a MinSG subgraph
 * @param node Node in the subtree of rootNode
 * @return The level of the node or 0xffffffffU (max. of uint32_t)
 */
MINSGAPI uint32_t getNodeLevel(Node * rootNode, Node * node);


/**
 * gets the of the given subtree.
 *
 * @param rootNode Root node of a MinSG subgraph
 * @return The depth of the given subtree
 */
MINSGAPI uint32_t getTreeDepth(Node * rootNode);

// -----------------------------------------------------------------------------------------------

//! @}
}
#endif // STDNODEVISITORS_H
