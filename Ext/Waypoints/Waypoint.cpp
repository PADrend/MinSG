/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_WAYPOINTS

#include "Waypoint.h"

#include "../../Core/FrameContext.h"
#include <Geometry/Matrix4x4.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/PrimitiveShapes.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Util/Graphics/ColorLibrary.h>

#include "PathNode.h"

namespace MinSG {

Waypoint::Waypoint(const Geometry::SRT & _srt, AbstractBehaviour::timestamp_t timeSec) :
	Node(), time(timeSec) {
	setRelTransformation(_srt);
}

void Waypoint::setTime(AbstractBehaviour::timestamp_t newTime) {
	PathNode * path = getPath();
	if(path) {
		path->updateWaypoint(this, newTime);
	} else {
		time = newTime;
	}
}

void Waypoint::doDisplay(FrameContext & context, const RenderParam & rp) {
	if(!(rp.getFlag(SHOW_META_OBJECTS))) {
		return;
	}

	static Util::Reference<Rendering::Mesh> arrowMesh;
	if(arrowMesh.isNull()) {
		Rendering::VertexDescription vd;
		vd.appendPosition3D();
		vd.appendNormalByte();
		vd.appendColorRGBAByte();
		arrowMesh = Rendering::MeshUtils::createArrow(vd, 0.1f, 1.0f);
		Geometry::Matrix4x4f transformation;
		transformation.rotate_deg(90.0f, 0.0f, 1.0f, 0.0f);
		Rendering::MeshUtils::transform(arrowMesh->openVertexData(), transformation);
	}
	context.getRenderingContext().displayMesh(arrowMesh.get());
}

const Geometry::Box& Waypoint::doGetBB() const {
	static const Geometry::Box bb(Geometry::Vec3f(0.0f, 0.0f, 0.0f), 0.001f);
	return bb;
}

}

#endif /* MINSG_EXT_WAYPOINTS */
