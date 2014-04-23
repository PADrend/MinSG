/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef BILLBOARD_NODE_H
#define BILLBOARD_NODE_H

#include "../../Core/Nodes/GeometryNode.h"
#include <Geometry/Rect.h>

namespace MinSG {

/**
 *  A rectangle which automatically rotates to face the observer.
 *	[BillboardNode] --->  [GeometryNode] ---|> [Node]
 *	2010-08-07 CJ
 *
 *  \todo C: I expect that the orientation may be wrong if the node's relative coordinate system is not the global one (not checked).
 *  \todo C: There are some cases where the frustum culling does not work properly. getBoundingBox has to be implementd to solve that.
 */
class BillboardNode : public GeometryNode {
	PROVIDES_TYPE_NAME(BillboardNode)

	public:
		/*! @param rotateUpAxis Orient the up-axis automatically orthogonal to the camera
								For clouds and fog probably "true", for trees and peoable probably "false"
			@param rotateRightAxis Orient the right-axis automatically orthogonal to the camera
								E.g. for multiscreen support
		*/
		BillboardNode(Geometry::Rect _rect,bool rotateUpAxis,bool rotateRightAxis=false);
		virtual ~BillboardNode();

		const Geometry::Rect & getRect()const 	{	return rect;	}
		void setRect(Geometry::Rect _rect);
		bool getRotateUpAxis()const 			{	return rotateUpAxis;	}
		bool getRotateRightAxis()const 			{	return rotateRightAxis;	}

		/// ---|> [Node]
		void doDisplay(FrameContext & context, const RenderParam & rp) override;

	private:
		explicit BillboardNode(const BillboardNode&) = default;
		/// ---|> [Node]
		BillboardNode * doClone()const override	{	return new BillboardNode(*this);	}

		bool rotateUpAxis;
		bool rotateRightAxis;
		Geometry::Rect rect;

		void createMesh();
};

}

#endif // BILLBOARD_NODE_H
