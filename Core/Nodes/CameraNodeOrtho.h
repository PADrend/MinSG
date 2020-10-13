/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef CAMERANODEORTHO_H
#define CAMERANODEORTHO_H

#include "AbstractCameraNode.h"

namespace MinSG {

/**
 * [CameraNodeOrtho] ---|> AbstractCamera ---|> [Node]
 * @ingroup nodes
 */
class CameraNodeOrtho : public AbstractCameraNode {
	PROVIDES_TYPE_NAME(CameraNodeOrtho)
	
	public:
	
		typedef Util::Reference<CameraNodeOrtho> ref_t;
	
		CameraNodeOrtho() :
			AbstractCameraNode(), left(-1.0f), right(1.0f), bottom(-1.0f), top(1.0f) {
			updateFrustum();
		}
		virtual ~CameraNodeOrtho() {}

		float getLeftClippingPlane() const {
			return left;
		}
		float getRightClippingPlane() const {
			return right;
		}
		float getBottomClippingPlane() const {
			return bottom;
		}
		float getTopClippingPlane() const {
			return top;
		}
		void setLeftClippingPlane(float newLeft) {
			left = newLeft;
			updateFrustum();
		}
	void setRightClippingPlane(float newRight) {
			right = newRight;
			updateFrustum();
		}
		void setBottomClippingPlane(float newBottom) {
			bottom = newBottom;
			updateFrustum();
		}
		void setTopClippingPlane(float newTop) {
			top = newTop;
			updateFrustum();
		}
		void setClippingPlanes(float newLeft, float newRight, float newBottom, float newTop) {
			left = newLeft;
			right = newRight;
			bottom = newBottom;
			top = newTop;
			updateFrustum();
		}
		/**
		 * Set the clipping planes of the orthographic frustum based on the viewport.
		 * The viewport is scaled by the given factor @a unitScale.
		 */
		void setFrustumFromScaledViewport(float unitScale) {
			const float halfWidth = 0.5f * getWidth() * unitScale;
			const float halfHeight = 0.5f * getHeight() * unitScale;
			setClippingPlanes(-halfWidth, halfWidth, -halfHeight, halfHeight);
		}


	protected:
		MINSGAPI void updateFrustum() override;

	private:
		explicit CameraNodeOrtho(const CameraNodeOrtho& other) :
			AbstractCameraNode(other), left(other.left), right(other.right), bottom(other.bottom), top(other.top) {
			updateFrustum();
		}
		
		//!---|> Node
		CameraNodeOrtho * doClone() const override		{	return new CameraNodeOrtho(*this);	}
		
		float left;
		float right;
		float bottom;
		float top;
};

}
#endif // CAMERANODEORTHO_H
