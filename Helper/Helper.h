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

/*!
 * removes a child from its present parent (if exists)
 * and adds it to the given new Parent (if exists)
 * such that the world matrix of the child does not change
 * @param child the child that shall be move to a new parent
 * @param newParent the parent to which the child shall be added
 * @note child->getParent() and newParent may be nullptr
 */
void changeParentKeepTransformation(Util::Reference<Node> child, GroupNode * newParent);

/*! Returns the combined world bounding boxes of the given nodes.*/
Geometry::Box combineNodesWorldBBs(const std::vector<Node*> & nodes);

//! Destroys the given subtree by calling doDestroy() on every node.
void destroy(Node * rootNode);

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
void initShaderState(ShaderState * shaderState,		const std::vector<std::string> & vsFiles,
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
Node * loadModel(const Util::FileName & filename, unsigned flags = 0, Geometry::Matrix4x4 * transMat = nullptr);
Node * loadModel(const Util::FileName & filename, unsigned flags, Geometry::Matrix4x4 * transMat,const Util::FileLocator& locator);

/*! Create a textureState from an image file.
	@param filename			Filename of the image
	@param useMipmaps Generate mipmaps for the texture and use them during rendering.
	@param clampToEdge Set the wrapping parameter such that texture accesses are clamped.
	@param textureUnit 		OpenGL texture unit (normally 0)
	@param textureRegistry 	If set and the texture is found in the registry, this texture is used; otherwise the texture file is
							loaded and stored in the registry with the filename as key.
	@return A TextureState is always returned even if the image file could not be loaded.	*/
TextureState * createTextureState(const Util::FileName & filename,
						   bool useMipmaps = false,
						   bool clampToEdge = false,
						   int textureUnit = 0,
						   std::map<const std::string, Util::Reference<Rendering::Texture> > * textureRegistry = nullptr);


}

#endif // HELPER_H
