/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef VISUALIZATION_H_
#define VISUALIZATION_H_

#include "../../Core/States/State.h"

namespace MinSG {
namespace TriangleTrees {

/**
 * State to display triangle trees.
 *
 * @author Benjamin Eikel
 * @date 2009-07-01
 */
class TreeVisualization : public State {
	PROVIDES_TYPE_NAME(TreeVisualization)

	public:
		//! Standard constructor
		MINSGAPI TreeVisualization();
		MINSGAPI virtual ~TreeVisualization();

		MINSGAPI TreeVisualization * clone() const override;

		//! Set the draw depth of the tree.
		void setDrawDepth(unsigned char depth) {
			drawDepth = depth;
		}
		unsigned char getDrawDepth() const {
			return drawDepth;
		}

		//! Set if splitting planes are rendered.
		void setShowSplittingPlanes(bool show) {
			showSplittingPlanes = show;
		}
		bool getShowSplittingPlanes() const {
			return showSplittingPlanes;
		}

		//! Set if bounding boxes are rendered.
		void setShowBoundingBoxes(bool show) {
			showBoundingBoxes = show;
		}
		bool getShowBoundingBoxes() const {
			return showBoundingBoxes;
		}

		//! Set if lines between parent and child are rendered.
		void setShowLines(bool show) {
			showLines = show;
		}
		bool getShowLines() const {
			return showLines;
		}

	private:
		//! Parameter that controls the draw depth of the tree.
		unsigned char drawDepth;

		//! Parameter that controls if splitting planes are rendered.
		bool showSplittingPlanes;

		//! Parameter that controls if bounding boxes are rendered.
		bool showBoundingBoxes;

		//! Parameter that controls if lines are rendered.
		bool showLines;

		//! Visualize a binary tree (e.g. ABTree, kDTree).
		MINSGAPI void displayBinaryTree(FrameContext & context, Node * root) const;

		//! Visualize an octree.
		MINSGAPI void displayOctree(FrameContext & context, Node * root) const;

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};

}
}

#endif /* VISUALIZATION_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
