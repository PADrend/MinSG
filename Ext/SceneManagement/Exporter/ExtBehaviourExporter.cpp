/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExtBehaviourExporter.h"
#include "../ExtConsts.h"

#include "../../../SceneManagement/SceneDescription.h"
#include "../../../SceneManagement/SceneManager.h"
#include "../../../SceneManagement/Exporter/ExporterTools.h"

#ifdef MINSG_EXT_WAYPOINTS
#include "../../Waypoints/FollowPathBehaviour.h"
#include "../../Waypoints/PathNode.h"
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
#include "../../SkeletalAnimation/AnimationBehaviour.h"
#include "../../SkeletalAnimation/JointPose/AbstractPose.h"

#include "../../SkeletalAnimation/Joints/AbstractJoint.h"
#endif

#ifdef MINSG_EXT_PARTICLE
#include "../../ParticleSystem/ParticleEmitters.h"
#include "../../ParticleSystem/ParticleAffectors.h"
#endif

#include <functional>
#include <cassert>

namespace MinSG {
namespace SceneManagement {

#ifdef MINSG_EXT_WAYPOINTS
static DescriptionMap * exportFollowPathBehaviour(ExporterContext & ctxt,AbstractBehaviour * behaviour) {
	FollowPathBehaviour * fpb=dynamic_cast<FollowPathBehaviour *>(behaviour);
	if(!fpb)
		return nullptr;

	auto desc = new DescriptionMap;
	desc->setString(Consts::ATTR_BEHAVIOUR_TYPE, Consts::BEHAVIOUR_TYPE_FOLLOW_PATH);

	PathNode * path = fpb->getPath();
	if(path!=nullptr) {
		std::string pathId=ctxt.sceneManager.getNameOfRegisteredNode(path);
		if(pathId.empty()) {
			WARN(std::string("FollowPathBehaviour references Path without id. "));
		} else {
			desc->setString(Consts::ATTR_FOLLOW_PATH_PATH_ID,pathId);
		}
	}
	ExporterTools::finalizeBehaviourDescription(ctxt,*desc,behaviour);
	return desc;
}
#endif


#ifdef MINSG_EXT_PARTICLE
static void exportEmitter(ParticleEmitter * em, DescriptionMap * attr, ExporterContext & ctxt) {
	attr->setValue(Consts::ATTR_PARTICLE_PER_SECOND, Util::GenericAttribute::createNumber(em->getParticlesPerSecond()));

	std::stringstream pos;
	pos<<em->getDirection().getX()<<" "<<em->getDirection().getY()<<" "<<em->getDirection().getZ();
	attr->setValue(Consts::ATTR_PARTICLE_DIRECTION,Util::GenericAttribute::createString(pos.str()));

	attr->setValue(Consts::ATTR_PARTICLE_DIR_VARIANCE, Util::GenericAttribute::createNumber(em->getDirectionVarianceAngle().deg()));

	attr->setValue(Consts::ATTR_PARTICLE_MIN_WIDTH, Util::GenericAttribute::createNumber(em->getMinWidth()));
	attr->setValue(Consts::ATTR_PARTICLE_MAX_WIDTH, Util::GenericAttribute::createNumber(em->getMaxWidth()));

	attr->setValue(Consts::ATTR_PARTICLE_MIN_HEIGHT, Util::GenericAttribute::createNumber(em->getMinHeight()));
	attr->setValue(Consts::ATTR_PARTICLE_MAX_HEIGHT, Util::GenericAttribute::createNumber(em->getMaxHeight()));

	attr->setValue(Consts::ATTR_PARTICLE_MIN_SPEED, Util::GenericAttribute::createNumber(em->getMinSpeed()));
	attr->setValue(Consts::ATTR_PARTICLE_MAX_SPEED, Util::GenericAttribute::createNumber(em->getMaxSpeed()));

	attr->setValue(Consts::ATTR_PARTICLE_MIN_LIFE, Util::GenericAttribute::createNumber(em->getMinLife()));
	attr->setValue(Consts::ATTR_PARTICLE_MAX_LIFE, Util::GenericAttribute::createNumber(em->getMaxLife()));

	const Util::Color4f minColorFloat(em->getMinColor());
	const Util::Color4f maxColorFloat(em->getMaxColor());
	attr->setString(Consts::ATTR_PARTICLE_MIN_COLOR,
					Util::StringUtils::implode(minColorFloat.data(), minColorFloat.data() + 4, " "));
	attr->setString(Consts::ATTR_PARTICLE_MAX_COLOR,
					Util::StringUtils::implode(maxColorFloat.data(), maxColorFloat.data() + 4, " "));

	if(em->getSpawnNode()) {
		attr->setString(Consts::ATTR_PARTICLE_SPAWN_NODE, ctxt.sceneManager.getNameOfRegisteredNode(em->getSpawnNode()));
	}

	attr->setValue(Consts::ATTR_PARTICLE_TIME_OFFSET, Util::GenericAttribute::createNumber(em->getTimeOffset()));
}

static DescriptionMap * exportParticleGravityAffector(ExporterContext & /*ctxt*/,AbstractBehaviour * behaviour) {
	if(behaviour == nullptr || behaviour->getTypeId() != ParticleGravityAffector::getClassId())
		return nullptr;

	auto behaviourProps = new DescriptionMap();
	ParticleGravityAffector * af = static_cast<ParticleGravityAffector *>(behaviour);
	behaviourProps->setString(Consts::ATTR_BEHAVIOUR_TYPE, Consts::BEHAVIOUR_TYPE_PARTICLE_GRAVITY_AFFECTOR);

	std::stringstream pos;
	pos<<af->getGravity().getX()<<" "<<af->getGravity().getY()<<" "<<af->getGravity().getZ();
	behaviourProps->setValue(Consts::ATTR_PARTICLE_GRAVITY,Util::GenericAttribute::createString(pos.str()));

	return behaviourProps;
}
static DescriptionMap * exportParticleReflectionAffector(ExporterContext & /*ctxt*/,AbstractBehaviour * behaviour) {
	if(behaviour == nullptr || behaviour->getTypeId() != ParticleReflectionAffector::getClassId())
		return nullptr;

	auto behaviourProps = new DescriptionMap;
	ParticleReflectionAffector * af = static_cast<ParticleReflectionAffector *>(behaviour);
	behaviourProps->setString(Consts::ATTR_BEHAVIOUR_TYPE, Consts::BEHAVIOUR_TYPE_PARTICLE_REFLECTION_AFFECTOR);

	std::stringstream s;
	s<<af->getPlane();
	behaviourProps->setValue(Consts::ATTR_PARTICLE_REFLECTION_PLANE,Util::GenericAttribute::createString(s.str()));
	
	behaviourProps->setValue(Consts::ATTR_PARTICLE_REFLECTION_REFLECTIVENESS, Util::GenericAttribute::createNumber(af->getReflectiveness()));
	behaviourProps->setValue(Consts::ATTR_PARTICLE_REFLECTION_ADHERENCE, Util::GenericAttribute::createNumber(af->getAdherence()));

	return behaviourProps;
}

static DescriptionMap * exportParticleFadeOutAffector(ExporterContext & /*ctxt*/,AbstractBehaviour * behaviour) {
	if(behaviour == nullptr || behaviour->getTypeId() != ParticleFadeOutAffector::getClassId())
		return nullptr;

	auto behaviourProps = new DescriptionMap();
	behaviourProps->setString(Consts::ATTR_BEHAVIOUR_TYPE, Consts::BEHAVIOUR_TYPE_PARTICLE_FADE_OUT_AFFECTOR);

	return behaviourProps;
}

static DescriptionMap * exportParticleAnimator(ExporterContext & /*ctxt*/,AbstractBehaviour * behaviour) {
	if(behaviour == nullptr || behaviour->getTypeId() != ParticleAnimator::getClassId())
		return nullptr;

	auto behaviourProps = new DescriptionMap();
	behaviourProps->setString(Consts::ATTR_BEHAVIOUR_TYPE, Consts::BEHAVIOUR_TYPE_PARTICLE_ANIMATOR);

	return behaviourProps;
}

static DescriptionMap * exportParticlePointEmitter(ExporterContext & ctxt,AbstractBehaviour * behaviour) {
	if(behaviour == nullptr || behaviour->getTypeId() != ParticlePointEmitter::getClassId())
		return nullptr;

	auto behaviourProps = new DescriptionMap();
	behaviourProps->setString(Consts::ATTR_BEHAVIOUR_TYPE, Consts::BEHAVIOUR_TYPE_PARTICLE_POINT_EMITTER);
	ParticlePointEmitter * em = dynamic_cast<ParticlePointEmitter *>(behaviour);
	exportEmitter(em, behaviourProps, ctxt);
	std::stringstream offset;
	offset << em->getMinOffset() << " " << em->getMaxOffset();
	behaviourProps->setValue(Consts::ATTR_PARTICLE_OFFSET, Util::GenericAttribute::createString(offset.str()));

	return behaviourProps;
}

static DescriptionMap * exportParticleBoxEmitter(ExporterContext & ctxt,AbstractBehaviour * behaviour) {
	if(behaviour == nullptr || behaviour->getTypeId() != ParticleBoxEmitter::getClassId())
		return nullptr;

	auto behaviourProps = new DescriptionMap();
	behaviourProps->setString(Consts::ATTR_BEHAVIOUR_TYPE, Consts::BEHAVIOUR_TYPE_PARTICLE_BOX_EMITTER);
	ParticleBoxEmitter * em = dynamic_cast<ParticleBoxEmitter *>(behaviour);
	exportEmitter(em, behaviourProps, ctxt);
	std::stringstream box;
	box << em->getEmitBounds().getMinX() << " " << em->getEmitBounds().getMaxX() << " " <<
		em->getEmitBounds().getMinY() << " " << em->getEmitBounds().getMaxY() << " " <<
		em->getEmitBounds().getMinZ() << " " << em->getEmitBounds().getMaxZ();
	behaviourProps->setValue(Consts::ATTR_PARTICLE_EMIT_BOUNDS,Util::GenericAttribute::createString(box.str()));

	return behaviourProps;
}
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	static DescriptionMap * exportSkeletalAnimationBehaviour(ExporterContext & ctxt,AbstractBehaviour * behaviour) {
		if(behaviour == nullptr || behaviour->getTypeId() != AnimationBehaviour::getClassId())
			return nullptr;
		
		AnimationBehaviour *ani = dynamic_cast<AnimationBehaviour *>(behaviour);
		if(ani->getStatus() == AnimationBehaviour::DESTROYED)
			return nullptr;

		auto desc = new DescriptionMap();
		desc->setString(Consts::ATTR_BEHAVIOUR_TYPE, Consts::BEHAVIOUR_TYPE_SKEL_ANIMATIONDATA);
		desc->setString(Consts::ATTR_SKEL_SKELETALANIMATIONNAME, ani->getName());
		auto poseDesc = new DescriptionArray();
		std::vector<AbstractPose *> poses;
		for(auto pose : ani->getPoses())
			if(dynamic_cast<AbstractPose *>(pose))
				poses.emplace_back(dynamic_cast<AbstractPose *>(pose));
		
		std::stringstream ss;
		for(const auto & pose : poses)
		{
			ss.str("");
			auto child = new DescriptionMap();
			{
				ss.str("");
				auto it = pose->getKeyframes().begin();
				assert (it!=pose->getKeyframes().end());
				ss << *it;
				while(++it != pose->getKeyframes().end())
					ss << ' ' << *it;
				child->setString(Consts::ATTR_SKEL_SKELETALSAMPLERDATA, ss.str());
			}
			{
				ss.str("");
				auto it = pose->getTimeline().begin();
				assert(it != pose->getTimeline().end());
				ss << *it;
				while(++it != pose->getTimeline().end())
					ss << ' ' << *it;
				child->setString(Consts::ATTR_SKEL_TIMELINE, ss.str());
			}
			{
				ss.str("");
				for(auto it = pose->getInterpolationTypes()->begin(); it != pose->getInterpolationTypes()->end(); ++it)
				{
					if(it != pose->getInterpolationTypes()->begin())
						ss << ' ';
					if(*it == AbstractPose::CONSTANT)
						ss << "CONSTANT";
					else if(*it == AbstractPose::LINEAR)
						ss << "LINEAR";
					else if(*it == AbstractPose::BEZIER)
						ss << "BEZIER";
				}
				child->setString(Consts::ATTR_SKEL_SKELETALINTERPOLATIONTYPE, ss.str());
			}
			
			ss.str("");
			ss << pose->getStartTime();
			child->setString(Consts::ATTR_SKEL_SKELETALANIMATIONSTARTTIME, ss.str());

			child->setString(Consts::ATTR_SKEL_SKELETALANIMATIONTARGET, pose->getBindetJoint()->getName());

			poseDesc->push_back(child);
		}
		desc->setString(Consts::DATA_BLOCK, poseDesc->toJSON());
		
		ss.str("");
		for(auto fromAni=ani->getFromAnimations().begin(); fromAni!=ani->getFromAnimations().end(); ++fromAni)
		{
			ss << (*fromAni)->getName();
			if(fromAni != ani->getFromAnimations().end())
				ss << ";";
		}
		desc->setString(Consts::ATTR_SKEL_SKELETALFROMANIMATIONS, ss.str());
		
		ss.str("");
		for(auto toAni=ani->getToAnimations().begin(); toAni!=ani->getToAnimations().end(); ++toAni)
		{
			ss << (*toAni)->getName();
			if(toAni != ani->getToAnimations().end())
				ss << ";";
		}
		desc->setString(Consts::ATTR_SKEL_SKELETALTOANIMATIONS, ss.str());
		if(ani->isStartAnimation())
			desc->setString(Consts::ATTR_SKEL_SKELETALSTARTANIMATION, "true");

		ExporterTools::finalizeBehaviourDescription(ctxt,*desc,behaviour);
		return desc;
	}
#endif

void initExtBehaviourExporter() {

#ifdef MINSG_EXT_WAYPOINTS
	ExporterTools::registerBehaviourExporter(&exportFollowPathBehaviour);
#endif

#ifdef MINSG_EXT_SKELETAL_ANIMATION
	ExporterTools::registerBehaviourExporter(&exportSkeletalAnimationBehaviour);
#endif

#ifdef MINSG_EXT_PARTICLE
	ExporterTools::registerBehaviourExporter(&exportParticleAnimator);
	ExporterTools::registerBehaviourExporter(&exportParticleBoxEmitter);
	ExporterTools::registerBehaviourExporter(&exportParticleFadeOutAffector);
	ExporterTools::registerBehaviourExporter(&exportParticleGravityAffector);
	ExporterTools::registerBehaviourExporter(&exportParticleReflectionAffector);
	ExporterTools::registerBehaviourExporter(&exportParticlePointEmitter);
#endif
}

}
}
