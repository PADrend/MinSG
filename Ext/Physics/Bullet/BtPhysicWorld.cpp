/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2015 Claudius J�hn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>
	Copyright (C) 2015 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#include "BtPhysicWorld.h"
#include "BtPhysicObject.h"
#include "BtConstraintObject.h"
#include "Helper.h"
#include "MotionState.h"
#include "BtCollisionShape.h"

//debug
#include "MyDebugDraw.h"
#include "../../../Core/Nodes/Node.h"
#include "../../../Core/Nodes/GeometryNode.h"
#include "../../../Core/NodeAttributeModifier.h"
#include "../../../Core/Transformations.h"
#include "../../../Helper/StdNodeVisitors.h"
#include <Geometry/Box.h>
#include <Geometry/Plane.h>
#include <Geometry/Sphere.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/LocalMeshDataHolder.h>
#include <Rendering/RenderingContext/RenderingContext.h>

#include <Util/Utils.h>
#include <iostream>

#include <Util/Macros.h>
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-W#warnings)
COMPILER_WARN_OFF_GCC(-Wswitch-default)
COMPILER_WARN_OFF_GCC(-Woverloaded-virtual)
COMPILER_WARN_OFF_GCC(-Wshadow)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
COMPILER_WARN_OFF_GCC(-Wcast-qual)
COMPILER_WARN_OFF_GCC(-Wunused)
COMPILER_WARN_OFF_GCC(-Wunused-parameter)
COMPILER_WARN_OFF_GCC(-Wunused-variable)
#include <LinearMath/btConvexHullComputer.h>
#if (BT_BULLET_VERSION == 282) && !defined(BULLET_WARNING_PATCH)
#define BULLET_WARNING_PATCH
inline int _suppressUnusedVariableWarning(){  return btInfinityMask;} // on mingw, -Wunused-variable does not work here.
#endif
COMPILER_WARN_POP

using Geometry::Box;
using namespace Util;

