#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	Bullet/BtPhysicObject.cpp
	Bullet/BtPhysicWorld.cpp
	Bullet/Helper.cpp
	Bullet/MotionState.cpp
	Bullet/BtConstraintObject.cpp
	PhysicWorld.cpp


)

minsg_add_extension(MINSG_EXT_PHYSICS "physic experiments" ${MINSG_RECOMMENDED_EXT})
minsg_add_dependencies(MINSG_EXT_PHYSICS MINSG_HAVE_LIB_BULLET)
