/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef ABSTRACTCAMERANODE_H
#define ABSTRACTCAMERANODE_H

#include "Node.h"
#include <Geometry/Frustum.h>
#include <Geometry/Rect.h>

namespace MinSG {

/**
 * (abstract)[AbstractCamera] ---|> [Node]
 * @ingroup nodes
 */
class AbstractCameraNode : public Node {
	protected:

		/** Default constructor */
		MINSGAPI AbstractCameraNode();

		/** Copy constructor */
		MINSGAPI AbstractCameraNode(const AbstractCameraNode & o);

	public:

		typedef Util::Reference<AbstractCameraNode> ref_t;
		
		MINSGAPI virtual ~AbstractCameraNode();

		/// ---|> [Node]
		MINSGAPI void doDisplay(FrameContext & context, const RenderParam & rp) override;

		int getWidth() const							{	return viewport.getWidth();		}
		int getHeight() const							{	return viewport.getHeight();	}
		float getNearPlane() const						{	return nearPlane;				}
		float getFarPlane() const						{	return farPlane;				}
		const Geometry::Frustum & getFrustum() const	{	return frustum;					}
		const Geometry::Rect_i & getViewport() const	{	return viewport;				}
		bool isScissorEnabled() const					{	return scissorEnabled;			}
		const Geometry::Rect_i & getScissor() const		{	return scissorRect;				}

		void setViewport(const Geometry::Rect_i & _viewport, bool scissor = false) {
			viewport = _viewport;
			if(scissor) {
				setScissorEnabled(true);
				setScissor(viewport);
			}
		}
		MINSGAPI void setNearFar(float near, float far);
		void setScissorEnabled(bool enabled)			{	scissorEnabled = enabled;	}
		void setScissor(const Geometry::Rect_i & rect)	{	scissorRect = rect;	}

		Geometry::Frustum::intersection_t testBoxFrustumIntersection(const Geometry::Box & b) const {
			return frustum.isBoxInFrustum(b);
		}

		virtual void updateFrustum() = 0;

	private:
		Geometry::Rect_i viewport;
		float nearPlane, farPlane;
		Geometry::Rect_i scissorRect;
		bool scissorEnabled;

	protected:
		Geometry::Frustum frustum;
};

}

#endif // ABSTRACTCAMERANODE_H