namespace MinSG {
namespace Physics {



// --------- Collision shape factories
CollisionShape* _createShape_AABB(const Geometry::Box& aabb);
CollisionShape* _createShape_Sphere(const Geometry::Sphere& sphere);
CollisionShape* _createShape_Composed(const std::vector<std::pair<Util::Reference<CollisionShape>,Geometry::SRT>>& shapes);

void BtPhysicWorld::initCollisionCallbacks(BtPhysicObject& physObj){
	if(!physObj.getRigidBody())
		return;
	
	bool enableCallback = false;

	// surface velocity
//	const auto& localSurfaceVelocity = PhysicWorld::getNodeProperty_localSurfaceVelocity(physObj.getNode());
	const auto& localSurfaceVelocity = Geometry::Vec3();
	if(!localSurfaceVelocity.isZero()){
		BtPhysicObject * physObjPtr = &physObj;

		physObj.contactListener = [physObjPtr,localSurfaceVelocity](btManifoldPoint& cp,  BtPhysicObject* physObj0, BtPhysicObject* physObj1){

			Geometry::Vec3 worldVelocity;
			worldVelocity = Transformations::localDirToWorldDir(*physObjPtr->getNode(),localSurfaceVelocity);
			if(worldVelocity.isZero() || !physObj0 || !physObj1)
				return false;

			auto* body0 = physObj0->getRigidBody();
			auto* body1 = physObj1->getRigidBody();
			cp.m_combinedFriction = btManifoldResult::calculateCombinedFriction( body0, body1);
			cp.m_combinedRestitution = btManifoldResult::calculateCombinedRestitution( body0,body1);

			cp.m_contactPointFlags |= BT_CONTACT_FLAG_LATERAL_FRICTION_INITIALIZED;

			if(physObj0==physObjPtr){

				// apply surface movement
				cp.m_lateralFrictionDir1 = toBtVector3(worldVelocity);
				cp.m_lateralFrictionDir1.normalize();
				cp.m_contactMotion1 = worldVelocity.length();


				// apply friction
				if(body1->getInvMass()!=0){
					// v1 is the velocity of the moving object relative to the movement of the surface
					const Geometry::Vec3 v1 = worldVelocity+toVec3(physObj1->getRigidBody()->getInterpolationLinearVelocity());

					// v2 is the velocity of the moving object projected on the collision plane; this velocity should
					//  be reduced by friction but this seems not to happen automatically...
					const Geometry::Vec3 contaceNormal = toVec3(cp.m_normalWorldOnB).normalize();
					const Geometry::Vec3 v2 = v1 - contaceNormal * v1.dot(contaceNormal);

//					std::cout << v2.length() << " " <<v2 << " on "<< toVec3(cp.m_normalWorldOnB)<<"\n";

					/* apply force to reduce v2.
						\note this is a heuristic to simulate friction on the conveyor belt. One problem is that the friction force is applied to the object's center
								and not at the contact point. This could possibly be improved. */
					body1->applyCentralForce(toBtVector3(-v2)*cp.m_combinedFriction/body1->getInvMass());
				}

			}else{
				cp.m_lateralFrictionDir1 = toBtVector3(-worldVelocity);
				cp.m_lateralFrictionDir1.normalize();
				cp.m_contactMotion1 = worldVelocity.length();

				// apply friction
				if(body0->getInvMass()!=0){
					const Geometry::Vec3 v1 = worldVelocity+toVec3(physObj0->getRigidBody()->getInterpolationLinearVelocity());

					// v2 is the velocity of the moving object projected on the collision plane; this velocity should
					//  be reduced by friction but this seems not to happen automatically...
					const Geometry::Vec3 contaceNormal = toVec3(cp.m_normalWorldOnB).normalize();
					const Geometry::Vec3 v2 = v1 - contaceNormal * v1.dot(contaceNormal);

					std::cout << "Physics: "<<v2.length() << " " <<v2 << " on "<< toVec3(cp.m_normalWorldOnB)<<"\n";

					/* apply force to reduce v2.
						\note this is a heuristic to simulate friction on the conveyor belt. One problem is that the friction force is applied to the object's center
								and not at the contact point. This could possibly be improved. */
					body0->applyCentralForce(toBtVector3(-v2)*cp.m_combinedFriction/body0->getInvMass());
				}

			}
			return true;
		};
		enableCallback = true;
	}

	// add further collision handlers here...

	btRigidBody* body = physObj.getRigidBody();
	if(enableCallback){
		body->setCollisionFlags(body->getCollisionFlags()  | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	}else{
		body->setCollisionFlags(body->getCollisionFlags()  &  ~btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	}
}


// -------------------------------
// physicObject attribute

static const Util::StringIdentifier ATTR_PHYSICS_OBJECT(  NodeAttributeModifier::create( "btPhysicObject", NodeAttributeModifier::PRIVATE_ATTRIBUTE));


static BtPhysicObject* getPhysicObject(Node* node){
	Util::ReferenceAttribute<BtPhysicObject> * poa = node ? dynamic_cast<Util::ReferenceAttribute<BtPhysicObject> *>(node->getAttribute(ATTR_PHYSICS_OBJECT)) : nullptr;
	return poa ? poa->get() : nullptr;
}

static std::vector<Node*> collectNodesWithPhyicsObject(Node* root){
	std::vector<Node*> nodes;
	forEachNodeTopDown(root,[&nodes](Node*node){	if(node->getAttribute(ATTR_PHYSICS_OBJECT)) nodes.push_back(node); } );
	return nodes;
}

static BtPhysicObject& accessPhysicsObject(Node& node){
	Util::ReferenceAttribute<BtPhysicObject> * poa = dynamic_cast<Util::ReferenceAttribute<BtPhysicObject> *>(node.getAttribute(ATTR_PHYSICS_OBJECT));
	if(!poa){
		poa = new Util::ReferenceAttribute<BtPhysicObject>(new BtPhysicObject(&node));
		node.setAttribute(ATTR_PHYSICS_OBJECT, poa);
	}
	return *poa->get();
}


static void removePhysicsObject(Node* node){
	node->unsetAttribute(ATTR_PHYSICS_OBJECT);
}

// ---------------------------------------------------------------------------------------------------------------------

BtPhysicWorld::BtPhysicWorld(){
	// init
	broadphase = new btDbvtBroadphase();
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	solver = new btSequentialImpulseConstraintSolver;
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);

	dynamicsWorld->setGravity(btVector3(0,-10,0));

	{	// custom surface properties (like conveyor effect)
		struct _{
			static bool onNewContact(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0,int /*partId0*/,int /*index0*/,const btCollisionObjectWrapper* colObj1,int /*partId1*/,int /*index1*/){
				BtPhysicObject* physObj0 = reinterpret_cast<BtPhysicObject*>(colObj0->getCollisionObject()->getUserPointer());
				BtPhysicObject* physObj1 = reinterpret_cast<BtPhysicObject*>(colObj1->getCollisionObject()->getUserPointer());
				bool contactPointModified = false;
				if( physObj0 && physObj0->contactListener ) {
					contactPointModified |= physObj0->contactListener(cp,  physObj0,physObj1);
				}
				if( physObj1 && physObj1->contactListener ) {
					contactPointModified |= physObj1->contactListener(cp,  physObj0,physObj1);
				}
				return contactPointModified;
			}
		};
		::gContactAddedCallback = _::onNewContact;

		///this flag will use the friction information from the contact point, if contactPoint.m_lateralFrictionInitialized==true
		dynamicsWorld->getSolverInfo().m_solverMode |= SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;
	}

	shapeFactory.registerType(SHAPE_AABB, [](const Geometry::Box& aabb) -> CollisionShape* {
		return _createShape_AABB(aabb);
	});
	shapeFactory.registerType(SHAPE_SPHERE, [](const Geometry::Sphere& sphere) -> CollisionShape* {
		return _createShape_Sphere(sphere);
	});
	shapeFactory.registerType(SHAPE_COMPOSED, [](const std::vector<std::pair<Util::Reference<CollisionShape>,Geometry::SRT>>& shapes) -> CollisionShape* {
		return _createShape_Composed(shapes);
	});
}

void BtPhysicWorld::createGroundPlane(const Geometry::Plane& plane ){
	const Geometry::Vec3 normal = plane.getNormal();
	const Geometry::Vec3 pos =  normal * (plane.getOffset());
	//Creating a static shape which will act as ground
	{
//! ToDo..................................................
		 btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(normal.getX(),normal.getY(),normal.getZ()),pos.getY());

		collisionShapes.push_back(groundShape);

		btScalar mass = 0; //rigidbody is static if mass is zero, otherwise dynamic
		btVector3 localInertia(0,0,0);

		groundShape->calculateLocalInertia(mass,localInertia);

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0,pos.getY(),0));

		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform); //motionstate provides interpolation capabilities, and only synchronizes 'active' objects
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,groundShape,localInertia);
		rbInfo.m_rollingFriction =0.5; // first check it if its ok!!!!!!!!
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setUserPointer(nullptr);
		dynamicsWorld->addRigidBody(body); //add the body to the dynamics world
	}
}

