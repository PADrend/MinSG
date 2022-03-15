/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "SkeletalNode.h"

#include "AnimationBehaviour.h"
#include "Joints/ArmatureNode.h"
#include "Joints/JointNode.h"
#include "Joints/RigidJoint.h"
#include "Renderer/SkeletalHardwareRendererState.h"
#include "Util/SkeletalAnimationUtils.h"

#include "../../Core/Behaviours/BehaviourManager.h"
#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/GeometryNode.h"

#include <stdexcept>


namespace MinSG {
    using namespace std;
    
    SkeletalNode::SkeletalNode() : startAnimation(nullptr), currentAnimation(nullptr), jointMap(), inverseWorldMatrix(getWorldTransformationMatrix().inverse())
    {

    }
    
    bool SkeletalNode::goToAnimationState(std::string _name)
    {
        if(animations.find(_name) == animations.end())
            return false;
        
        auto tracker = new std::vector<AnimationBehaviour *>();
        currentAnimation->goToAnimationState(_name, tracker);
        
        if(tracker->empty())
            return false;
        else
        {
            animationTracker = *tracker;
            return true;
        }
    }
    
    void SkeletalNode::startLoop(string aniName)
    {
        if(animations.find(aniName) != animations.end())
            animations[aniName]->startLoop();
    }
    
    void SkeletalNode::stopLoop(string aniName)
    {
        if(animations.find(aniName) != animations.end())
            animations[aniName]->stopLoop();
    }
    
    bool SkeletalNode::hasAnimation(string aniName)
    {
        if(animations.find(aniName) != animations.end())
            return true;
        else
            return false;
    }
    
    std::unordered_map<std::string, AbstractJoint *> &SkeletalNode::getJointMap()
    {
        if(jointMap.empty())
            validateJointMap();
        
        return jointMap;
    }
    
    void SkeletalNode::validateJointMap()
    {        
        jointMap.clear();
        for(uint32_t i=0; i<countChildren(); ++i)
        {
            ArmatureNode *child = dynamic_cast<ArmatureNode *>(getChild(i));
            if(child != nullptr)
            {
                child->generateJointNodeMap(jointMap);
            }
        }
        
        if(getStateListPtr() == nullptr)
            return;
        
        for(const auto state : *getStateListPtr())
        {
            SkeletalHardwareRendererState *renderer = dynamic_cast<SkeletalHardwareRendererState *> (state.first.get());
            if(renderer != nullptr)
            {
                renderer->validateMatriceOrder(this);
                break;
            }
        }
    }
    
    unordered_map<string, AbstractJoint *> SkeletalNode::generateJointMap(SkeletalNode *object)
    {
        unordered_map<string, AbstractJoint *> jMap;
        
        for(uint32_t i=0; i<object->countChildren(); ++i)
        {
            JointNode *joint = dynamic_cast<JointNode *> (object->getChild(i));
            if(joint != nullptr)
            {
                jMap[joint->getName()] = joint;
                joint->generateJointNodeMap(jMap);
            }
        }
        
        return jMap;
    }
    
//    SkeletalNode *SkeletalNode::doClone() const
//    {
//        auto myclone = new SkeletalNode();
//        
//        for(uint32_t i=0; i<countChildren(); ++i)
//        {
//            if(dynamic_cast<AbstractJoint *> (getChild(i)) != nullptr)
//            {
//                AbstractJoint *joint = (dynamic_cast<AbstractJoint *>(getChild(i)))->clone();
//                myclone->addChild(joint);
//            }
//            else
//                myclone->addChild(getChild(i)->clone());
//        }
//        
//        for(const auto animation : animations)
//        {
//            AnimationBehaviour *ani = animation.second->clone(myclone);
//            myclone->addAnimation(ani, animation.first);
//            if(startAnimation != nullptr)
//                if(ani->getName() == startAnimation->getName())
//                    myclone->setStartAnimation(ani->getName());
//        }
//        
//        const Node::stateList_t *stateList = getStateListPtr();
//        if(stateList != nullptr)
//            for(auto & state : *stateList)
//                myclone->addState(state.first.get()->clone());
//        
//        return myclone;
//    }
    
    bool SkeletalNode::separateAnimation(string source, vector<string> names, vector<uint32_t> indices, BehaviourManager *manager)
    {
        if(animations.find(source) == animations.end())
            return false;
        
        for(uint32_t i=0; i<names.size()-1; ++i)
            for(uint32_t j=i+1; j<names.size(); ++j)
                if(names[i] == names[j])
                    return false;
        
        vector<AnimationBehaviour *> anis = animations[source]->separate(indices, names);
        
        if(names.size() != anis.size())
            return false;
        
        for(auto animation : anis)
        {
            addAnimation(animation, animation->getName());
            manager->registerBehaviour(animation);
        }
        
        return true;
    }
    
