/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "CameraNodeOrtho.h"

namespace MinSG {

void CameraNodeOrtho::updateFrustum() {
	frustum.setOrthogonal(left, right, bottom, top, getNearPlane(), getFarPlane());

	const Geometry::Matrix4x4 & worldMat = getWorldMatrix();
	const Geometry::Vec3 pos = worldMat.getColumnAsVec3(3);
	const Geometry::Vec3 dir = -worldMat.getColumnAsVec3(2);
	const Geometry::Vec3 up = worldMat.getColumnAsVec3(1);
	frustum.setPosition(pos, dir, up);
}

}
