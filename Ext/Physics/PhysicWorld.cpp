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

#include "PhysicWorld.h"
#include "Bullet/BtPhysicWorld.h"
#include "../../Core/Nodes/Node.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../../Core/NodeAttributeModifier.h"
#include <Util/StringUtils.h>

namespace MinSG {
namespace Physics {

const Util::StringIdentifier PhysicWorld::ATTR_PHYSICS_SHAPE_DESC( NodeAttributeModifier::create( "PhysicShapeDescription", NodeAttributeModifier::PRIVATE_ATTRIBUTE));
static const Util::StringIdentifier ATTR_SHAPE_TYPE( "Physics_shapeType" );
static const Util::StringIdentifier ATTR_MASS( "Physics_mass" );
static const Util::StringIdentifier ATTR_FRICTION( "Physics_friction" );
static const Util::StringIdentifier ATTR_ROLLING_FRICTION( "Physics_rollingFriction" );
static const Util::StringIdentifier ATTR_LOCAL_SURFACE_FORCE( "Physics_localSurfaceVelocity" );
static const Util::StringIdentifier ATTR_CONSTRAINT_PIVOT( "Physics_constraintPivot" );

const Util::StringIdentifier PhysicWorld::SHAPE_TYPE("type");

const char* const PhysicWorld::SHAPE_TYPE_BOX = "bb";
const char* const PhysicWorld::SHAPE_TYPE_CONVEX_HULL = "cHull";
const char* const PhysicWorld::SHAPE_TYPE_STATIC_TRIANGLE_MESH = "mesh";
const char* const PhysicWorld::SHAPE_TYPE_SPHERE ="sphere";

////! (static)
//float PhysicWorld::getNodeProperty_mass(const Node* node, float defaultValue){
//	auto attr = node->findAttribute(ATTR_MASS);
//	return attr ? attr->toFloat() : defaultValue;
//}
////! (static)
//void PhysicWorld::setNodeProperty_mass(Node* node, float value){
//	node->setAttribute(ATTR_MASS,Util::GenericAttribute::createNumber(value));
//}
////! (static)
//float PhysicWorld::getNodeProperty_friction(const Node* node, float defaultValue){
//	auto attr = node->findAttribute(ATTR_FRICTION);
//	return attr ? attr->toFloat() : defaultValue;
//}
////! (static)
//void PhysicWorld::setNodeProperty_friction(Node* node, float value){
//	node->setAttribute(ATTR_FRICTION,Util::GenericAttribute::createNumber(value));
//}
////! (static)
//float PhysicWorld::getNodeProperty_rollingFriction(const Node* node, float defaultValue){
//	auto attr = node->findAttribute(ATTR_ROLLING_FRICTION);
//	return attr ? attr->toFloat() : defaultValue;
//}
////! (static)
//void PhysicWorld::setNodeProperty_rollingFriction(Node* node, float value){
//	node->setAttribute(ATTR_ROLLING_FRICTION,Util::GenericAttribute::createNumber(value));
//}
//! (static)
//uint32_t PhysicWorld::getNodeProperty_shapeType(const Node* node, uint32_t defaultValue){
//	auto attr = node->findAttribute(ATTR_SHAPE_TYPE);
//	return attr ? attr->toInt() : defaultValue;
//}
//
////! (static)
//Util::GenericAttributeMap* PhysicWorld::getNodeProperty_shapeDescription(const Node* node){
//	return dynamic_cast<Util::GenericAttributeMap*>(node->findAttribute(ATTR_PHYSICS_SHAPE_DESC));
//}
//
////! (static)
//bool PhysicWorld::hasLocalShapeDescription(const Node* node){
//	return dynamic_cast< Util::GenericAttributeMap*>(node->getAttribute(ATTR_PHYSICS_SHAPE_DESC));
//}
//
////! (static)
//void PhysicWorld::setNodeProperty_shapeDescription(Node* node, const Util::GenericAttributeMap& desc){
//	node->setAttribute(ATTR_PHYSICS_SHAPE_DESC,desc.clone());
//}


////! (static)
//Geometry::Vec3 PhysicWorld::getNodeProperty_localSurfaceVelocity(const Node* node){
//	const auto * attr = node->findAttribute(ATTR_LOCAL_SURFACE_FORCE);
//	if(attr){
//		const auto components = Util::StringUtils::toFloats(attr->toString());
//		FAIL_IF(components.size() != 3);
//		return Geometry::Vec3(components.data());
//	}
//	return Geometry::Vec3();
//}
//
////! (static)
//void PhysicWorld::setNodeProperty_localSurfaceVelocity(Node* node, const Geometry::Vec3& value){
//	node->unsetAttribute(ATTR_LOCAL_SURFACE_FORCE);
//	// only set the value if the force is not zero or if the prototype defines a force
//	if(!value.isZero() || node->findAttribute(ATTR_LOCAL_SURFACE_FORCE)){
//		std::ostringstream s;
//		s<<value;
//		node->setAttribute( ATTR_LOCAL_SURFACE_FORCE, Util::GenericAttribute::createString( s.str() ) );
//	}
//}
//
////! (static)
//std::string PhysicWorld::getNodeProperty_constraintPivot(const Node* node){
//	const auto *attr = node->findAttribute(ATTR_CONSTRAINT_PIVOT);
//	return attr? attr->toString(): "";
//}
////! (static)
//void PhysicWorld::setNodeProperty_constraintPivot(Node* node, const std::string &name){
//    node->unsetAttribute(ATTR_CONSTRAINT_PIVOT);
//    node->setAttribute( ATTR_CONSTRAINT_PIVOT, Util::GenericAttribute::createString( name ) );
//}

//! (static)
PhysicWorld * PhysicWorld::createBulletWorld(){
	return new BtPhysicWorld;

}

////! (static)
//bool PhysicWorld::hasPhysicsProperties(Node* node){
//	return node->findAttribute(ATTR_MASS)!=nullptr;
//}
//
////! (static)
//std::vector<Node*> PhysicWorld::collectNodesWithPhysicsProperties(Node* root){
//	std::vector<Node*> pNodes;
//	forEachNodeTopDown(root,[&pNodes](Node*node){	if(hasPhysicsProperties(node)) pNodes.push_back(node); } );
//	return pNodes;
//}
//
//Geometry::Vec3 PhysicWorld::getLocalSurfaceVelocity(Node* node) {
//	return getNodeProperty_localSurfaceVelocity(node);
//}
//
//std::string PhysicWorld::getConstraintPivot(Node* node){
//    return getNodeProperty_constraintPivot(node);
//
//}

}
}
#endif //MINSG_EXT_PHYSICS
