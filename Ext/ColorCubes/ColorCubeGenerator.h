/*
	This file is part of the MinSG library extension ColorCubes.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2010-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2010-2011 Paul Justus <paul.justus@gmx.net>
	Copyright (C) 2010-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_COLORCUBES

#ifndef COLORCUBEGENERATOR_H
#define COLORCUBEGENERATOR_H

#include <Geometry/Definitions.h>
#include <Util/References.h>

#include <cstdint>
#include <deque>

namespace Rendering {
class Texture;
class FBO;
}
namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}

namespace MinSG {
class FrameContext;
class CameraNodeOrtho;
class Node;

/**
 *	ColorCubeGenerator is responsible for processing of color cubes.
 *
 * @author Paul Justus
 * @date 2010-08-15
 * @ingroup ext
 */
class ColorCubeGenerator {
	private:
		/** data needed to process the color cubes */
		Util::Reference<Rendering::Texture> depthTexture;
		Util::Reference<Rendering::Texture> colorTexture;
		Util::Reference<Rendering::FBO> fbo;
		Util::Reference<CameraNodeOrtho> camera;

		/** determines the maximum extension of the projection texture */
		static const uint32_t maxTexSize = 32;

	public:

		//! [ctor]
		MINSGAPI ColorCubeGenerator();

		//! [dtor]
		MINSGAPI ~ColorCubeGenerator();

		/**
		 * generates color cubes for all nodes of the specified subtree
		 * @param context : current frame context
		 * @param node    : the root of the subtree
		 */
		MINSGAPI void generateColorCubes(FrameContext& context, Node * node, uint32_t nodeCount, uint32_t triangleCount);

	private:
		/**
		 * processes color cubes for all nodes in the subtree of node stored in the ColorCubeGenerator
		 * @param context 	current rendering context
		 * @param node		root of the subtree
		 */
		MINSGAPI void processColorCubes(FrameContext& context, Node * _node, uint32_t nodeCount, uint32_t triangleCount);

		/**
		 * processes the color cube of the specified node
		 * @note the children of the specified node must already have processed color cubes
		 *
		 * @param context current rendering context
		 * @param node 		node whose color cube have to be processed
		 * @param children 	reference to the list of children of the specified node
		 */
		MINSGAPI void processColorCube(FrameContext& context, Node* node, std::deque<Node*> &children);

		/**
		 * prepares the camera: will set viewport, near/far-plane, unit scale and enables the camera if width and height
		 * of the projection resulting from the specified box side are larger than zero; otherwise camera will not be set.
		 * (it doesn't make sense to calculate the color for a side, having width or height of zero)
		 *
		 * @param context 	current rendering context
		 * @param box 	bounding box of the node, that should be processed
		 * @param side	side of the box to which the camera should be pointed
		 * @return true if camera was successfully set, otherwise false
		 */
		MINSGAPI bool prepareCamera(FrameContext& context, const Geometry::Box& box, Geometry::side_t);
};

}

#endif // COLORCUBEGENERATOR_H
#endif // MINSG_EXT_COLORCUBES
