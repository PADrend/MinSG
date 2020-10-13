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
	public:
		enum type_t{
			TYPE_P2P,
			TYPE_HINGE
		};
	private:
		type_t type;
		Util::Reference<Node> nodeA, nodeB;
		Geometry::Vec3 posA,dirA,posB,dirB;
		btTypedConstraint* constraint;
	
		//! create a new physic object
		BtConstraintObject(type_t t,Node & _nodeA,const Geometry::Vec3& _posA, Node & _nodeB,const Geometry::Vec3& _posB):
				type(t),nodeA(&_nodeA), nodeB(&_nodeB),posA(_posA),posB(_posB),constraint(nullptr){}
		BtConstraintObject(type_t t,Node & _nodeA,const Geometry::Vec3& _posA,const Geometry::Vec3& _dirA, Node & _nodeB,const Geometry::Vec3& _posB,const Geometry::Vec3& _dirB):
				type(t),nodeA(&_nodeA), nodeB(&_nodeB),posA(_posA),dirA(_dirA),posB(_posB),dirB(_dirB),constraint(nullptr){}
		BtConstraintObject(const BtConstraintObject&) = delete;
		BtConstraintObject(BtConstraintObject&&) = delete;
		
	public:
		static Util::Reference<BtConstraintObject> createP2P(Node& _nodeA, const Geometry::Vec3& _posA,Node& _nodeB, const Geometry::Vec3& _posB){
			return new BtConstraintObject(TYPE_P2P,_nodeA,_posA,_nodeB,_posB);
		}
		static Util::Reference<BtConstraintObject> createHinge(Node & _nodeA,const Geometry::Vec3& _posA,const Geometry::Vec3& _dirA, Node & _nodeB,const Geometry::Vec3& _posB,const Geometry::Vec3& _dirB){
			return new BtConstraintObject(TYPE_HINGE,_nodeA,_posA,_dirA,_nodeB,_posB,_dirB);
		}
		
		MINSGAPI ~BtConstraintObject();

		Node& getNodeA()const 						{	return *nodeA.get();	}
		Node& getNodeB()const 						{	return *nodeB.get();	}
		btTypedConstraint* getBtConstraint()const	{	return constraint;	}
		void setBtConstraint(btTypedConstraint* c)	{	constraint = c;	}
		const Geometry::Vec3& getPosA()const		{	return posA;	}
		const Geometry::Vec3& getDirA()const		{	return dirA;	}
		const Geometry::Vec3& getPosB()const		{	return posB;	}
		const Geometry::Vec3& getDirB()const		{	return dirB;	}
		type_t getType()const						{	return type;	}

};
}
}

#endif /* BTCONSTRAINTOBJECT_H */

#endif /* MINSG_EXT_PHYSICS */