static Geometry::Vec3 worldPosToLocalBodyPos(const BtPhysicObject& physObj,const Geometry::Vec3 & worldPos){
	return (Transformations::worldPosToLocalPos(*physObj.getNode(),worldPos) - physObj.getCenterOfMass()) * physObj.getNode()->getRelScaling();
}

/*! Every time the properties are applied, the old body is destroyed and a new one is created. 
	When some properties change, the old body could just be updated -- this would be more
	complex and produce more special cases. */
void BtPhysicWorld::applyProperties(Node& node){
	BtPhysicObject& physObj = accessPhysicsObject(node);
	physObj.setCenterOfMass((node.getBB()).getCenter()); // to allow a custom center of mass, this calculation must be made optional.

	btVector3 linearVelocity(0,0,0);
	btVector3 angularVelocity(0,0,0);

	auto oldBody = physObj.getRigidBody();
	if(oldBody){
		linearVelocity = oldBody->getLinearVelocity();
		angularVelocity = oldBody->getAngularVelocity();

		for(auto& constraint : physObj.getConstraints()){
			btTypedConstraint* btConstraint = constraint->getBtConstraint();
			if(btConstraint){
				constraintsToRemove.push_back(btConstraint);
				constraint->setBtConstraint(nullptr);
			}
		}
		bodiesToRemove.push_back(oldBody);
		physObj.setBody(nullptr);
	}

	auto* btShapeContainer = physObj.getShape();
	if(btShapeContainer){
		auto* btShape = btShapeContainer->getShape();
		// add additional translation proxy shape
		if(!physObj.getCenterOfMass().isZero()){ 
			auto proxyShape = new btCompoundShape;
			btTransform transformation;
			transformation.setIdentity();
			transformation.setOrigin(toBtVector3( -physObj.getCenterOfMass() ));
			proxyShape->addChildShape( transformation,  btShape );
			btShape = proxyShape;
			std::cout << "Physics: Correct center of mass "<<std::endl;
		}

		// create body
		const float mass = physObj.getMass();
	
		btVector3 localInertia(0,0,0);
		btShape->calculateLocalInertia(mass,localInertia);

		auto worldSRT =  node.getWorldTransformationSRT();
		worldSRT.translate( Transformations::localDirToWorldDir(node,physObj.getCenterOfMass() ) );

		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,
														new MotionState(*this, physObj, toBtTransform( worldSRT )),
														btShape,localInertia);
		rbInfo.m_friction = physObj.getFriction();
		rbInfo.m_rollingFriction = physObj.getRollingFriction();
		rbInfo.m_linearDamping = physObj.getLinearDamping();
		rbInfo.m_angularDamping = physObj.getAngularDamping();
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setUserPointer(&physObj);

		// init kinematic bodies
		if(physObj.getKinematicObjectMarker()){
			body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT );
			body->setActivationState(DISABLE_DEACTIVATION);
		}

		// add to world
		physObj.setBody( body );
		dynamicsWorld->addRigidBody( body );
		
		initCollisionCallbacks(physObj);
		
		// add constraints
		for(auto& constraint : physObj.getConstraints()){
			Node& otherNode = &constraint->getNodeA()==&node ? constraint->getNodeB() : constraint->getNodeA();
			BtPhysicObject& otherPhysObj = accessPhysicsObject(otherNode);
			btRigidBody* otherBody = otherPhysObj.getRigidBody();
			if(!otherBody) // add constraint when second body is available
				continue;

			const Geometry::Vec3 worldAnchorA	= Transformations::localPosToWorldPos(constraint->getNodeA(), constraint->getPosA());
			const Geometry::Vec3 worldAnchorB	= Transformations::localPosToWorldPos(constraint->getNodeB(), constraint->getPosB());
			const btVector3 thisLocalAnchor 	= toBtVector3(worldPosToLocalBodyPos(physObj,		&constraint->getNodeA()==&node ? worldAnchorA : worldAnchorB));
			const btVector3 otherLocalAnchor 	= toBtVector3(worldPosToLocalBodyPos(otherPhysObj,	&constraint->getNodeA()==&node ? worldAnchorB : worldAnchorA));
			
			switch(constraint->getType()){
				case BtConstraintObject::TYPE_P2P:{
					btTypedConstraint* p = new btPoint2PointConstraint(*body, *otherBody, thisLocalAnchor,otherLocalAnchor);
		//			constraint->setBtConstraint(p);

					std::cout<<"Physics: p2p created: "<<body<<" <->"<<otherBody<< std::endl;
					std::cout<<	(&constraint->getNodeA()==&node ? constraint->getPosA() : constraint->getPosB())<<" <->"<<
								(&constraint->getNodeA()!=&node ? constraint->getPosA() : constraint->getPosB())<< std::endl;
					p->setDbgDrawSize(5);
					dynamicsWorld->addConstraint(p, true);
					break;
				}

				case BtConstraintObject::TYPE_HINGE:{
					const btVector3 thisLocalDir 	= toBtVector3(	&constraint->getNodeA()==&node ? constraint->getDirA() : constraint->getDirB()	);
					const btVector3 otherLocalDir 	= toBtVector3(	&constraint->getNodeA()==&node ? constraint->getDirB() : constraint->getDirA()	);

					btHingeConstraint* p = new btHingeConstraint(*body, *otherBody, thisLocalAnchor,otherLocalAnchor,thisLocalDir,otherLocalDir);
					p->setDbgDrawSize(5);
					dynamicsWorld->addConstraint(p, true);			
					break;
				}
				default:
					throw std::logic_error("Invalid constraint.");
			}
		}
		
		// restore prior movement
		body->setLinearVelocity(linearVelocity); 
		body->setAngularVelocity(angularVelocity);
	}else{
		std::cout << "Physics: No shape!";
	}
}
void BtPhysicWorld::removeNode(Node* node){
	BtPhysicObject *physObj = getPhysicObject(node);
	if(physObj){
		removeConstraints(*node);

		btRigidBody* body = physObj->getRigidBody();
		if(body){
			bodiesToRemove.push_back(body);
			physObj->setBody(nullptr);
		}

		removePhysicsObject(node);
		std::cout<<"Physics: Node is removed!\n";
	}
}


