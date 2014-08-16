/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExtBehaviourImporter.h"
#include "../ExtConsts.h"

#include "../../../Core/Nodes/Node.h"
#include "../../../Core/Behaviours/BehaviourManager.h"
#include "../../../SceneManagement/SceneDescription.h"
#include "../../../SceneManagement/SceneManager.h"
#include "../../../SceneManagement/Importer/ImporterTools.h"
#include "../../../SceneManagement/Importer/ImportContext.h"

#ifdef MINSG_EXT_PARTICLE
#include "../../ParticleSystem/ParticleSystemNode.h"
#include "../../ParticleSystem/ParticleEmitters.h"
#include "../../ParticleSystem/ParticleAffectors.h"
#endif

#ifdef MINSG_EXT_WAYPOINTS
#include "../../Waypoints/FollowPathBehaviour.h"
#include "../../Waypoints/PathNode.h"
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
#include "../../SkeletalAnimation/SkeletalNode.h"
#include "../../SkeletalAnimation/JointPose/SRTPose.h"
#include "../../SkeletalAnimation/Joints/ArmatureNode.h"
#include "../../SkeletalAnimation/Joints/JointNode.h"
#include "../../SkeletalAnimation/AnimationBehaviour.h"
#include "../../SkeletalAnimation/Util/SkeletalAnimationUtils.h"

#include <Util/JSON_Parser.h>
#endif

#include <functional>
#include <cassert>
#include <string>

namespace MinSG {
namespace SceneManagement {

template < typename T >
static T * convertToTNode(Node * node) {
	T * t = dynamic_cast<T *>(node);
	if(t == nullptr)
		WARN(std::string(node != nullptr ? node->getTypeName() : "nullptr") + " can not be casted to " + T::getClassName());
	return t;
}

#ifdef MINSG_EXT_PARTICLE

struct AssignSpawnNodeAction {
	ParticleEmitter * emitter;
	std::string spawnNodeId;

	AssignSpawnNodeAction(ParticleEmitter * _emitter,std::string _spawnNodeId) :
		emitter(_emitter),spawnNodeId(std::move(_spawnNodeId)) {}

