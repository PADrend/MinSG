/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_TRANSFORMATIONS_H
#define MINSG_TRANSFORMATIONS_H

#include <Geometry/Angle.h>
#include <Geometry/Line.h>
#include <Geometry/SRT.h>

namespace MinSG {
class Node;

//! @ingroup helper
namespace Transformations {
	
//! @name coordinate transformations
//	@{
MINSGAPI Geometry::Vec3 localDirToWorldDir(const Node & node, const Geometry::Vec3 & localDir);
MINSGAPI Geometry::Vec3 localDirToRelDir(const Node & node, const Geometry::Vec3 & localDir);
MINSGAPI Geometry::Vec3 localPosToWorldPos(const Node & node, const Geometry::Vec3 & localPos);
MINSGAPI Geometry::Vec3 localPosToRelPos(const Node & node, const Geometry::Vec3 & localPos);
MINSGAPI Geometry::Vec3 relDirToWorldDir(const Node & node, const Geometry::Vec3 & relDir);
MINSGAPI Geometry::Vec3 relDirToLocalDir(const Node & node, const Geometry::Vec3 & relDir);
MINSGAPI Geometry::Vec3 relPosToWorldPos(const Node & node, const Geometry::Vec3 & relPos);
MINSGAPI Geometry::Vec3 relPosToLocalPos(const Node & node, const Geometry::Vec3 & relPos);
MINSGAPI Geometry::Vec3 worldDirToLocalDir(const Node & node, const Geometry::Vec3 & worldDir);
MINSGAPI Geometry::Vec3 worldDirToRelDir(const Node & node, const Geometry::Vec3 & worldDir);
MINSGAPI Geometry::Vec3 worldPosToLocalPos(const Node & node, const Geometry::Vec3 & worldPos);
MINSGAPI Geometry::Vec3 worldPosToRelPos(const Node & node, const Geometry::Vec3 & worldPos);
//	@}


// --------------------------------------------
	
/*! @name Node transformations
	These functions serve as an addition to the Node's internal transformation functions.
	\note If new transformations are needed, they should be added here and not to the Node!
	\note All functions defined here should also work for nodes having only matrixes (and no SRTs) -- if not noted otherwise.
*/
//	@{
MINSGAPI Geometry::SRT worldSRTToRelSRT(const Node & node, const Geometry::SRT & worldSRT);

MINSGAPI void rotateAroundLocalAxis(Node & node, const Geometry::Angle & angle,const Geometry::Line3 & localAxis);
MINSGAPI void rotateAroundRelAxis(Node & node, const Geometry::Angle & angle, const Geometry::Line3 & relAxis);
MINSGAPI void rotateAroundWorldAxis(Node & node, const Geometry::Angle & angle,const Geometry::Line3 & worldAxis);

/*! \note As no up vector is specified, the rotation is not fully determined (so just see what happens...) */
MINSGAPI void rotateToWorldDir(Node & node, const Geometry::Vec3& worldDir);
//	@}

}
}

#endif // MINSG_TRANSFORMATIONS_H
