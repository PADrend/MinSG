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

#ifndef COLORCUBERENDERER_H_
#define COLORCUBERENDERER_H_

#include "ColorCube.h"

#include "../../Core/States/NodeRendererState.h"
#include "../../Helper/DistanceSorting.h"

namespace Rendering {
class Mesh;
}
namespace MinSG {
class FrameContext;
enum class NodeRendererResult : bool;

/**
 * color cube renderer, which is based on color-cube rendering approach. Its implementation here slightly differs from
 * that described in the paper (see below). Moreover any scene graph (Node) can be rendered with our color cube
 * renderer, because our implementation  uses  nodes' absolute bounding boxes as their color cubes
 * (this requires a different processing approach for inner nodes, see ColorCube.cpp for more details).
 *
 * 1. Before the subtree is displayed, nodes whose projection on the screen is smaller than the specified size
 *    (maximumProjSize) are collected to a priority queue (sorted in back-to-front order). Besides they are set
 *    inactive, so their subtrees cannot be displayed as geometry.
 *
 * 2. Next the subtree (of the node containing the color cube renderer) is displayed in common way (with exception
 *    of deactivated nodes and their subtrees).
 *
 * 3. Finally the nodes from the priority queue are displayed as color cubes and set active again.
 *
 * @note the various lighting states (LightingState) or shader (ShaderState) should be enabled before current
 * ColorCubeRenderer, otherwise the drawing of color cubes will ignore those lights and shaders enabled after current
 * ColorCubeRenderer.
 *
 * @author Paul Justus
 * @date 2010-07-06
 * @ingroup ext
 *
 * @see Bradford Chamberlain. "Fast rendering of complex environments using a spatial hierarchy"
 * Proceedings of the conference on Graphics interface '96, Pages: 132-141, Toronto, Ontario, Canada , 1996
 */
class ColorCubeRenderer : public NodeRendererState {
	PROVIDES_TYPE_NAME(ColorCubeRenderer)

	/**
	 *	distance queue containing nodes whose color cubes should be rendered
	 */
	std::deque<Node *> colorcubeNodes;

	/**
	 * highlight color cubes for debugging
	 */
	 bool highlight;

	 /**
	  * determine the size of the largest mesh used to display colorcubes
	  * latgestDisplayMeshSize has to be 2^largestDisplayMeshExp
	  */
	 static const uint32_t largestDisplayMeshExp = 10;
	 static const uint32_t largestDisplayMeshSize = 1024;

public:
	//!	[ctor]
	MINSGAPI ColorCubeRenderer();

	//!	[ctor]
	MINSGAPI ColorCubeRenderer(Util::StringIdentifier channel);

	//! returns if color cube highlighting is enabled (for debugging)
	bool isHighlightEnabled() const				{	return highlight;	}

	//! set whether color cube highlighting is enabled (for debugging)
	void setHighlightEnabled(bool b)			{	highlight=b;	}

	MINSGAPI NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

	MINSGAPI ColorCubeRenderer * clone() const override;

private:

	//!	renders color cubes (nodes a the priority queue)
	MINSGAPI void renderColorCubes(FrameContext& context,std::deque<Node*> & nodes)const;

	/**
	 *	On enabling the state a priority queue is created and all nodes, whose color cubes should be displayed, are collected
	 *	to it. The nodes in the priority queue are sorted in back-to-front order according to their distance from the camera.
	 *	These nodes are also set inactive, so that their subtrees cannot be displayed as geometry. Then the displaying of the
	 *	subtree will be continued in common way (STATE_OK).
	 *	If node containing the ColorCubeRenderer is small enough to be drawn as color cube, its color cube will be rendered
	 *	here and the displaying of the subtree will be skipped (STATE_SKIP_RENDERING).
	 *
	 *	@param context current rendering context
	 *	@param node containing current ColorCubeRenderer in its state list.
	 *	@param flats current rendering flags
	 */
	MINSGAPI stateResult_t doEnableState(FrameContext& context, Node * node, const RenderParam & rp) override;

	/**
	 *	normally the drawing of color cubes is done here after the displaying of the subtree has been finished (if the node
	 *	containing the ColorCubeRenderer (root) should be drawn as color cube, the disableState will not be called, so the
	 *	color cube of the root has to be displayed in enableState(...) ). After color cubes have been drawn, their nodes are
	 *	set active again.
	 *
	 *	@param context current rendering context
	 *	@param node containing current ColorCubeRenderer
	 *	@param flags current rendering flags
	 */
	MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;

	MINSGAPI static Rendering::Mesh * getColorCubesMesh(FrameContext& context, std::deque<Node*> & nodes, uint32_t meshSize, uint32_t meshIndex);
	MINSGAPI static Rendering::Mesh * createMesh(uint32_t size);
};

}

#endif // COLORCUBERENDERER_H_
#endif // MINSG_EXT_COLORCUBES
