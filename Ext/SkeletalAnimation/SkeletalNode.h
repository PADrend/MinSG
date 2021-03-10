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
	 * @ingroup nodes
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
		MINSGAPI SkeletalNode(void);
		virtual ~SkeletalNode(void) {}

		MINSGAPI std::unordered_map<std::string, AbstractJoint *> &getJointMap();

		/****************************************************************
		 * Animation related.
		 ****************************************************************/
		MINSGAPI void playAnimation(const std::string & name, float duration, bool force=true);

		MINSGAPI AnimationBehaviour * getAnimation(const std::string & name);
		MINSGAPI void addAnimation(AnimationBehaviour *ani, const std::string & name);
		
		MINSGAPI bool clearAnimation(std::string _name, BehaviourManager *manager);
		MINSGAPI void clearAnimations(BehaviourManager *manager);

		uint32_t getAnimationCount() { return static_cast<uint32_t>(animations.size()); }
		const std::unordered_map<std::string, AnimationBehaviour *> & getAnimations() { return animations;}

		void setJointMap(std::unordered_map<std::string, AbstractJoint *> map) { jointMap = map; }
		uint32_t getJointMapSize() { return static_cast<uint32_t>(jointMap.size()); }

		MINSGAPI bool splitAnimation(std::string sourceName, std::string targetName, int index);
		MINSGAPI bool separateAnimation(std::string source, std::vector<std::string> names, std::vector<uint32_t> indices, BehaviourManager *manager);

		MINSGAPI void printAnimationNames();
		MINSGAPI std::vector<std::string> getAnimationNames();

		MINSGAPI void startLoop(std::string aniName);
		MINSGAPI void stopLoop(std::string aniName);

		MINSGAPI bool hasAnimation(std::string aniName);
		
		MINSGAPI void validateJointMap();
		
		MINSGAPI bool setStartAnimation(std::string animationName);
		MINSGAPI void setStartAnimationByName(std::string _name);
		
		MINSGAPI bool goToAnimationState(std::string _name);
		
		MINSGAPI Geometry::Matrix4x4 &getInverseWorldMatrix() const;

		/****************************************************************
		 *                      ---|> Node
		 ****************************************************************/
		MINSGAPI virtual void doAddChild(Util::Reference<Node> child) override;

		/****************************************************************
		 *  Debug caller for skeleton. Calls debugMode for all joints.
		 *  Renders a circle foreach joint and a line representing the
		 *  parent -> children representation. 
		 ****************************************************************/
		MINSGAPI void showSkeleton();
		MINSGAPI void hideSkeleton();

		MINSGAPI void hideMesh();
		MINSGAPI void showMesh();
		
		/****************************************************************
		 *  Test functions for rigidjoints. 
		 ****************************************************************/
		MINSGAPI void attachObject(Util::Reference<Node> _object, std::string _id);
		MINSGAPI void detachObject(Util::Reference<Node> _object, std::string _id);
		
		MINSGAPI RigidJoint *getAnchorJoint();
		
	private:
		SkeletalNode(const SkeletalNode &) = default;
		SkeletalNode* doClone()const override {	return new SkeletalNode(*this);	}
	};
}


#endif /* SKELETALNODE_H_ */
#endif /* MINSG_EXT_SKELETAL_ANIMATION */
