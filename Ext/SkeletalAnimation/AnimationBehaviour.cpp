/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "AnimationBehaviour.h"

#include "Joints/JointNode.h"
#include "JointPose/AbstractPose.h"
#include "Joints/AbstractJoint.h"
#include "SkeletalNode.h"


namespace MinSG {
    using namespace std;

AnimationBehaviour::AnimationBehaviour(SkeletalNode *node, std::string _name) : AbstractNodeBehaviour(node), nextAnimation(nullptr)
{
	animationStatus = STOP;

	minTime = 0;
	maxTime = 0;
	animationTime = 0;
    startTime = 0;

	timeFactor = 1.0;

	name = _name;

    if(node != nullptr)
        node->addAnimation(this, name);
}

AnimationBehaviour::~AnimationBehaviour(void)
{
 
}
    
AbstractPose * AnimationBehaviour::getPoseWithJointId(const uint32_t jId) const {
    for(auto pose : poses)
        if(pose->getBindetJoint()->getId() == jId)
            return pose;
    
    return nullptr;
}
    
std::vector<AnimationBehaviour *> *AnimationBehaviour::goToAnimationState(std::string _name, std::vector<AnimationBehaviour *> *tracker)
{    
    if(name == _name)
    {
        tracker->emplace_back(this);
        return tracker;
    }else if(toAnimations.empty())
    {
        tracker->clear();
        return tracker;
    }
    
    tracker->emplace_back(this);    
    for(AnimationBehaviour *beh : toAnimations)
    {
        bool loop = false;
        for(auto item : *tracker)
            if(item->getName() == beh->getName())
            {
                loop = true;
                break;
            }
        if(loop)
            continue;
        
        auto branch = new std::vector<AnimationBehaviour *>(*tracker);
        if(beh->goToAnimationState(_name, branch)->empty())
            delete branch;
        else
            return branch;
    }
    
    tracker->clear();
    return tracker;
}
    
bool AnimationBehaviour::finishedPlaying()
{    
    if(animationTime >= maxTime)
        return true;
    
    for(AbstractPose *pose : poses)
        if(pose->getStatus() == AbstractPose::STOPPED)
            return false;
    
    return true;
}
    
void AnimationBehaviour::restartPoses()
{
    for(const auto it : poses)
        it->play();
}
    
void AnimationBehaviour::setTimeOffset(double time)
{
    for(AbstractPose *item : poses)
        item->setStartTime(time);
}
    
void AnimationBehaviour::gotoTime(double time)
{
    if(time > maxTime)
        time = maxTime;
    
    if(time < startTime)
        time = startTime;
    
    for(const auto item : poses)
        item->update(time);
}
    
void AnimationBehaviour::_destroy() {
    for(auto pose : poses)
        delete pose;
    
    poses.clear();
    
    setStatus(DESTROYED);
}

AbstractBehaviour::behaviourResult_t AnimationBehaviour::doExecute()
{
    if(getStatus() == DESTROYED)
        return AbstractBehaviour::FINISHED;
    
	const timestamp_t timeSec = getCurrentTime();
    if(animationStatus == INIT) {
        oldTimeSec = timeSec;
        animationStatus = PLAYING;
    }
    
    if(animationStatus == PLAYING)
	{
		if((timeSec - oldTimeSec)*timeFactor < 0.0)
			return AbstractBehaviour::CONTINUE;

		animationTime += (timeSec - oldTimeSec)*timeFactor;
        gotoTime(animationTime);
        
        animationBehaviourFunc();

		oldTimeSec = timeSec;
	}

	return AbstractBehaviour::CONTINUE;
}
    
void AnimationBehaviour::playAnimationOnce() {
    if(animationTime >= maxTime) {
        animationStatus = STOP;
        if (nextAnimation != nullptr) {
            stop();
            nextAnimation->play();
        }
    }
}
    
void AnimationBehaviour::loopAnimation() {
    if(animationTime >= maxTime) {
        animationTime = 0.0;
        restartPoses();
    }
}
    
SkeletalNode *AnimationBehaviour::getSkeleton() 
{ 
    return dynamic_cast<SkeletalNode *>(getNode()); 
}

std::vector<AnimationBehaviour *> AnimationBehaviour::split(std::string _name, uint32_t index)
{
	std::vector<AnimationBehaviour *> anis;

	anis.push_back(new AnimationBehaviour(getSkeleton(), _name));
	for(const auto item : poses)
		anis[0]->addPose(item->split(0, index));

	anis.push_back(new AnimationBehaviour(getSkeleton(), _name));
	for(const auto item : poses)
		anis[1]->addPose(item->split(index, item->getMaxPoseCount()-1));

	return anis;
}

std::vector<AnimationBehaviour *> AnimationBehaviour::separate(std::vector<uint32_t> index, vector<string> names)
{
	std::vector<AnimationBehaviour *> anis;

	for(uint32_t i=1; i<index.size(); ++i)
	{
		anis.push_back(new AnimationBehaviour(getSkeleton(), names[i-1]));
		for(const auto item : poses)
			anis.back()->addPose(item->split(index[i-1], index[i]));
	}

	return anis;
}

void AnimationBehaviour::startLoop()
{
	play();
	animationStatus = INIT;
    
    animationBehaviourFunc = std::bind(&AnimationBehaviour::loopAnimation, this);
}

void AnimationBehaviour::stopLoop()
{
	animationStatus = PLAYING;
}
    
void AnimationBehaviour::addPoseArray(std::vector<AbstractPose*> & poseArray) {
    for(auto pose : poseArray)
        addPose(pose);
}

void AnimationBehaviour::addPose(AbstractPose *_pose)
{
	if(poses.empty())
    {
		minTime = _pose->getMinTime();
        startTime = _pose->getStartTime();
    }
	else
    {
		minTime = std::min(minTime, _pose->getMinTime());
        startTime = std::min(startTime, _pose->getStartTime());
    }

	_pose->restart();

	maxTime = std::max(maxTime, _pose->getMaxTime());
    
	poses.push_back(_pose);
}
    
void AnimationBehaviour::validateAnimationStates()
{
    const std::unordered_map<std::string, AnimationBehaviour *> states = getSkeleton()->getAnimations();
    
    fromAnimations.clear();
    for(auto from : fromAnimationNames)
    {
        auto it = states.find(from);
        if(it != states.end())
            addSourceAnimation(it->second);
    }
    
    toAnimations.clear();
    for(auto to : toAnimationNames)
    {
        auto it = states.find(to);
        if(it != states.end())
            addTargetAnimation(it->second);
    }
}

AnimationBehaviour * AnimationBehaviour::clone(SkeletalNode *nodeClone)
{
	AnimationBehaviour *myclone = new AnimationBehaviour(nodeClone, name);

    unordered_map<string, AbstractJoint *> jMap = nodeClone->getJointMap();
	for(const auto item : poses) {
        AbstractPose *poseClone = item->clone();
        poseClone->bindToJoint(jMap[item->getBindetJoint()->getName()]);
		myclone->addPose(poseClone);
    }
    
    for(const auto from : fromAnimationNames)
        myclone->addAnimationSourceName(from);
    
    for(const auto to : toAnimationNames)
        myclone->addAnimationTargetName(to);

	return myclone;
}

void AnimationBehaviour::play()
{    
	if(animationStatus == STOP)
	{
		animationStatus = INIT;
		animationTime = 0.0;
        
        animationBehaviourFunc = std::bind(&AnimationBehaviour::playAnimationOnce, this);
        
        restartPoses();
	}
}
    
void AnimationBehaviour::stop()
{
    for(const auto item : poses)
    {
        item->stop();
    }
    animationStatus = STOP;
}

void AnimationBehaviour::setStatus(int _status)
{
	animationStatus = _status;
}

}

#endif /* MINSG_EXT_SKELETAL_ANIMATION */
