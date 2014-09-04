/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "SkeletalAbstractRendererState.h"

#include "../SkeletalNode.h"

#include "../Joints/AbstractJoint.h"

#include <Geometry/Matrix4x4.h>

using namespace Geometry;

namespace MinSG {
    
SkeletalAbstractRendererState::SkeletalAbstractRendererState(const SkeletalAbstractRendererState &source) : ShaderState(source), 
                                                                            validatedMatrices(false), matriceOrder(), rootJoint(nullptr),
                                                                            bindMatrix(source.getBindMatrix()), debugJointId(source.getDebugJointId()) {

}

SkeletalAbstractRendererState::SkeletalAbstractRendererState() : ShaderState(), validatedMatrices(false), matriceOrder(), rootJoint(nullptr), bindMatrix(Matrix4x4()), debugJointId(-1) {
    
}

void SkeletalAbstractRendererState::setBindMatrix(Geometry::Matrix4x4 _matrix)
{
    bindMatrix = Matrix4x4(_matrix);
}

void SkeletalAbstractRendererState::setBindMatrix(const float _matrix[])
{
    setBindMatrix(Matrix4x4(_matrix));
}

void SkeletalAbstractRendererState::setBindMatrix(std::vector<float> _matrix)
{
	setBindMatrix(&_matrix[0]);
}

void SkeletalAbstractRendererState::validateMatriceOrder(Node *node) {
    
    matriceOrder.clear();
    rootJoint = dynamic_cast<SkeletalNode *>(node);
    if(rootJoint == nullptr)
        return;
    
    if(rootJoint->getJointMapSize() < 1)
        return;

    std::unordered_map<std::string, AbstractJoint *> &jMap = rootJoint->getJointMap();
    if(jMap.empty())
        return;
    
    for(const auto item : jMap)
    {
        bool added = false;
        for(uint32_t i=0; i<matriceOrder.size(); ++i)
        {
            if(item.second->getId() < matriceOrder[i]->getId())
            {
                matriceOrder.insert(matriceOrder.begin()+i, item.second);
                added = true;
                break;
            }
        }
        if(!added)
            matriceOrder.push_back(item.second);
    }
    
    jointMats.clear();
    for(const auto item : matriceOrder) {
        jointMats.emplace_back(item->getWorldTransformationMatrix());
        inverseMatContainer.emplace_back(item->getInverseBindMatrix());
    }
    
    jointSize = inverseMatContainer.size();

    validatedMatrices = true;
}

}

#endif
