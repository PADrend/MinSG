/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2015 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#ifndef BTPHYSICOBJECT_H
#define BTPHYSICOBJECT_H

#include "BtConstraintObject.h"
#include "BtCollisionShape.h"
#include "../../../Core/Nodes/Node.h"
#include <Util/References.h>
#include <functional>
#include <set>

class btManifoldPoint;
class btCollisionObjectWrapper;
class btRigidBody;

namespace MinSG {

namespace Physics {

class BtPhysicObject : public Util::ReferenceCounter<BtPhysicObject>{
		Util::Reference<Node> node;
		Util::Reference<BtCollisionShape> shape;	// keep a reference as long as the body exists
		btRigidBody* body;
		std::vector<Util::Reference<BtConstraintObject>> constraints;
		Geometry::Vec3 centerOfMass;
		float mass, friction, rollingFriction,linearDamping,angularDamping;
		bool kinematicObjectMarker;
	public:

		//! create a new physic object
		BtPhysicObject(Node * _node): node(_node),body(nullptr),mass(1.0),friction(0),rollingFriction(0),linearDamping(0),angularDamping(0),
				kinematicObjectMarker(false){}
		BtPhysicObject(const BtPhysicObject&) = delete;
		BtPhysicObject(BtPhysicObject&&) = default;
		MINSGAPI ~BtPhysicObject();

		Node* getNode()const							{	return node.get();	}

		const Geometry::Vec3& getCenterOfMass()const	{	return centerOfMass;	}
		void setCenterOfMass(const Geometry::Vec3& v)	{	centerOfMass = v;	}

		bool getKinematicObjectMarker()const			{	return kinematicObjectMarker;	}
		void setKinematicObjectMarker(bool b)			{	kinematicObjectMarker = b;	}

		btRigidBody* getRigidBody()const				{	return body;	}
		void setBody(btRigidBody* _body)				{	body = _body;	}

		BtCollisionShape* getShape()const				{	return shape.get();	}
		MINSGAPI void setShape(Util::Reference<CollisionShape> _shape);
		
		float getMass()const							{	return mass;	}
		void setMass(float f)							{	mass = f;	}

		float getFriction()const						{	return friction;	}
		void setFriction(float f)						{	friction = f;	}

		float getAngularDamping()const					{	return angularDamping;	}
		void setAngularDamping(float f)					{	angularDamping = f;	}

		float getLinearDamping()const					{	return linearDamping;	}
		void setLinearDamping(float f)					{	linearDamping = f;	}

		float getRollingFriction()const					{	return rollingFriction;	}
		void setRollingFriction(float f)				{	rollingFriction = f;	}

		const std::vector<Util::Reference<BtConstraintObject>>& getConstraints() const { return constraints; }
		MINSGAPI void removeConstraint(BtConstraintObject& constraint);
		void clearConstraints()							{	constraints.clear();	}

		void addConstraintObject(BtConstraintObject& constraint) { constraints.emplace_back(&constraint); }

		typedef std::function<bool (btManifoldPoint& cp, BtPhysicObject* obj0, BtPhysicObject* obj1)> contactListener_t;
		contactListener_t contactListener;

};
}
}

#endif /* BTPHYSICOBJECT_H */

#endif /* MINSG_EXT_PHYSICS */