void BtPhysicWorld::cleanupWorld(){
	//cleanup in the reverse order of creation/initialization
	for(int i=dynamicsWorld->getNumCollisionObjects()-1; i>=0; --i){ //remove the rigidbodies from the dynamics world and delete them

		btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if(body && body->getMotionState())
			delete body->getMotionState();

		dynamicsWorld->removeCollisionObject(obj);
		delete obj;
	}

	for(int j=0; j<collisionShapes.size(); j++) //delete collision shapes
		delete collisionShapes[j];

	collisionShapes.clear();
	delete dynamicsWorld;
	delete solver;
	delete dispatcher;
	delete collisionConfiguration;
	delete broadphase;
}

void BtPhysicWorld::stepSimulation(float time){
	if(dynamicsWorld){//step the simulation

//		std::cout << "<";
		while(!nodesToUpdate.empty()){
			Node& node = *nodesToUpdate.begin()->get();
			if(!node.isDestroyed())
				applyProperties( node );
			nodesToUpdate.erase(nodesToUpdate.begin());
//			std::cout << ".";
		}
//			std::cout << "|";

		// cleanup
		for(auto* btConstraint : constraintsToRemove){
			dynamicsWorld->removeConstraint(btConstraint);
//			delete btConstraint;	//! Ugly hack to prevent crashes
		}
		constraintsToRemove.clear();
		for(auto* btBody : bodiesToRemove){
			dynamicsWorld->removeRigidBody(btBody);
			delete btBody->getMotionState();
//			delete btBody;			//! Ugly hack to prevent crashes
		}
		bodiesToRemove.clear();
//std::cout << "|";

		simulationIsActive = true;
		dynamicsWorld->stepSimulation(time,10);
		simulationIsActive = false;
//std::cout << ">";
	}
}

