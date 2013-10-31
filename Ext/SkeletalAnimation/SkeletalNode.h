/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef SKELETALNODE_H_
#define SKELETALNODE_H_

#include <map>
#include <string>

#include "../../Core/Nodes/ListNode.h"

namespace MinSG
{
	class SkeletalNode;
    class AnimationBehaviour;
    class AbstractJoint;
    class RigidJoint;
    class FrameContext;
    
    class BehaviourManager;
}

namespace MinSG
{
    /*
     *  @brief Manager for Skeleton, Animationdata and their corresponding mesh.
     *
     *  Skeleton is represented by the armature node, only one is allowed and used.
     *  By adding the armature or animationdata an connection will be created automatically.
     *
     *  Animationdata is an behaviour added to the skeletalnode. 
     *
     *  Mesh is represented by a geometrynode. 
     *
     */
	class SkeletalNode : public ListNode
	{
		PROVIDES_TYPE_NAME(SkeletalNode)
	private:
		std::unordered_map<std::string, AnimationBehaviour *> animations;
        
        AnimationBehaviour *startAnimation;
        AnimationBehaviour *currentAnimation;
        std::vector<AnimationBehaviour *> animationTracker;
        
		uint32_t jointSize;

	std::unordered_map<std::string, AbstractJoint *> jointMap;
		std::unordered_map<std::string, AbstractJoint *> generateJointMap(SkeletalNode *object);
        
        Geometry::Matrix4x4 inverseWorldMatrix;

	public:
		SkeletalNode(void);
		virtual ~SkeletalNode(void) {}

		std::unordered_map<std::string, AbstractJoint *> &getJointMap();

        /****************************************************************
         * Animation related.
         ****************************************************************/
		void playAnimation(const std::string & name, float duration, bool force=true);

		AnimationBehaviour * getAnimation(const std::string & name);
		void addAnimation(AnimationBehaviour *ani, const std::string & name);
        
        bool clearAnimation(std::string _name, BehaviourManager *manager);
        void clearAnimations(BehaviourManager *manager);

		uint32_t getAnimationCount() { return animations.size(); }
		const std::unordered_map<std::string, AnimationBehaviour *> & getAnimations() { return animations;}

        void setJointMap(std::unordered_map<std::string, AbstractJoint *> map) { jointMap = map; }
		uint32_t getJointMapSize() { return jointMap.size(); }

		bool splitAnimation(std::string sourceName, std::string targetName, int index);
		bool separateAnimation(std::string source, std::vector<std::string> names, std::vector<uint32_t> indices, BehaviourManager *manager);

		void printAnimationNames();
		std::vector<std::string> getAnimationNames();

		void startLoop(std::string aniName);
		void stopLoop(std::string aniName);

		bool hasAnimation(std::string aniName);
        
        void validateJointMap();
        
        bool setStartAnimation(std::string animationName);
        void setStartAnimationByName(std::string _name);
        
        bool goToAnimationState(std::string _name);
        
        Geometry::Matrix4x4 &getInverseWorldMatrix() const;

        /****************************************************************
         *                      ---|> Node
         ****************************************************************/
		virtual void doAddChild(Util::Reference<Node> child);

        /****************************************************************
         *  Debug caller for skeleton. Calls debugMode for all joints.
         *  Renders a circle foreach joint and a line representing the
         *  parent -> children representation. 
         ****************************************************************/
		void showSkeleton();
		void hideSkeleton();

		void hideMesh();
		void showMesh();
        
        /****************************************************************
         *  Test functions for rigidjoints. 
         ****************************************************************/
        void attachObject(Util::Reference<Node> _object, std::string _id);
        void detachObject(Util::Reference<Node> _object, std::string _id);
        
        RigidJoint *getAnchorJoint();
        
	private:
		SkeletalNode(const SkeletalNode &) = default;
		SkeletalNode* doClone()const override {	return new SkeletalNode(*this);	}
	};
}


#endif /* SKELETALNODE_H_ */
#endif /* MINSG_EXT_SKELETAL_ANIMATION */
