/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_HELPER_H
#define MINSG_HELPER_H

#include <map>
#include <vector>
#include <string>
#include <cstdint>

/** @defgroup helper Helper 
 * Things that ease your work with MinSG.
 * Most important are the @link MinSG::NodeVisitor standard visitors@endlink to traverse the scene graph.
 */

// Forward declarations.
namespace Geometry {
template<typename _T> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4f;
typedef _Matrix4x4<double> Matrix4x4d;
typedef Matrix4x4f Matrix4x4;
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace Rendering {
class Texture;
}
namespace Util {
class FileName;
template<class t> class Reference;
class FileLocator;
}
namespace MinSG {

class Node;
class GroupNode;
class ShaderState;
class TextureState;

/** @addtogroup helper
 * @{
 */
 
/*!
 * removes a child from its present parent (if exists)
 * and adds it to the given new Parent (if exists)
 * such that the world matrix of the child does not change
 * @param child the child that shall be move to a new parent
 * @param newParent the parent to which the child shall be added
 * @note child->getParent() and newParent may be nullptr
 */
MINSGAPI void changeParentKeepTransformation(Util::Reference<Node> child, GroupNode * newParent);

/*! Returns the combined world bounding boxes of the given nodes.*/
MINSGAPI Geometry::Box combineNodesWorldBBs(const std::vector<Node*> & nodes);

//! Destroys the given subtree by calling doDestroy() on every node.
MINSGAPI void destroy(Node * rootNode);

/**
 * Return the node itself or the prototype of the node if and only if the node
 * is an instance.
 * 
 * @param node Node for which the original is requested
 * @return Original node (node itself or prototype)
 */
template<class node_type_t>
node_type_t * getOriginalNode(node_type_t * node) {
	if(node->isInstance()) {
		return static_cast<node_type_t *>(node->getPrototype());
	} else {
		return node;
	}
}

/*!	Init the given shaderState with a new Shader loaded from the given filenames. The metadata of the State is set accordingly. */
MINSGAPI void initShaderState(ShaderState * shaderState,		const std::vector<std::string> & vsFiles,
													const std::vector<std::string> & gsFiles,
													const std::vector<std::string> & fsFiles,
													uint32_t usageType,
													const Util::FileLocator& locator);


static const unsigned MESH_AUTO_CENTER = 1 << 0;
static const unsigned MESH_AUTO_SCALE = 1 << 1;
static const unsigned MESH_AUTO_CENTER_BOTTOM = 1 << 2;

/**
 *  Load a Meshes from a file.
 *  @param loader   GenericLoader that reads the mesh-data (may not be nullptr!).
 *  @param flags    Mesh modification flags (defined in MeshUtils).
 *  @param transMat Transformation matrix applied to the mesh.
 *  @param metaData
 */
MINSGAPI Node * loadModel(const Util::FileName & filename, unsigned flags = 0, Geometry::Matrix4x4 * transMat = nullptr);
MINSGAPI Node * loadModel(const Util::FileName & filename, unsigned flags, Geometry::Matrix4x4 * transMat,const Util::FileLocator& locator);

//! @}
}

#endif // HELPER_H