void BtPhysicWorld::initNodeObserver(Node * rootNode){
	rootNode->clearTransformationObservers();
	rootNode->addTransformationObserver(std::bind(&BtPhysicWorld::onNodeTransformed,this,std::placeholders::_1));

//	//! Add all objects with physical properties to the world
//	rootNode->addNodeAddedObserver(std::bind(&BtPhysicWorld::onNodeAdded,this,std::placeholders::_1));

	// search and remove all objects physics objects
	rootNode->addNodeRemovedObserver([this](GroupNode *,Node * root){
		for(auto & node : collectNodesWithPhyicsObject(root))
			removeNode(node);
	});
//	onNodeAdded(rootNode);
}


void BtPhysicWorld::onNodeTransformed(Node * node){
	if(!simulationIsActive){
		BtPhysicObject *physObj = getPhysicObject(node);
		if(physObj){
			btRigidBody* body = physObj->getRigidBody();
			if(body){
				auto worldSRT =  node->getWorldTransformationSRT();
				worldSRT.translate( Transformations::localDirToWorldDir(*node,physObj->getCenterOfMass() ) );

				body->setWorldTransform( toBtTransform( worldSRT ));
				body->activate(true);
			
				// wake up connected nodes. Especially for kinematic objects, this seems not to happen automatically...
				for(auto& constraint : physObj->getConstraints()){
					Node& otherNode = &constraint->getNodeA()==node ? constraint->getNodeB() : constraint->getNodeA();
					BtPhysicObject& otherPhysObj = accessPhysicsObject(otherNode);
					btRigidBody* otherBody = otherPhysObj.getRigidBody();
					if(otherBody)
						otherBody->activate(true);
				}
			}
		}
	}
}

