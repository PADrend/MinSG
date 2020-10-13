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

#ifndef COLORCUBE_H
#define COLORCUBE_H

#include <Geometry/Definitions.h>
#include <Util/Graphics/Color.h>
#include <array>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace Rendering {
class Mesh;
}

namespace MinSG {
class Node;
class FrameContext;
class ColorCubeGenerator;

/**
 * class containing color-data used for coloring the faces of node's bounding box.
 *
 * @note a color cube does not contain information about the form/geometry of the color cubes, because it is described
 * by the absolute bounding box stored at the corresponding node.
 *
 * @note The processing of color cubes can only be done by an instance of ColorCubeGenerator. It is automatically
 * created when the color cube of any node is required but is missing and then registered at current rendering context.
 * The appropriate color cube is then available at the next frame see ColorCube::buildColorCubes(FrameContext, Node).
 *
 * @author Paul Justus
 * @date 2010-08-15
 * @ingroup ext
 *
 */
class ColorCube {
	/** colors for each of the 6 faces of a box */
	std::array<Util::Color4ub,6> colors;

	friend class ColorCubeGenerator; // define as friend, so it can modify the colors, create ColorCube-objects, etc.

public:
	/**
	 * [ctor] creates a (default) color cube with all faces colored red
	 * @note use static function buildColorCubes(FrameContext, Node) instead to ensure the existence of a color cube
	 */
	ColorCube() {
		colors.fill(Util::Color4ub(255, 0, 0, 255));
	}

	/**
	 * returns the color of the specified side
	 * @return the color
	 */
	const Util::Color4ub & getColor(Geometry::side_t side) const {
		return colors[static_cast<uint8_t>(side)];
	}

	//! Return the color cube of the given node. If the node has no color cube, throw an exception.
	MINSGAPI static const ColorCube & getColorCube(Node * node);

	//! Check if the node has got a color cube.
	MINSGAPI static bool hasColorCube(Node * node);

	/**!
	 * Stores the given colorCube in the given Node as nodeAttribute
	 * calculates a string representation of the color cube for saving in minsg files and stores it as nodeAttribute
	 *
	 * @note 	first calculate the color cube and then attach it,
	 * 			do NOT try to create, attach and finally fill the color cube with values
	 */
	MINSGAPI static void attachColorCube(Node * node, const ColorCube & cc);

	/**
	 * removes the color cube from the specified node. It also removes all color cubes on the path from specified node up to
	 * the rootNode. If the second parameter recursive is set to true, color cubes from all nodes in the subtree (with node
	 * as its root) are removed recursively (this parameter defaults to true).
	 * @note this function could/should be used when there is need for recalculating of color cube for any node/subtree
	 *
	 * @param node whose color cube should be removed / root of the subtree in which all color cubes should be removed
	 * @param recursive determines whether all color cubes in the subtree should be removed or only the color cube of the
	 * specified node.
	 */
	MINSGAPI static void removeColorCube(Node * node, bool recursive=true);

	/**
	 *
	 */
	MINSGAPI static void buildColorCubes(FrameContext & context, Node * node, uint32_t nodeCount=100, uint32_t triangleCount=20000);


private:
	//! get color cube mesh
	MINSGAPI static Rendering::Mesh * getCubeMesh();

	/**
	 * this function is used by removeColorCube(...) to remove all color cubes in the subtree
	 * @param node root of the subtree in which all color cubes should be removed
	 */
	MINSGAPI static void removeColorCubesRecursive(Node * node);

	/**
	 *	draws the box (the bounding of corresponding node) with faces colored using processed colors
	 *	(is called during rendering to draw the color cube, or during the preprocessing to draw the cubes of the children)
	 *	@param box 	to draw (belongs to the corresponding node)
	 */
	MINSGAPI void drawColoredBox(FrameContext & context, const Geometry::Box& box) const;


};

}

#endif // BOXCOLORINFO
#endif // MINSG_EXT_COLORCUBES