	void operator()(ImportContext & ctxt) {
		Node * spawnNode=ctxt.sceneManager.getRegisteredNode(spawnNodeId);
		if(spawnNode!=nullptr) {
			emitter->setSpawnNode(spawnNode);
		}
	}
};

//! load common and special data for emitters
static void initEmitter(const DescriptionMap & d, ParticleEmitter * em) {
	// common things to do with emitters
	if(d.contains(Consts::ATTR_PARTICLE_PER_SECOND))
		em->setParticlesPerSecond(d.getValue(Consts::ATTR_PARTICLE_PER_SECOND)->toFloat());

	if(d.contains(Consts::ATTR_PARTICLE_DIR_VARIANCE))
		em->setDirectionVarianceAngle(Geometry::Angle::deg(d.getValue(Consts::ATTR_PARTICLE_DIR_VARIANCE)->toFloat()));
	std::vector<float> v =
		Util::StringUtils::toFloats(d.getString(Consts::ATTR_PARTICLE_DIRECTION, ""));
	if(v.size() >= 3)
		em->setDirection(
			Geometry::Vec3f(v[0], v[1], v[2]));

	if(d.contains(Consts::ATTR_PARTICLE_MIN_LIFE))
		em->setMinLife(d.getValue(Consts::ATTR_PARTICLE_MIN_LIFE)->toFloat());
	if(d.contains(Consts::ATTR_PARTICLE_MAX_LIFE))
		em->setMaxLife(d.getValue(Consts::ATTR_PARTICLE_MAX_LIFE)->toFloat());

	if(d.contains(Consts::ATTR_PARTICLE_MIN_SPEED))
		em->setMinSpeed(d.getValue(Consts::ATTR_PARTICLE_MIN_SPEED)->toFloat());
	if(d.contains(Consts::ATTR_PARTICLE_MAX_SPEED))
		em->setMaxSpeed(d.getValue(Consts::ATTR_PARTICLE_MAX_SPEED)->toFloat());

	if(d.contains(Consts::ATTR_PARTICLE_MIN_WIDTH))
		em->setMinWidth(d.getValue(Consts::ATTR_PARTICLE_MIN_WIDTH)->toFloat());
	if(d.contains(Consts::ATTR_PARTICLE_MAX_WIDTH))
		em->setMaxWidth(d.getValue(Consts::ATTR_PARTICLE_MAX_WIDTH)->toFloat());

	if(d.contains(Consts::ATTR_PARTICLE_MIN_HEIGHT))
		em->setMinHeight(d.getValue(Consts::ATTR_PARTICLE_MIN_HEIGHT)->toFloat());
	if(d.contains(Consts::ATTR_PARTICLE_MAX_HEIGHT))
		em->setMaxHeight(d.getValue(Consts::ATTR_PARTICLE_MAX_HEIGHT)->toFloat());

	v = Util::StringUtils::toFloats(d.getString(Consts::ATTR_PARTICLE_MIN_COLOR, ""));
	if(v.size() == 4) {
		em->setMinColor(Util::Color4ub(Util::Color4f(v[0], v[1], v[2], v[3])));
	} else if(v.size() == 3) {
		em->setMinColor(Util::Color4ub(Util::Color4f(v[0], v[1], v[2])));
	}

	v = Util::StringUtils::toFloats(d.getString(Consts::ATTR_PARTICLE_MAX_COLOR, ""));
	if(v.size() == 4) {
		em->setMaxColor(Util::Color4ub(Util::Color4f(v[0], v[1], v[2], v[3])));
	} else if(v.size() == 3) {
		em->setMaxColor(Util::Color4ub(Util::Color4f(v[0], v[1], v[2])));
	}

	if(d.contains(Consts::ATTR_PARTICLE_TIME_OFFSET)) {
		em->setTimeOffset(d.getValue(Consts::ATTR_PARTICLE_TIME_OFFSET)->toFloat());
	}
}

static bool importParticlePointEmitter(ImportContext & ctxt, const std::string & type, const DescriptionMap & d, Node * parentNode) {
	if(type != Consts::BEHAVIOUR_TYPE_PARTICLE_POINT_EMITTER)
		return false;
	ParticleSystemNode * psn = convertToTNode<ParticleSystemNode>(parentNode);
	if(psn==nullptr)
		return false;
	auto point = new ParticlePointEmitter(psn);
	if(point) {
		std::vector<float> v =
			Util::StringUtils::toFloats(d.getString(Consts::ATTR_PARTICLE_OFFSET, ""));
		if(v.size() >= 2) {
			point->setMinOffset(v[0]);
			point->setMaxOffset(v[1]);
		}
	}
	initEmitter(d, point);
	ctxt.addFinalizingAction(ImportContext::FinalizeAction(AssignSpawnNodeAction(point, d.getString(Consts::ATTR_PARTICLE_SPAWN_NODE,""))));
	ctxt.sceneManager.getBehaviourManager()->registerBehaviour(point);
	return true;
}

static bool importParticleBoxEmitter(ImportContext & ctxt, const std::string & type, const DescriptionMap & d, Node * parentNode) {
	if(type != Consts::BEHAVIOUR_TYPE_PARTICLE_BOX_EMITTER)
		return false;
	ParticleSystemNode * psn = convertToTNode<ParticleSystemNode>(parentNode);
	if(psn==nullptr)
		return false;
	auto box = new ParticleBoxEmitter(psn);
	if(box) {
		std::vector<float> v =
			Util::StringUtils::toFloats(d.getString(Consts::ATTR_PARTICLE_EMIT_BOUNDS, ""));
		if(v.size() >= 6) {
			box->setEmitBounds(Geometry::Box(v[0], v[1], v[2], v[3], v[4], v[5]));
		}
	}
	initEmitter(d, box);
	ctxt.addFinalizingAction(ImportContext::FinalizeAction(AssignSpawnNodeAction(box, d.getString(Consts::ATTR_PARTICLE_SPAWN_NODE,""))));
	ctxt.sceneManager.getBehaviourManager()->registerBehaviour(box);
	return true;
}

static bool importParticleGravityEffector(ImportContext & ctxt, const std::string & type, const DescriptionMap & d, Node * parentNode) {
	if(type != Consts::BEHAVIOUR_TYPE_PARTICLE_GRAVITY_AFFECTOR)
		return false;
	ParticleSystemNode * psn = convertToTNode<ParticleSystemNode>(parentNode);
	if(psn==nullptr)
		return false;

	auto af = new ParticleGravityAffector(psn);
	std::vector<float> gravity = Util::StringUtils::toFloats(d.getString(Consts::ATTR_PARTICLE_GRAVITY, "0 -1 0"));
	if(gravity.size() >= 3)
		af->setGravity(Geometry::Vec3f(gravity[0], gravity[1], gravity[2]));

	ctxt.sceneManager.getBehaviourManager()->registerBehaviour(af);
	return true;
}
static bool importParticleReflectionAffector(ImportContext & ctxt, const std::string & type, const DescriptionMap & d, Node * parentNode) {
	if(type != Consts::BEHAVIOUR_TYPE_PARTICLE_REFLECTION_AFFECTOR)
		return false;
	ParticleSystemNode * psn = convertToTNode<ParticleSystemNode>(parentNode);
	if(psn==nullptr)
		return false;

	auto af = new ParticleReflectionAffector(psn);

	const std::string s = d.getString(Consts::ATTR_PARTICLE_REFLECTION_PLANE, "");
	if(!s.empty()){
		std::istringstream stream(s);
		Geometry::Plane p;
		stream>>p;
		af->setPlane(p);
	}
	af->setReflectiveness(d.getFloat(Consts::ATTR_PARTICLE_REFLECTION_REFLECTIVENESS,1.0));
	af->setAdherence(d.getFloat(Consts::ATTR_PARTICLE_REFLECTION_ADHERENCE,0.0));
	
	ctxt.sceneManager.getBehaviourManager()->registerBehaviour(af);
	return true;
}
static bool importParticleFadeOutAffector(ImportContext & ctxt, const std::string & type, const DescriptionMap & /*d*/, Node * parentNode) {
	if(type != Consts::BEHAVIOUR_TYPE_PARTICLE_FADE_OUT_AFFECTOR)
		return false;
	ParticleSystemNode * psn = convertToTNode<ParticleSystemNode>(parentNode);
	if(psn==nullptr)
		return false;

	auto af = new ParticleFadeOutAffector(psn);

	ctxt.sceneManager.getBehaviourManager()->registerBehaviour(af);
	return true;
}

static bool importParticleAnimator(ImportContext & ctxt, const std::string & type, const DescriptionMap & /*d*/, Node * parentNode) {
	if(type != Consts::BEHAVIOUR_TYPE_PARTICLE_ANIMATOR)
		return false;
	ParticleSystemNode * psn = convertToTNode<ParticleSystemNode>(parentNode);
	if(psn==nullptr)
		return false;

	auto af = new ParticleAnimator(psn);

	ctxt.sceneManager.getBehaviourManager()->registerBehaviour(af);
	return true;
}
#endif

#ifdef MINSG_EXT_WAYPOINTS
struct AssignPathAction {
	FollowPathBehaviour * behaviour;
	std::string pathId;