void BtPhysicWorld::markAsKinematicObject(Node& node, bool b){
	accessPhysicsObject(node).setKinematicObjectMarker(b);
	nodesToUpdate.insert(&node);
}

const Geometry::Vec3 BtPhysicWorld::getGravity(){
	return toVec3(dynamicsWorld->getGravity());
}

void BtPhysicWorld::setGravity(const Geometry::Vec3&  gravity){
	dynamicsWorld->setGravity(toBtVector3(gravity));
}

void BtPhysicWorld::setMass(Node& node, float mass){
	accessPhysicsObject(node).setMass(mass);
	nodesToUpdate.insert(&node);	
}

void BtPhysicWorld::setFriction(Node& node, float fric){
	accessPhysicsObject(node).setFriction(fric);
	nodesToUpdate.insert(&node);
}

void BtPhysicWorld::setRollingFriction(Node& node, float rollfric){
	accessPhysicsObject(node).setRollingFriction(rollfric);
	nodesToUpdate.insert(&node);
}

void BtPhysicWorld::setShape(Node& node,  Util::Reference<CollisionShape> shape){
	accessPhysicsObject(node).setShape(shape);
	nodesToUpdate.insert(&node);
}

void BtPhysicWorld::setLinearDamping(Node& node, float f){
	accessPhysicsObject(node).setLinearDamping(f);
	nodesToUpdate.insert(&node);
}
void BtPhysicWorld::setAngularDamping(Node& node, float f){
	accessPhysicsObject(node).setAngularDamping(f);
	nodesToUpdate.insert(&node);
}

void BtPhysicWorld::updateLocalSurfaceVelocity(Node* node, const Geometry::Vec3& localSurfaceVelocity){
//	setNodeProperty_localSurfaceVelocity(node,localSurfaceVelocity);
	BtPhysicObject& physObj = accessPhysicsObject(*node);
	initCollisionCallbacks(physObj);
	nodesToUpdate.insert(node);
}

void BtPhysicWorld::renderPhysicWorld(Rendering::RenderingContext& rctxt){
	MyDebugDraw *db = new MyDebugDraw(rctxt);
	db->setDebugMode(  btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawConstraints);
	dynamicsWorld->setDebugDrawer(db);
	dynamicsWorld->debugDrawWorld();

}

// ------------------------
// constraints

void BtPhysicWorld::addConstraint_p2p(Node& nodeA, const Geometry::Vec3& pivotLocalA, Node& nodeB,const Geometry::Vec3& pivotLocalB ){    
	Util::Reference<BtConstraintObject> constraint = BtConstraintObject::createP2P(nodeA,pivotLocalA,nodeB,pivotLocalB);
	
	accessPhysicsObject(nodeA).addConstraintObject( *constraint.get() );
	accessPhysicsObject(nodeB).addConstraintObject( *constraint.get() );
	nodesToUpdate.insert(&nodeA);
}

