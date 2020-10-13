/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012,2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef CAMERANODE_H
#define CAMERANODE_H

#include "AbstractCameraNode.h"
namespace MinSG {

/**
 * [CameraNode] ---|> [AbstractCamera] ---|> [Node]
 * @ingroup nodes
 */
class CameraNode : public AbstractCameraNode {
		PROVIDES_TYPE_NAME(CameraNode)
		
	public:
	
		typedef Util::Reference<CameraNode> ref_t;

		MINSGAPI CameraNode();
		virtual ~CameraNode() {
		}

		//! Set the four frustum angles using the given horizontal apex angle and the aspect ratio of the current viewport.
		MINSGAPI void applyHorizontalAngle(const Geometry::Angle & angle);
		void applyHorizontalAngle(float deg){	applyHorizontalAngle(Geometry::Angle::deg(deg));	}
		
		//! Set the four frustum angles using the given vertical apex angle and the aspect ratio of the current viewport.
		MINSGAPI void applyVerticalAngle(const Geometry::Angle & angle);
		void applyVerticalAngle(float deg)	{	applyVerticalAngle(Geometry::Angle::deg(deg));	}

		MINSGAPI void setAngles(const Geometry::Angle & _fovLeft, const Geometry::Angle & _fovRight,
						const Geometry::Angle & _fovBottom, const Geometry::Angle & _fovTop);
		void setAngles(float _fovLeftDeg, float _fovRightDeg, float _fovBottomDeg, float _fovTopDeg){
			setAngles(Geometry::Angle::deg(_fovLeftDeg),Geometry::Angle::deg(_fovRightDeg),
						Geometry::Angle::deg(_fovBottomDeg),Geometry::Angle::deg(_fovTopDeg));
		}
		
		void getAngles(float & _fovLeftDeg, float & _fovRightDeg, float & _fovBottomDeg, float & _fovTopDeg) const {
			_fovLeftDeg = fovLeft.deg();
			_fovRightDeg = fovRight.deg();
			_fovBottomDeg = fovBottom.deg();
			_fovTopDeg = fovTop.deg();
		}
		Geometry::Angle getLeftAngle()const			{	return fovLeft;	}
		Geometry::Angle getRightAngle()const		{	return fovRight;	}
		Geometry::Angle getBottomAngle()const		{	return fovBottom;	}
		Geometry::Angle getTopAngle()const			{	return fovTop;	}
		
	private:
		MINSGAPI explicit CameraNode(const CameraNode & o);
		/// ---|> [Node]
		CameraNode * doClone()const override		{	return new CameraNode(*this);	}

	protected:
		Geometry::Angle fovLeft;
		Geometry::Angle fovRight;
		Geometry::Angle fovBottom;
		Geometry::Angle fovTop;

		MINSGAPI void updateFrustum() override;
};
}

#endif // CAMERANODE_H
