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
#include "../Transformations.h"

namespace MinSG {

void CameraNodeOrtho::updateFrustum() {
	frustum.setOrthogonal(left, right, bottom, top, getNearPlane(), getFarPlane());
	frustum.setPosition( 	getWorldOrigin(), // pos
							Transformations::localDirToWorldDir(*this,Geometry::Vec3(0,0,-1)),  // dir
							Transformations::localDirToWorldDir(*this,Geometry::Vec3(0,1,0)));	// up
}

}
