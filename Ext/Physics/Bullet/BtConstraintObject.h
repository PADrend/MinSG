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

#ifndef BTCONSTRAINTOBJECT_H
#define BTCONSTRAINTOBJECT_H

#include "../../../Core/Nodes/Node.h"
#include <Util/ReferenceCounter.h>
#include <Util/References.h>
#include <functional>

class btManifoldPoint;
class btCollisionObjectWrapper;
class btTypedConstraint;

namespace MinSG {

namespace Physics {
class BtConstraintObject : public Util::ReferenceCounter<BtConstraintObject>{
		Node* nodeA;
		Node* nodeB;
		std::unique_ptr<btTypedConstraint> constraint;
    public:

        //! create a new physic object
        BtConstraintObject(Node * _nodeA, Node * _nodeB, btTypedConstraint * _constraint):nodeA(_nodeA), nodeB(_nodeB), constraint(_constraint){}
        BtConstraintObject(const BtConstraintObject&) = delete;
        BtConstraintObject(BtConstraintObject&&) = delete;
		virtual ~BtConstraintObject();

		Node* getNodeA()const 					{	return nodeA;	}
		Node* getNodeB()const 					{	return nodeB;	}
		btTypedConstraint* getConstraint()		{	return constraint.get();	}

};
}
}

#endif /* BTCONSTRAINTOBJECT_H */

#endif /* MINSG_EXT_PHYSICS */
