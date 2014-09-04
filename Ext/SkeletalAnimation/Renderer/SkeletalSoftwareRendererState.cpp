/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "SkeletalSoftwareRendererState.h"
#include "../Joints/AbstractJoint.h"
#include "../SkeletalNode.h"

#include "../../../Core/Nodes/GeometryNode.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexDescription.h>

#include <Geometry/Vec4.h>

namespace MinSG {

SkeletalSoftwareRendererState::SkeletalSoftwareRendererState() : SkeletalAbstractRendererState(), meshSkins() {
    
}
    
SkeletalSoftwareRendererState::SkeletalSoftwareRendererState(const SkeletalSoftwareRendererState &source) : SkeletalAbstractRendererState(source), meshSkins() {
    
}
    
SkeletalSoftwareRendererState *SkeletalSoftwareRendererState::clone()const {
    return new SkeletalSoftwareRendererState(*this);
}

void SkeletalSoftwareRendererState::validateMatriceOrder(Node *node) {
    SkeletalAbstractRendererState::validateMatriceOrder(node);
    
    if(!validatedMatrices)
        return;
    
    std::vector<GeometryNode *> geoNodes;
    geoNodes = SkeletalAnimationUtils::getGeometryListOfSkeleton(rootJoint);
    if(!geoNodes.empty())
        validatedMatrices = true;
    
    for(const auto geoNode : geoNodes) {        
        Rendering::MeshVertexData &vData = geoNode->getMesh()->openVertexData();
        const Rendering::VertexDescription &vd = vData.getVertexDescription();
        
        const Rendering::VertexAttribute &positionAttr = vd.getAttribute("sg_Position");
        const Rendering::VertexAttribute &weightAttrCount = vd.getAttribute("sg_WeightsCount");
        uint16_t weightIndexOffset = vd.getAttribute("sg_WeightsIndex1").getOffset();
        uint16_t weightsOffset = vd.getAttribute("sg_Weights1").getOffset();
        
        meshSkins.emplace_back(std::make_pair(geoNode, std::vector<VertexPair>()));
        for(uint32_t i=0; i<vData.getVertexCount(); ++i) {
            float *weightCount = reinterpret_cast<float *>(vData[i]+weightAttrCount.getOffset());
            if(weightCount[0] < 0.1)
                continue;
            
            float *position = reinterpret_cast<float *>(vData[i]+positionAttr.getOffset());
            VertexPair pair;
            pair.vId = i;
            pair.bindPosition = Geometry::Vec4(position[0], position[1], position[2], 1.0);
            
            float *jointId;
            float *weight;
            for(uint32_t j=0; j<weightCount[0]; ++j) {
                if(j==4) {
                    weightIndexOffset = vd.getAttribute("sg_WeightsIndex2").getOffset();
                    weightsOffset = vd.getAttribute("sg_weights2").getOffset();
                }else if(j==8) {
                    weightIndexOffset = vd.getAttribute("sg_WeightsIndex3").getOffset();
                    weightsOffset = vd.getAttribute("sg_weights3").getOffset();
                }else if(j==12) {
                    weightIndexOffset = vd.getAttribute("sg_WeightsIndex4").getOffset();
                    weightsOffset = vd.getAttribute("sg_weights4").getOffset();
                }
                
                if(j%4 == 0) {
                    jointId = reinterpret_cast<float *>(vData[i]+weightIndexOffset);
                    weight = reinterpret_cast<float *>(vData[i]+weightsOffset);
                }
                
                pair.jointIds.emplace_back(jointId[0]);
                pair.weights.emplace_back(weight[0]);
            }
            
            meshSkins.back().second.emplace_back(pair);
        }
    }
}

State::stateResult_t SkeletalSoftwareRendererState::doEnableState(FrameContext & /*context*/, Node *node, const RenderParam &/*rp*/) {
    if(!validatedMatrices) {
        validateMatriceOrder(node);
        return State::STATE_OK;
    }
    
    jointMats.clear();
    for(const auto joint : matriceOrder) {
        jointMats.emplace_back(joint->getWorldTransformationMatrix());
    }
    
    const Geometry::Matrix4x4 inverse = rootJoint->getWorldTransformationMatrix().inverse();
    for(const auto skin : meshSkins) {
        Rendering::MeshVertexData &vData = skin.first->getMesh()->openVertexData();
        const Rendering::VertexDescription &vd = vData.getVertexDescription();        
        const Rendering::VertexAttribute &positionAttr = vd.getAttribute("sg_Position");

        for(const auto skinInfo : skin.second) {
            float *vertexPosition = reinterpret_cast<float *>(vData[skinInfo.vId]+positionAttr.getOffset());
            Geometry::Vec4 newPosition(0, 0, 0, 0);
            
            for(uint32_t j=0; j<skinInfo.jointIds.size(); ++j) {
                Geometry::Matrix4x4 deformedSkin = inverse * jointMats[skinInfo.jointIds[j]] * inverseMatContainer[skinInfo.jointIds[j]];
                newPosition += deformedSkin * skinInfo.weights[j] * skinInfo.bindPosition;
            }
            
            vertexPosition[0] = newPosition.getX();
            vertexPosition[1] = newPosition.getY();
            vertexPosition[2] = newPosition.getZ();
        }
        
        vData.markAsChanged();
    }

    return State::STATE_OK;
}

void SkeletalSoftwareRendererState::doDisableState(FrameContext & /*context*/, Node */*node*/, const RenderParam &/*rp*/) {

}

}

#endif
