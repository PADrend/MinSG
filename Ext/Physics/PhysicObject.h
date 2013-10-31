/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#ifndef PHYSICOBJECT_H
#define PHYSICOBJECT_H

#include <Util/ReferenceCounter.h>

template<typename T_> class _Vec3;
typedef _Vec3<float> Vec3;
namespace MinSG {
class Node;
namespace Physics {

class PhysicObject : public Util::ReferenceCounter<PhysicObject>{
	public:
		PhysicObject() = default;
		PhysicObject(PhysicObject&&)=default;
		virtual ~PhysicObject(){}

		//! ---o
		virtual Node* getNode()const = 0;

};

}
}

#endif /* PHYSICOBJECT_H */

#endif /* MINSG_EXT_PHYSICS */
