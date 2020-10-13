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

#ifndef WAYPOINT_H
#define WAYPOINT_H

#include "PathNode.h"
#include "../../Core/Behaviours/AbstractBehaviour.h"
#include "../../Core/Nodes/Node.h"
#include <list>

namespace MinSG {
class PathNode;

/** Waypoint class internally used by PathNodes.
 * Waypoint ---|> Node
 * @ingroup nodes
 */
class Waypoint: public Node {
		PROVIDES_TYPE_NAME(Waypoint)

		AbstractBehaviour::timestamp_t time;

		friend class PathNode;
	public:
		MINSGAPI Waypoint(const Geometry::SRT & _srt,AbstractBehaviour::timestamp_t timeSec);
		MINSGAPI Waypoint(const Waypoint &) = default;

		PathNode * getPath()const						{	return dynamic_cast<PathNode*>(getParent());	}

		AbstractBehaviour::timestamp_t getTime()const	{	return time;	}
		MINSGAPI void setTime(AbstractBehaviour::timestamp_t newTime);

		// ---|> Node
		MINSGAPI void doDisplay(FrameContext & context, const RenderParam & rp) override;

	private:
		Waypoint * doClone()const override				{	return new Waypoint(*this);	}
		MINSGAPI const Geometry::Box& doGetBB() const override;
		
};


}

#endif // WAYPOINT_H

#endif /* MINSG_EXT_WAYPOINTS */