	AssignPathAction(FollowPathBehaviour * _behaviour,std::string  _pathId) :
		behaviour(_behaviour),pathId(std::move(_pathId)) {}

	void operator()(ImportContext & ctxt) {
		std::cout << "Info: Assigning PathNode to FollowPathBehaviour: "<<pathId<<" ... ";
		PathNode * pn=dynamic_cast<PathNode *>(ctxt.sceneManager.getRegisteredNode(pathId));
		if(pn!=nullptr) {
			behaviour->setPath(pn);
			std::cout << "ok.\n";
		} else {
			std::cout <<" not found!\n";
		}
	}
};

static bool importFollowPathBehaviour(ImportContext & ctxt, const std::string & type, const DescriptionMap & d, Node * parent) {
	if(type != Consts::BEHAVIOUR_TYPE_FOLLOW_PATH)
		return false;

	auto b=new FollowPathBehaviour(nullptr,parent);
	std::string pathId=d.getString(Consts::ATTR_FOLLOW_PATH_PATH_ID);
	if(!pathId.empty()) {
		ctxt.addFinalizingAction(ImportContext::FinalizeAction(AssignPathAction(b, pathId)));
	}
	ctxt.sceneManager.getBehaviourManager()->registerBehaviour(b);
	return true;
}
#endif
	
#ifdef MINSG_EXT_SKELETAL_ANIMATION
typedef Util::WrapperAttribute<std::deque<float> > float_data_t;
typedef Util::WrapperAttribute<std::deque<int> > int_data_t;
	
	
static bool importerSkeletalAnimationBehaviour(ImportContext & ctxt, const std::string & type, const DescriptionMap & d, Node * parent) 
{
	if(type != Consts::BEHAVIOUR_TYPE_SKEL_ANIMATIONDATA)
		return false;
	
	AnimationBehaviour *ani = new AnimationBehaviour(dynamic_cast<SkeletalNode*>(parent), d.getString(Consts::ATTR_SKEL_SKELETALANIMATIONNAME));
	DescriptionArray *children = dynamic_cast<DescriptionArray *>(Util::JSON_Parser::parse(d.getString(Consts::DATA_BLOCK)));
	if(children == nullptr)
		WARN("Animationdata corrupt.");
	
	std::unordered_map<std::string, AbstractJoint *> jMap = convertToTNode<SkeletalNode>(parent)->getJointMap();
	for(auto & elem : *children)
	{		
		DescriptionMap *child = dynamic_cast<DescriptionMap *>(elem.get());
		
		std::deque<float> sampleDataTemp;
		Util::StringUtils::extractFloats(child->getString(Consts::ATTR_SKEL_SKELETALSAMPLERDATA), sampleDataTemp);
		std::deque<float> timelineDataTemp;
		Util::StringUtils::extractFloats(child->getString(Consts::ATTR_SKEL_TIMELINE), timelineDataTemp);

		std::deque<double> sampleData;
		for(auto item : sampleDataTemp)
			sampleData.emplace_back(item);
		
		std::deque<double> timelineData;
		for(auto item : timelineDataTemp)
			timelineData.emplace_back(item);
		
		std::deque<uint32_t> types;
		std::string tmp;
		std::string interData = child->getString(Consts::ATTR_SKEL_SKELETALINTERPOLATIONTYPE);
		for(auto item : interData)
		{
			if(item != ' ')
				tmp += item;
			else
			{
				if(tmp == "LINEAR")
					types.push_back(0);
				else if(tmp == "CONSTANT")
					types.push_back(1);
				else if(tmp == "BEZIER")
					types.push_back(2);
				
				tmp.clear();
			}
		}
		
		tmp.clear();
		interData = child->getString(Consts::ATTR_SKEL_SKELETALTOANIMATIONS);
		for(auto item : interData)
		{
			if(item != ';')
				tmp += item;
			else
			{
				ani->addAnimationTargetName(tmp);
				tmp.clear();
			}
		}
		
 	   tmp.clear();
		interData = child->getString(Consts::ATTR_SKEL_SKELETALFROMANIMATIONS);
		for(auto item : interData)
		{
			if(item != ';')
				tmp += item;
			else
			{
				ani->addAnimationSourceName(tmp);
				tmp.clear();
			}
		}
		
		float startTime = Util::StringUtils::toFloats(child->getString(Consts::ATTR_SKEL_SKELETALANIMATIONSTARTTIME)).at(0);
		std::string targetName = child->getString(Consts::ATTR_SKEL_SKELETALANIMATIONTARGET);
		ani->addPose(new SRTPose(sampleData, timelineData, types, startTime, jMap[targetName]));
	}
	
	if(d.contains(Consts::ATTR_SKEL_SKELETALSTARTANIMATION))
		(convertToTNode<SkeletalNode>(parent))->setStartAnimation(ani->getName());

	ctxt.sceneManager.getBehaviourManager()->registerBehaviour(ani);
  return true;
}
	
#endif


//! template for new importers
// static bool importXY(ImportContext & ctxt, const std::string & behaviourType, const DescriptionMap & d, Node * parent) {
//  if(behaviourType != Consts::BEHAVIOUR_TYPE_XY) // check parent != nullptr is done by SceneManager
//	  return false;
//
//  XY * behaviour = new XY;
//
//  //TODO
//
//
//  ctxt.sceneManager.getBehaviourManager()->registerBehaviour(behaviour);
//  return true;
// }

void initExtBehaviourImporter() {

#ifdef MINSG_EXT_PARTICLE
	ImporterTools::registerBehaviourImporter(&importParticlePointEmitter);
	ImporterTools::registerBehaviourImporter(&importParticleBoxEmitter);
	ImporterTools::registerBehaviourImporter(&importParticleGravityEffector);
	ImporterTools::registerBehaviourImporter(&importParticleReflectionAffector);
	ImporterTools::registerBehaviourImporter(&importParticleFadeOutAffector);
	ImporterTools::registerBehaviourImporter(&importParticleAnimator);
#endif


#ifdef MINSG_EXT_WAYPOINTS
	ImporterTools::registerBehaviourImporter(&importFollowPathBehaviour);
#endif
	
#ifdef MINSG_EXT_SKELETAL_ANIMATION
	ImporterTools::registerBehaviourImporter(&importerSkeletalAnimationBehaviour);
#endif
}

}
}