void BtPhysicWorld::addConstraint_hinge(Node& nodeA, const Geometry::Vec3& pivotLocalA, const Geometry::Vec3& dirLocalA,Node& nodeB, const Geometry::Vec3& pivotLocalB, const Geometry::Vec3& dirLocalB){
	Util::Reference<BtConstraintObject> constraint = BtConstraintObject::createHinge(nodeA,pivotLocalA,dirLocalA,nodeB,pivotLocalB,dirLocalB);
	accessPhysicsObject(nodeA).addConstraintObject( *constraint.get() );
	accessPhysicsObject(nodeB).addConstraintObject( *constraint.get() );
	nodesToUpdate.insert(&nodeA);
}

void BtPhysicWorld::removeConstraints(Node& node){
	auto& physObj = accessPhysicsObject(node);
	for(auto& constraint: physObj.getConstraints() ){
		if(constraint->getBtConstraint()){
			constraintsToRemove.push_back(constraint->getBtConstraint());
			constraint->setBtConstraint(nullptr);
		}
		if( &constraint->getNodeA()!=&node )
			accessPhysicsObject(constraint->getNodeA()).removeConstraint(*constraint.get());
		if( &constraint->getNodeB()!=&node )
			accessPhysicsObject(constraint->getNodeB()).removeConstraint(*constraint.get());
	}
	physObj.clearConstraints();
}

void BtPhysicWorld::removeConstraintBetweenNodes(Node& nodeA,Node& nodeB){
	auto& physObjA = accessPhysicsObject(nodeA);
	auto& physObjB = accessPhysicsObject(nodeB);
	for(auto& constraint: physObjA.getConstraints() ){
		if( &constraint->getNodeA()==&nodeB || &constraint->getNodeB()==&nodeB ){
			if(constraint->getBtConstraint()){
				constraintsToRemove.push_back(constraint->getBtConstraint());
				constraint->setBtConstraint(nullptr);
			}				
			physObjA.removeConstraint(*constraint.get());
			physObjB.removeConstraint(*constraint.get());
			break;
		}
	}
}
// --------- Interaction

void BtPhysicWorld::setLinearVelocity(Node& node,const Geometry::Vec3& v){
	BtPhysicObject *physObj = getPhysicObject(&node);
	if(physObj){
		btRigidBody* body = physObj->getRigidBody();
		if(body)
			body->setLinearVelocity(toBtVector3(v)); 
	}
}
void BtPhysicWorld::setAngularVelocity(Node& node,const Geometry::Vec3&v){
	BtPhysicObject *physObj = getPhysicObject(&node);
	if(physObj){
		btRigidBody* body = physObj->getRigidBody();
		if(body)
			body->setAngularVelocity(toBtVector3(v)); 
	}
}


// --------- Collision shape factories

CollisionShape* _createShape_AABB(const Geometry::Box& aabb){
	auto shape = new btCompoundShape;
	btTransform transformation;
	transformation.setIdentity();
	transformation.setOrigin(toBtVector3( aabb.getCenter() ));
	shape->addChildShape( transformation,  new btBoxShape(btVector3(aabb.getExtentX()*0.5f,aabb.getExtentY()*0.5f,aabb.getExtentZ()*0.5f)) );
	return new BtCollisionShape(shape);
}