    bool SkeletalNode::splitAnimation(string sourceName, string targetName, int index)
    {
        if(animations.find(sourceName) == animations.end())
            return false;
        
        if(animations.find(targetName) != animations.end())
            return false;
        
        vector<AnimationBehaviour *> anis = animations[sourceName]->split(targetName, index);
        if(anis.size() == 2)
        {
            animations.erase(sourceName);
            animations[sourceName] = anis[0];
            animations[targetName] = anis[1];
            
            return true;
        }
        
        return false;
    }
    
    void SkeletalNode::attachObject(Util::Reference<Node> _object, std::string _id)
    {
        RigidJoint *anchor = nullptr;
        
        for(auto node : jointMap)
        {
            anchor = dynamic_cast<RigidJoint *> (node.second);
            if(anchor == nullptr)
                continue;
            
            if(anchor->getName() == _id)
            {
                anchor->doAddChild(_object);
                break;
            }
        }
    }
    
    void SkeletalNode::detachObject(Util::Reference<Node> _object, std::string _id)
    {
        RigidJoint *anchor = nullptr;
        
        for(auto node : jointMap)
        {
            anchor = dynamic_cast<RigidJoint *> (node.second);
            if(anchor == nullptr)
                continue;
            
            if(anchor->getName() == _id)
            {
                anchor->removeChild(_object);
                break;
            }
        }
    }
    
    void SkeletalNode::doAddChild(Util::Reference<Node> child)
    {
        ListNode::doAddChild(child);
        // only unique ids are allowed!
        JointNode *joint = dynamic_cast<JointNode *>(child.get());
        if(joint != nullptr)
            if(jointMap.find(joint->getName()) != jointMap.end()){
                ListNode::removeChild(child);
                throw std::invalid_argument("only unique ids are allowed");
            }
        
        if(dynamic_cast<ArmatureNode*>(child.get()) != nullptr)
            validateJointMap();
    }
    
    bool SkeletalNode::clearAnimation(std::string _name, BehaviourManager *manager)
    {
        const auto animationPosition = animations.find(_name);
        if(animationPosition == animations.end())
            return false;
        
        animationPosition->second->stop();
        animationPosition->second->finalize();
        manager->removeBehaviour(animationPosition->second);
        
        animations.erase(animationPosition);
        
        return true;
    }
    
    void SkeletalNode::clearAnimations(BehaviourManager *manager)
    {                                 
        for(const auto animation : animations)
        {
            animation.second->finalize();
            manager->removeBehaviour(animation.second);
        }
        
        animations.clear();
        startAnimation = nullptr;
        currentAnimation = nullptr;
        animationTracker.clear();
    }
    
    void SkeletalNode::printAnimationNames()
    {
        for(const auto name : getAnimationNames())
            cout << name << endl;
    }
    
    vector<string> SkeletalNode::getAnimationNames()
    {
        vector<string> names;
        for(const auto animation : animations)
            names.push_back(animation.first);
        
        return names;
    }
    
    AnimationBehaviour * SkeletalNode::getAnimation(const string & name)
    {
        auto it = animations.find(name);
        if(it == animations.end())
            return nullptr;
        else
            return animations.find(name)->second;
    }
    
    bool SkeletalNode::setStartAnimation(std::string animationName)
    {
        auto aniIt = animations.find(animationName);
        if(aniIt != animations.end())
        {
            startAnimation = aniIt->second;
            return true;
        }
        
        return false;
    }
    
    void SkeletalNode::setStartAnimationByName(std::string _name) {
        auto aniIt = animations.find(_name);
        if(aniIt != animations.end())
            startAnimation = aniIt->second;

    }
    
    void SkeletalNode::addAnimation(AnimationBehaviour *ani, const string & name)
    {        
        ani->setNode(this);
        
        animations[name] = ani;       
    }
    
    void SkeletalNode::playAnimation(const string & name, float duration, bool force)
    {
        if(duration < 0.0)
            return;
        
        if(force)
            for(const auto animation : animations)
                if(animation.first == name)
                {
                    if(currentAnimation != nullptr)
                        if(currentAnimation->isPlaying())
                            currentAnimation->stop();
                    
                    animation.second->setAnimationSpeed(animation.second->getMaxTime()/duration);
                    animation.second->play();
                    
                    currentAnimation = animation.second;
                    return;
                }
        
        if(currentAnimation == nullptr)
        {
            if(startAnimation != nullptr)
                currentAnimation = startAnimation;
            else
                return;
        }

        if(goToAnimationState(name))
        {
            for(auto it=animationTracker.begin(); it != animationTracker.end(); ++it)
                if((it+1) != animationTracker.end())
                    (*it)->setNextAnimation(*(it+1));
                
            if(animationTracker.empty())
                return;
            
            float sumTimes = 0.0f;
            for(auto animation : animationTracker)
                sumTimes += static_cast<float>(animation->getMaxTime());
            
            for(auto animation : animationTracker)
                animation->setAnimationSpeed((animation->getMaxTime() / sumTimes) * duration);
            
            animationTracker[0]->play();
        }else
            WARN("Animation "+name+" not found!");
    }    
}

#endif /* MINSG_EXT_SKELETAL_ANIMATION */
