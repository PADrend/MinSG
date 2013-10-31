/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef ANIMATIONBEHAVIOUR_H_
#define ANIMATIONBEHAVIOUR_H_

#include <string>
#include <vector>
#include <map>
#include <functional>

#include "../../Core/Behaviours/AbstractBehaviour.h"

namespace MinSG
{
	class AnimationBehaviour;
    class AbstractJoint;
    class JointNode;
    class AbstractPose;
    class SkeletalNode;
}


/***
 ** AnimationBehaviour ---|> AbstractNodeBehaviour
 **/

namespace MinSG
{
    /*
     *  @Brief Structure for managing animation data.
     *
     *  Behaviour storing all pose transformation joints.
     *
     */
	class AnimationBehaviour : public AbstractNodeBehaviour
	{
		PROVIDES_TYPE_NAME(SkeletalAnimationBehaviour)
        
    private:
        double minTime;
		double maxTime;
        
        double startTime;
        
        double animationTime;
		double oldTimeSec;
        
		int animationStatus;
        
		mutable std::vector<AbstractPose *> poses;
        
		double timeFactor;
        
		std::string name;
        
        std::vector<AnimationBehaviour *> fromAnimations;
        std::vector<std::string> fromAnimationNames;
        
        std::vector<AnimationBehaviour *> toAnimations;
        std::vector<std::string> toAnimationNames;
        
        AnimationBehaviour *nextAnimation;
        
        std::function<void()> animationBehaviourFunc;
        
        void playAnimationOnce();
        void loopAnimation();

	public:
        AnimationBehaviour(SkeletalNode *node, std::string _name);
		virtual ~AnimationBehaviour(void);
        
        /****************************************************************
         * Animationstatus.
         ****************************************************************/
		static const int INIT=1;
		static const int PLAYING=2;
		static const int STOP=4;
        static const int DESTROYED=7;
        
        int getStatus() const { return animationStatus; }
		void setStatus(int _status);

        /****************************************************************
         * Posehandler
         ****************************************************************/
		void addPose(AbstractPose *_pose);
        void addPoseArray(std::vector<AbstractPose*> & poseArray);
		std::vector<AbstractPose *> &getPoses() const { return poses; }
        AbstractPose * getPoseWithJointId(const uint32_t jId) const;

		int getPoseCount() const { return poses.size(); }
        
        void restartPoses();

        /****************************************************************
         * Animation access.
         ****************************************************************/
        std::string getName() const { return name; }
        
		void play();
        void stop();
        virtual void gotoTime(double time);
        
        bool isPlaying() { return animationStatus==PLAYING ? true: false; }

		void startLoop();
		void stopLoop();

		double getMinTime() const { return minTime; }
		double getMaxTime() const { return maxTime; }
        void setMaxTime(double time) { maxTime = time; }
        double getDuration() const { return maxTime-startTime; }
        double getStartTime() const { return minTime; }

		double getAnimationSpeed() const { return timeFactor; }
		void setAnimationSpeed(double _speed) { timeFactor = _speed; }
        
        void setTimeOffset(double time);
        
        bool isStartAnimation() { if(fromAnimations.empty()) return true; else return false; }
        bool isStopAnimation() { if(toAnimations.empty()) return true; else return false; }
        
        bool finishedPlaying();

        /****************************************************************
         * Animationdata manipulator
         ****************************************************************/
		std::vector<AnimationBehaviour *> split(std::string name, uint32_t index);
		std::vector<AnimationBehaviour *> separate(std::vector<uint32_t> index, std::vector<std::string> names);

        /****************************************************************
         * Animationdata validator
         ****************************************************************/
		SkeletalNode *getSkeleton();
        
        /****************************************************************
         * Animation state handler.
         ****************************************************************/
        void addSourceAnimation(AnimationBehaviour *ani) { if(ani != nullptr) fromAnimations.emplace_back(ani); }
        void addTargetAnimation(AnimationBehaviour *ani) { if(ani != nullptr) toAnimations.emplace_back(ani); }
        
        std::vector<AnimationBehaviour *> *goToAnimationState(std::string _name, std::vector<AnimationBehaviour *> *tracker);
        void setNextAnimation(AnimationBehaviour *_next) { if(_next != nullptr) nextAnimation = _next; }
        
        void addAnimationSourceName(std::string _name) { fromAnimationNames.emplace_back(_name); }
        void addAnimationTargetName(std::string _name) { toAnimationNames.emplace_back(_name); }
        
        std::vector<AnimationBehaviour *> getFromAnimations() { return fromAnimations; }
        std::vector<AnimationBehaviour *> getToAnimations() { return toAnimations; }
        
        void validateAnimationStates();
        
        /****************************************************************
         *              remove
         ****************************************************************/
        void _destroy();
        
        /****************************************************************
         *              ---|> Behaviour
         ****************************************************************/
		virtual behaviourResult_t doExecute() override;
        virtual AnimationBehaviour *clone(SkeletalNode *nodeClone);
	};
}

#endif /* ANIMATIONBEHAVIOUR_H_ */
#endif /* MINSG_EXT_SKELETAL_ANIMATION */