CollisionShape* _createShape_Sphere(const Geometry::Sphere& sphere){
	auto shape = new btCompoundShape;
	btTransform transformation;
	transformation.setIdentity();
	transformation.setOrigin(toBtVector3( sphere.getCenter() ));
	shape->addChildShape( transformation,  new btSphereShape(sphere.getRadius()) );
	return new BtCollisionShape(shape);
}
CollisionShape* _createShape_Composed(const std::vector<std::pair<Util::Reference<CollisionShape>,Geometry::SRT>>& shapes){
	auto containerShape = new btCompoundShape;

	std::vector<Util::Reference<CollisionShape>> childShapes;
	for(const auto& childShapeEntry : shapes){
		BtCollisionShape* btShape = dynamic_cast<BtCollisionShape*>(childShapeEntry.first.get());
		if( !btShape )
			throw std::runtime_error("createShape_Composed: Invalid Shape type.");
		childShapes.push_back(childShapeEntry.first);

		btTransform transformation(toBtMatrix3x3(childShapeEntry.second.getRotation()),toBtVector3(childShapeEntry.second.getTranslation()) );
		containerShape->addChildShape( transformation,  btShape->getShape());
	}

	return new BtCombinedCollisionShape(containerShape,std::move(childShapes));
}

////
//////! (static,internal)
////Util::Reference<CollisionShape> BtPhysicWorld::creatShape_StaticTriangleMesh(Geometry::Mesh& mesh){
////	const Rendering::MeshIndexData & indices = mesh.openIndexData();
////	const uint32_t indexCount = mesh.getIndexCount();
////	Rendering::MeshVertexData & vertexData = mesh.openVertexData();
////	const uint32_t vertexCount = mesh.getVertexCount();
////	Util::Reference<Rendering::PositionAttributeAccessor> positionAccessor(Rendering::PositionAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::POSITION));
////	std::vector<btScalar> vertexPos;
////	vertexPos.reserve(vertexCount*3);
////	for(uint32_t i = 0; i < vertexCount; i ++) {
////		const Geometry::Vec3 a( positionAccessor->getPosition(i)*s );
////		vertexPos.push_back(a.getX());
////		vertexPos.push_back(a.getY());
////		vertexPos.push_back(a.getZ());
////	}
////	static_assert(sizeof(int)==sizeof(uint32_t),"assertion failed!");
////	btTriangleIndexVertexArray * meshInterface = new btTriangleIndexVertexArray(
////			indexCount / 3,
////			const_cast<int *>(reinterpret_cast<const int *>(indices.data())),
////			3 * sizeof(int),
////			vertexCount,
////			vertexPos.data(),
////			3 * sizeof(btScalar)
////		);
////	auto* shape = new btBvhTriangleMeshShape(meshInterface, true, true);
////	return new BtCollisionShape(shape); 
////}
////! (static,internal)
//btCollisionShape* createConvexHullShape(Node* node,const Geometry::Vec3 & centerOfMass){
//	const float s = node->getRelScaling();
//	auto geometryNode = dynamic_cast<GeometryNode *>(node);
//	if(!geometryNode && !geometryNode->getMesh())
//		throw std::logic_error("PhysicWorld::createConvexHullShape: No Mesh!");
//	Rendering::Mesh* mesh = geometryNode->getMesh();
//	Rendering::MeshVertexData& vertexData = mesh->openVertexData();
//	const uint32_t vertexCount = mesh->getVertexCount();
//	btAlignedObjectArray<btVector3> vertices2;
//	Util::Reference<Rendering::PositionAttributeAccessor> positionAccessor(Rendering::PositionAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::POSITION));
//
//	for(uint32_t i = 0; i < vertexCount; i ++) {
//		const Geometry::Vec3 a( (positionAccessor->getPosition(i)-centerOfMass)*s );
//		vertices2.push_back(btVector3(a.x(),a.y(),a.z()));
//	}
//	static_assert(sizeof(int)==sizeof(uint32_t),"assertion failed!");
//	btConvexHullComputer hullComputer;
//	hullComputer.compute(&vertices2[0].getX(), sizeof(btVector3), vertices2.size(),0.0,0);
//
//	btConvexHullShape* chs = new btConvexHullShape;
//	for(int i=0;i< hullComputer.vertices.size();++i){
//		chs->addPoint(hullComputer.vertices[i]);
//	}
//	return chs;
//}


}
}
#endif //MINSG_EXT_PHYSICS
