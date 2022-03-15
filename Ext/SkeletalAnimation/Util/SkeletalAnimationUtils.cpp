/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "SkeletalAnimationUtils.h"

#include "../SkeletalNode.h"
#include "../Renderer/SkeletalHardwareRendererState.h"
#include "../AnimationBehaviour.h"

#include "../JointPose/AbstractPose.h"

#include "../Joints/ArmatureNode.h"
#include "../Joints/AbstractJoint.h"
#include "../Joints/JointNode.h"
#include "../Joints/RigidJoint.h"

#include "../../../Core/Nodes/GeometryNode.h"
#include "../../../Core/Nodes/ListNode.h"

#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>

#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/Texture/TextureUtils.h>

#include <Util/Graphics/PixelAccessor.h>

#include <vector>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2		1.57079632679489661923
#endif

using namespace Geometry;
using namespace Rendering;
using namespace std;
using namespace Util;

namespace MinSG {

Rendering::Mesh *SkeletalAnimationUtils::getAnimatedMesh(SkeletalNode *source) {
    std::vector<GeometryNode *> geometries = getGeometryListOfSkeleton(source);
    
    std::deque<Mesh *> meshArray;
    for(const auto geometry : geometries)
        meshArray.emplace_back(geometry->getMesh());
    
    return MeshUtils::combineMeshes(meshArray);
}

ArmatureNode *SkeletalAnimationUtils::getArmatureFromSkeletalNode(SkeletalNode *_sourceNode) {
    ArmatureNode *armNode = nullptr;
    
    std::vector<Node *> openList;
    openList.emplace_back(_sourceNode);
    
    while(!openList.empty()) {
        armNode = dynamic_cast<ArmatureNode *>(openList.front());
        if(armNode != nullptr)
            return armNode;
        
        ListNode *list = dynamic_cast<ListNode *>(openList.front());
        openList.erase(openList.begin());
        if(list != nullptr)
            for(uint32_t i=0; i<list->countChildren(); ++i)
                openList.emplace_back(list->getChild(i));
    }
    
    return armNode;
}

Matrix3x3 SkeletalAnimationUtils::getRotationMatrix(Geometry::Matrix4x4 mat)
{
   Geometry::Matrix3x3 rot(mat[0], mat[1], mat[2],
                            mat[4], mat[5], mat[6],
                            mat[8], mat[9], mat[10]);
    
    return rot;
}

bool SkeletalAnimationUtils::decomposeRotationMatrix2(Geometry::Matrix3x3 *source, Geometry::Vec3 angles[2])
{
    if(source == nullptr)
        return false;
    
    angles[0].setValue(0, 0, 0);
    angles[1].setValue(0, 0, 0);

    if(source->at(2, 0) != 1.0 && source->at(2, 0) != -1.0)
    {
        angles[0].setX(-std::asin(source->at(2, 0)));
        angles[1].setX(static_cast<float>(M_PI) - angles[0].x());
        
        angles[0].setY(std::atan2(source->at(2, 1)/std::cos(angles[0].x()), source->at(2, 2)/std::cos(angles[0].x())));
        angles[1].setY(std::atan2(source->at(2, 1)/std::cos(angles[1].x()), source->at(2, 2)/std::cos(angles[1].x())));
        
        angles[0].setZ(std::atan2(source->at(1, 0)/std::cos(angles[0].x()), source->at(0, 0)/std::cos(angles[0].x())));
        angles[1].setZ(std::atan2(source->at(1, 0)/std::cos(angles[1].x()), source->at(0, 0)/std::cos(angles[1].x())));
    }else
    {
        if(source->at(2, 0) == -1.0)
        {
            angles[0].setX(static_cast<float>(M_PI_2));
            angles[1].setX(angles[0].x());
            
            angles[0].setY(angles[0].z() + std::atan2(source->at(0, 1), source->at(0, 2)));
            angles[1].setY(angles[0].y());
        }else
        {
            angles[0].setX(-static_cast<float>(M_PI_2));
            angles[1].setX(angles[0].x());
            
            angles[0].setY(-angles[0].z() + std::atan2(-source->at(0, 1), -source->at(0, 2)));
            angles[1].setY(angles[0].y());
        }
    }
    
    return true;
}

bool SkeletalAnimationUtils::decomposeRotationMatrix(Geometry::Matrix3x3 *source, Geometry::Vec3 *angles, bool testForGimbalLock)
{
    if(source == nullptr)
        return false;
    
    if(angles == nullptr)
        angles = new Geometry::Vec3();
    
    angles->setY(-asin(source->at(0, 2)));
    float C = cos(angles->getY());
    
    if(fabs(angles->getY()) < 0.005 && testForGimbalLock)       // Gimbal Lock ?
    {
        angles->setX(0.0);
        angles->setZ(atan2(source->at(1, 0), source->at(1, 1)));
    }else
    {
        angles->setX(atan2(-source->at(1, 2)/C, source->at(2, 2)/C));
        angles->setZ(atan2(-source->at(0, 1)/C, source->at(0, 0)/C));
    }
    
    return true;
}

void SkeletalAnimationUtils::transformMeshFromWorldSpaceIntoBindSpace(SkeletalNode *root, Geometry::Matrix4x4 bindMatrix)
{
    std::vector<GeometryNode *> geometryNodes = getGeometryListOfSkeleton(root);
    
    for(const auto geoNode : geometryNodes) {
        std::vector<WeightPair> pairs = SkeletalAnimationUtils::getWeightPairs(&geoNode->getMesh()->openVertexData());
        
        float *vertexPosition;
        for(const auto pair : pairs) {
            vertexPosition = pair.link;
            Geometry::Vec4 positionVec(vertexPosition[0], vertexPosition[1], vertexPosition[2], 1.0);
            positionVec = bindMatrix * positionVec;
            vertexPosition[0] = positionVec[0];
            vertexPosition[1] = positionVec[1];
            vertexPosition[2] = positionVec[2];
        }
        
        geoNode->getMesh()->openVertexData().markAsChanged();
        geoNode->getMesh()->openVertexData().updateBoundingBox();
    }
}

void SkeletalAnimationUtils::convertJointNodesToRigidNodes(ArmatureNode *armature, bool validateRigids)
{
    deque<AbstractJoint *> openList;
    
    for(uint32_t i=0; i<armature->countChildren(); ++i)
        if(dynamic_cast<JointNode *> (armature->getChild(i)) != nullptr)
            openList.emplace_back(dynamic_cast<JointNode *> (armature->getChild(i)));
    
    while(!openList.empty())
    {
        JointNode *old = dynamic_cast<JointNode *> (openList.back());
        openList.pop_back();
        
        if(old == nullptr)
            continue;
        
        auto rigid = new RigidJoint(old);
        rigid->addToParent(old->getParent());
        for(uint32_t i=0; i<old->countChildren(); ++i)
        {
            rigid->doAddChild(old->getChild(i));
            JointNode *child = dynamic_cast<JointNode *> (old->getChild(i));
            if(child != nullptr)
                openList.emplace_back(child);
        }
        old->removeFromParent();
    }
    
    if(validateRigids)
        if(dynamic_cast<SkeletalNode *> (armature->getParent()) != nullptr)
            (dynamic_cast<SkeletalNode *> (armature->getParent()))->validateJointMap();
}

SkeletalNode * SkeletalAnimationUtils::getSkeletalNode(Node *child){
    if(child == nullptr || child->getParent() == nullptr)
        return nullptr;
    
    Node *curNode = child;
    
    uint32_t maxSteps = 10000;
    while(maxSteps > 0) {
        if(dynamic_cast<SkeletalNode*>(curNode->getParent()) != nullptr)
            return dynamic_cast<SkeletalNode*>(curNode->getParent());
        
        curNode = curNode->getParent();
        maxSteps--;
    }
    
    return nullptr;
}

std::vector<float *> SkeletalAnimationUtils::getDirectlyAffectedVerticesByJoint(AbstractJoint *joint)
{
    vector<float *> vertexPositionList;
    
    SkeletalNode *root = getSkeletalNode(joint);
    if(root == nullptr)
        return vertexPositionList;
    
    vector<GeometryNode *> geometryNodes = getGeometryListOfSkeleton(root);
    
    for(auto geoNode : geometryNodes) {
        std::vector<WeightPair> weightPairs = getWeightPairs(&geoNode->getMesh()->openVertexData());
        
    }
    
    return vertexPositionList;
}

std::vector<AbstractJoint *> *SkeletalAnimationUtils::collectAllJoints(ArmatureNode *source) {
    auto jointList = new std::vector<AbstractJoint *>();
    
    if(source == nullptr)
        return jointList;
    
    if(source->countChildren() != 1)
        return jointList;
    
    if(dynamic_cast<AbstractJoint *>(source->getChild(0)) == nullptr)
        return jointList;
    
    std::vector<AbstractJoint *> openList;
    AbstractJoint *child = dynamic_cast<AbstractJoint*>(source->getChild(0));
    openList.emplace_back(child);
    
    while(!openList.empty()) {
        AbstractJoint *joint = openList.front();
        for(uint32_t i=0; i<joint->countChildren(); ++i) {
            child = dynamic_cast<AbstractJoint*>(joint->getChild(i));
            if(child != nullptr)
                openList.emplace_back(child);
        }
        
        jointList->emplace_back(joint);
        openList.erase(openList.begin());
    }
    
    return jointList;
}

std::vector<AbstractJoint *> *SkeletalAnimationUtils::collectAllJointsSortedById(ArmatureNode *source) {
    std::vector<AbstractJoint *> *joints = SkeletalAnimationUtils::collectAllJoints(source);
    if(joints == nullptr || joints->empty())
        return joints;
    
    std::vector<AbstractJoint *> *sortedJoints = new std::vector<AbstractJoint *>(joints->size());
    for(const auto joint : *joints) {
        (*sortedJoints)[joint->getId()] = joint;
    }
    
    return sortedJoints;
}

Geometry::Box SkeletalAnimationUtils::getBoundingBoxOfJoint(AbstractJoint *joint)
{
    SkeletalNode *root = getSkeletalNode(joint);
    if(root == nullptr)
        return Box();
    
    vector<GeometryNode *> geometryNodes = getGeometryListOfSkeleton(root);
    
    Box box;
    
    for(auto geoNode : geometryNodes)
        box.include(getBoundingBoxOfJointForMesh(&geoNode->getMesh()->openVertexData(), joint));
    
    return Geometry::Helper::getTransformedBox(box, joint->getWorldTransformationMatrix().inverse());
}

Geometry::Box SkeletalAnimationUtils::getBoundingBoxOfJointForMesh(Rendering::MeshVertexData *mesh, AbstractJoint *joint)
{
    vector<WeightPair> pairs = getWeightPairs(mesh);
    
    if(pairs.empty())
        return Box();
    
    bool init = true;
    float minx = 0.0;
    float maxx = 0.0;
    float miny = 0.0;
    float maxy = 0.0;
    float minz = 0.0;
    float maxz = 0.0;
    
    uint32_t jointId = joint->getId();
    for(auto pair : pairs)
    {
        for(uint32_t id : pair.nodeIds)
        {
            if(id == jointId)
            {
                if(init)
                {
                    minx = pair.pos.x();
                    maxx = pair.pos.x();
                    miny = pair.pos.y();
                    maxy = pair.pos.y();
                    minz = pair.pos.z();
                    maxz = pair.pos.z();
                    init = false;
                }else
                {
                    minx = fmin(minx, pair.pos.x());
                    maxx = fmax(maxx, pair.pos.x());
                    miny = fmin(miny, pair.pos.y());
                    maxy = fmax(maxy, pair.pos.y());
                    minz = fmin(minz, pair.pos.z());
                    maxz = fmax(maxz, pair.pos.z());
                }
            }
        }
    }
    
    return Box(minx, maxx, miny, maxy, minz, maxz);
}
    
void SkeletalAnimationUtils::removeGeometryFromSkeleton(SkeletalNode *root) {
    vector<ListNode *> openList;
    openList.push_back(root);
    
    while(!openList.empty()) {
        ListNode *node = openList.back();
        openList.pop_back();
        
        if(dynamic_cast<AbstractJoint *>(node) != nullptr)
            continue;
        
        for(uint32_t i=0; i<node->countChildren(); ++i) {
            if(dynamic_cast<GeometryNode *> (node->getChild(i)) != nullptr) {
                node->removeChild(node->getChild(i));
                i--;
            } else {
                ListNode *child = dynamic_cast<ListNode *> (node->getChild(i));
                if(child != nullptr)
                    openList.emplace_back(child);
            }
        }
    }
}

vector<GeometryNode *> SkeletalAnimationUtils::getGeometryListOfSkeleton(SkeletalNode *root)
{
    deque<Node *> openList;
    vector<GeometryNode *> geoList;
    openList.push_front(root);
    while(!openList.empty())
    {
        if(dynamic_cast<ArmatureNode *>(openList.front()) == nullptr)
        {
            ListNode *listNode = dynamic_cast<ListNode *>(openList.front());
            if(listNode != nullptr)
            {
                for(uint32_t i=0; i<listNode->countChildren(); ++i)
                    openList.emplace_back(listNode->getChild(i));
            }
            else 
            {
                GeometryNode *geoNode = dynamic_cast<GeometryNode *>(openList.front());
                if(geoNode != nullptr)
                    geoList.emplace_back(geoNode);
            }
        }
        openList.pop_front();
    }
    
    return geoList;
}

bool SkeletalAnimationUtils::generateUniformTexture(const Matrix4x4 bindMatrix, std::vector<Matrix4x4> invBindMatrix, const std::vector<Matrix4x4> jointMatrix, Reference<PixelAccessor> *pa)
{    
    Vec4 data;
    data[0] = static_cast<float>(invBindMatrix.size());
    data[1] = static_cast<float>(jointMatrix.size());
    data[2] = 9.0f; // inverse bind matrix index in array
    data[3] = static_cast<float>(9 + invBindMatrix.size()*4); // joint index in array    
    putVec4InTexture(0, data, pa);
    
    if(!putMatrixInTexture(1, bindMatrix, pa))
        return false;
    
    if(!putMatricesInTexture(9, invBindMatrix, pa))
        return false;
    
    if(!putMatricesInTexture(static_cast<uint32_t>(9+invBindMatrix.size()*4), jointMatrix, pa))
        return false;
    
    return true;
}

bool SkeletalAnimationUtils::putMatrixInTexture(const uint32_t offset, const Geometry::Matrix4x4 & matrix, Util::Reference<Util::PixelAccessor> *pa)
{
    for(uint32_t i=0; i<4; ++i)
        if(!putVec4InTexture(offset+i, Vec4(matrix.at(i*4), matrix.at(i*4+1), matrix.at(i*4+2), matrix.at(i*4+3)), pa))
            return false;
    
    return true;
}

bool SkeletalAnimationUtils::putMatricesInTexture(const uint32_t offset, const std::vector<Geometry::Matrix4x4> matrices, Reference<PixelAccessor> *pa)
{    
    for(uint32_t i=0; i<matrices.size(); ++i)
        if(!putMatrixInTexture(offset+i*4, matrices[i], pa))
            return false;
    
    return true;
}

bool SkeletalAnimationUtils::putVec4InTexture(const uint32_t offset, const Geometry::Vec4 vector, Util::Reference<Util::PixelAccessor> *pa)
{
    if(pa->get() == nullptr)
        return false;
    if(pa->get()->getBitmap()->getPixelFormat().getDataType() != Util::TypeConstant::FLOAT)
        return false;
    
    float *data = pa->get()->_ptr<float>(offset, 0);    
    data[0] = vector[0];
    data[1] = vector[1];
    data[2] = vector[2];
    data[3] = vector[3];
    
    return true;
}

void SkeletalAnimationUtils::normalizeAnimationDuration(AnimationBehaviour &animation) {
    std::vector<AbstractPose*> &poses = animation.getPoses();
    double duration = animation.getDuration();
    double maxTime = 0;
    for(auto pose : poses) {
        std::deque<double> timeline = pose->getTimeline();
        for(uint32_t i=1; i<timeline.size(); ++i) {
            timeline[i] = timeline[i] / duration;
            maxTime = std::max(maxTime, timeline[i]);
        }
        pose->setTimeline(timeline);
    }
    
    animation.setMaxTime(maxTime);
}

bool SkeletalAnimationUtils::writeDataIntoMesh(Rendering::MeshVertexData *mesh, std::vector<WeightPair> *pairs)
{
    if(mesh->getVertexCount() != pairs->size())
        return false;
    
    VertexDescription vd = mesh->getVertexDescription();
    if(!vd.hasAttribute("sg_WeightsCount"))
        appendSkeletanDescriptionToMesh(mesh);
    
    const VertexAttribute &  weightAttr1 = vd.getAttribute("sg_Weights1");
    const VertexAttribute &  weightAttr2 = vd.getAttribute("sg_Weights2");
    const VertexAttribute &  weightAttr3 = vd.getAttribute("sg_Weights3");
    const VertexAttribute &  weightAttr4 = vd.getAttribute("sg_Weights4");
    
    const VertexAttribute &  weightAttrIndex1 = vd.getAttribute("sg_WeightsIndex1");
    const VertexAttribute &  weightAttrIndex2 = vd.getAttribute("sg_WeightsIndex2");
    const VertexAttribute &  weightAttrIndex3 = vd.getAttribute("sg_WeightsIndex3");
    const VertexAttribute &  weightAttrIndex4 = vd.getAttribute("sg_WeightsIndex4");
    
    const VertexAttribute &  weightAttrCount = vd.getAttribute("sg_WeightsCount");

    float *value;
    for(uint32_t i=0; i<mesh->getVertexCount(); ++i)
    {
        value = reinterpret_cast<float *>((*mesh)[i]+weightAttrCount.getOffset());
        value[0] = static_cast<float>((*pairs)[i].weights.size());
        
        for(uint32_t j=0; j<(*pairs)[i].weights.size(); ++j)
        {
            if(j<4)
            {
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttr1.getOffset());
                value[j] = (*pairs)[i].weights[j];
                
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttrIndex1.getOffset());
                value[j] = static_cast<float>((*pairs)[i].nodeIds[j]);
            }else if(j<8)
            {
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttr2.getOffset());
                value[j-4] = (*pairs)[i].weights[j];
                
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttrIndex2.getOffset());
                value[j-4] = static_cast<float>((*pairs)[i].nodeIds[j]);
            }else if(j<12)
            {
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttr3.getOffset());
                value[j-8] = (*pairs)[i].weights[j];
                
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttrIndex3.getOffset());
                value[j-8] = static_cast<float>((*pairs)[i].nodeIds[j]);
            }else if(j<16)
            {
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttr4.getOffset());
                value[j-12] = (*pairs)[i].weights[j];
                
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttrIndex4.getOffset());
                value[j-12] = static_cast<float>((*pairs)[i].nodeIds[j]);
            }
        }
    }
    
    return true;
}

std::vector<SkeletalAnimationUtils::WeightPair> SkeletalAnimationUtils::getWeightPairs(Rendering::MeshVertexData *mesh)
{
    VertexDescription vd = mesh->getVertexDescription();
    
    if(!vd.hasAttribute("sg_WeightsCount") || !vd.hasAttribute(VertexAttributeIds::POSITION))
        return vector<WeightPair>();
    
    const VertexAttribute & position = vd.getAttribute(VertexAttributeIds::POSITION);
    
    const VertexAttribute &  weightAttr1 = vd.getAttribute("sg_Weights1");
    const VertexAttribute &  weightAttr2 = vd.getAttribute("sg_Weights2");
    const VertexAttribute &  weightAttr3 = vd.getAttribute("sg_Weights3");
    const VertexAttribute &  weightAttr4 = vd.getAttribute("sg_Weights4");
    
    const VertexAttribute &  weightAttrIndex1 = vd.getAttribute("sg_WeightsIndex1");
    const VertexAttribute &  weightAttrIndex2 = vd.getAttribute("sg_WeightsIndex2");
    const VertexAttribute &  weightAttrIndex3 = vd.getAttribute("sg_WeightsIndex3");
    const VertexAttribute &  weightAttrIndex4 = vd.getAttribute("sg_WeightsIndex4");
    
    const VertexAttribute &  weightAttrCount = vd.getAttribute("sg_WeightsCount");
    
    std::vector<WeightPair> pairs;
    float *value;
    float *id;
    for(uint32_t i=0; i<mesh->getVertexCount(); ++i)
    {
        pairs.emplace_back(WeightPair());
        pairs.back().weights = vector<float>();
        pairs.back().nodeIds = vector<uint32_t>();
        const float count = *(reinterpret_cast<float *>((*mesh)[i]+weightAttrCount.getOffset()));
        uint32_t offset = 0;
        for(uint32_t j=0; j<count; ++j)
        {
            if(j<4)
            {
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttr1.getOffset());
                id = reinterpret_cast<float *>((*mesh)[i]+weightAttrIndex1.getOffset());
                offset = 0;
            }
            else if(j<8)
            {
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttr2.getOffset());
                id = reinterpret_cast<float *>((*mesh)[i]+weightAttrIndex2.getOffset());
                offset = 4;
            }
            else if(j<12)
            {
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttr3.getOffset());
                id = reinterpret_cast<float *>((*mesh)[i]+weightAttrIndex3.getOffset());
                offset = 8;
            }
            else
            {
                value = reinterpret_cast<float *>((*mesh)[i]+weightAttr4.getOffset());
                id = reinterpret_cast<float *>((*mesh)[i]+weightAttrIndex4.getOffset());
                offset = 12;
            }
            
            pairs.back().weights.emplace_back(value[j-offset]);
            pairs.back().nodeIds.emplace_back(static_cast<uint32_t>(id[j-offset]));
        }
        
        value = id = reinterpret_cast<float *>((*mesh)[i]+position.getOffset());
        pairs.back().pos = Vec3(value[0], value[1], value[2]);
        pairs.back().link = value;
        pairs.back().vertexId = i;
    }
    
    return pairs;
}

bool SkeletalAnimationUtils::normalizeWeights(MeshVertexData *mesh, float precision)
{    
    vector<WeightPair> pairs = getWeightPairs(mesh);
    
    for(auto pair : pairs)
    {
        float sumWeights = 0.0;
        for(const auto weight : pair.weights)
            sumWeights += weight;
        
        if(fabs(sumWeights - 1.0) > precision)
            for(uint32_t i=0; i<pair.weights.size(); ++i)
                pair.weights[i] = 1.0f/pair.weights.size();
    }
    
    writeDataIntoMesh(mesh, &pairs);
    
    return true;
}



bool SkeletalAnimationUtils::appendSkeletanDescriptionToMesh(MeshVertexData *mesh)
{
    VertexDescription vd = mesh->getVertexDescription();
    std::deque<VertexAttribute> attributes = vd.getAttributes();
    std::deque<std::deque<float>> values;
    
    float *value;
    for(const auto attr : attributes)
    {
        values.emplace_back(std::deque<float>());
        for(uint32_t i=0; i<mesh->getVertexCount(); ++i)
        {
            value = reinterpret_cast<float *>((*mesh)[i]+attr.getOffset());
            for(uint32_t j=0; j<attr.getComponentCount(); ++j)
                values.back().emplace_back(value[j]);
        }
    }
    
    const std::string	 	ATTR_NAME_WEIGHTS1("sg_Weights1");
    const Util::StringIdentifier	ATTR_ID_WEIGHTS1("sg_Weights1");
    const std::string	 	ATTR_NAME_WEIGHTS2("sg_Weights2");
    const Util::StringIdentifier	ATTR_ID_WEIGHTS2("sg_Weights2");
    const std::string	 	ATTR_NAME_WEIGHTS3("sg_Weights3");
    const Util::StringIdentifier	ATTR_ID_WEIGHTS3("sg_Weights3");
    const std::string	 	ATTR_NAME_WEIGHTS4("sg_Weights4");
    const Util::StringIdentifier	ATTR_ID_WEIGHTS4("sg_Weights4");
    
    const std::string	 	ATTR_NAME_WEIGHTSINDEX1("sg_WeightsIndex1");
    const Util::StringIdentifier	ATTR_ID_WEIGHTSINDEX1("sg_WeightsIndex1");
    const std::string	 	ATTR_NAME_WEIGHTSINDEX2("sg_WeightsIndex2");
    const Util::StringIdentifier	ATTR_ID_WEIGHTSINDEX2("sg_WeightsIndex2");
    const std::string	 	ATTR_NAME_WEIGHTSINDEX3("sg_WeightsIndex3");
    const Util::StringIdentifier	ATTR_ID_WEIGHTSINDEX3("sg_WeightsIndex3");
    const std::string	 	ATTR_NAME_WEIGHTSINDEX4("sg_WeightsIndex4");
    const Util::StringIdentifier	ATTR_ID_WEIGHTSINDEX4("sg_WeightsIndex4");
    
    const std::string	 	ATTR_NAME_WEIGHTSCOUNT("sg_WeightsCount");
    const Util::StringIdentifier	ATTR_ID_WEIGHTSCOUNT("sg_WeightsCount");
    
    vd.appendFloatAttribute(ATTR_ID_WEIGHTS1,4);
    vd.appendFloatAttribute(ATTR_ID_WEIGHTS2,4);
    vd.appendFloatAttribute(ATTR_ID_WEIGHTS3,4);
    vd.appendFloatAttribute(ATTR_ID_WEIGHTS4,4);
    
    if ( vd.getAttribute(ATTR_ID_WEIGHTSINDEX1).isValid() ) {
        WARN("WEIGHTS used multiple times.");
        return false;
    }
    vd.appendFloatAttribute(ATTR_ID_WEIGHTSINDEX1,4);
    vd.appendFloatAttribute(ATTR_ID_WEIGHTSINDEX2,4);
    vd.appendFloatAttribute(ATTR_ID_WEIGHTSINDEX3,4);
    vd.appendFloatAttribute(ATTR_ID_WEIGHTSINDEX4,4);
    
    if ( vd.getAttribute(ATTR_ID_WEIGHTSCOUNT).isValid() ) {
        WARN("WEIGHTS used multiple times.");
        return false;
    }
    vd.appendFloatAttribute(ATTR_ID_WEIGHTSCOUNT,1);

    mesh->allocate(mesh->getVertexCount(), vd);
    for(uint32_t i=0; i<attributes.size(); ++i)
        for(uint32_t j=0; j<mesh->getVertexCount(); ++j)
        {
            value = reinterpret_cast<float *>((*mesh)[j]+attributes[i].getOffset());
            for(uint32_t k=0; k<attributes[i].getComponentCount(); ++k)
                value[k] = values.at(i).at((j*attributes[i].getComponentCount())+k);
        }
    
    return true;
}
        
SkeletalNode *SkeletalAnimationUtils::generateSkeletalNode(GeometryNode *mesh, ArmatureNode * armature, float radius, uint32_t precision)
{
    auto node = new SkeletalNode();
    
    if(mesh->getVertexCount() == 0)
        return node;
    
    std::deque<JointNode *> joints;
    armature->catchJointNodes(&joints);
    
    if(joints.empty())
        return node;
    
    MeshVertexData & vData = mesh->getMesh()->openVertexData();
    if(vData.getVertexCount() == 0)
        return node;
    
    if(!appendSkeletanDescriptionToMesh(&vData))
        return node;
    
    VertexDescription vd = vData.getVertexDescription();   
    
    node->addChild(armature);
    node->addChild(mesh);
    
    const VertexAttribute & position = vd.getAttribute(VertexAttributeIds::POSITION);
    std::deque<Vec3> vertices;
    float *vertexData;
    for(uint32_t i=0; i<mesh->getVertexCount(); ++i)
    {
        vertexData = reinterpret_cast<float *> (vData[i]+position.getOffset());
        vertices.emplace_back(Vec3(vertexData[0], vertexData[1], vertexData[2]));                                            
    }
    
    std::vector<WeightPair> pairs;
    for(uint32_t i=0; i<mesh->getVertexCount(); ++i)
    {        
        std::vector<JointNode *> nodes;
        float xOffset = 0.0;
        float yOffset = 0.0;
        float zOffset = 0.0;
        for(JointNode *joint : joints)
        {
            Vec3 curNodePos = joint->getWorldTransformationMatrix().getColumnAsVec3(3);
            Vec3 parentNodePose = joint->getParent()->getWorldTransformationMatrix().getColumnAsVec3(3);
            if(precision != 0)
            {
                xOffset = (parentNodePose.x() - curNodePos.x())/precision;
                yOffset = (parentNodePose.y() - curNodePos.y())/precision;
                zOffset = (parentNodePose.z() - curNodePos.z())/precision;
            }
            
            for(uint32_t j=0; j<precision; j++)
                if(vertices[i].distance(Vec3(curNodePos.x()+(xOffset*j), curNodePos.y()+(yOffset*j), curNodePos.z()+(zOffset*j))) <= radius)
                {
                    nodes.emplace_back(joint);
                    break;
                }
        }
        
        if(nodes.empty())
        {
            JointNode *closestNode = joints[0];
            for(JointNode *joint : joints)
                if(vertices[i].distance(joint->getWorldTransformationMatrix().getColumnAsVec3(3)) < vertices[i].distance(closestNode->getWorldTransformationMatrix().getColumnAsVec3(3)))
                    closestNode = joint;
            
            nodes.emplace_back(closestNode);
        }
        
        pairs.emplace_back(WeightPair());
        for(const auto & n : nodes)
        {
            pairs.back().weights.emplace_back(1.0f);
            pairs.back().nodeIds.emplace_back(n->getId());
        }
        
        nodes.clear();
    }
    
    writeDataIntoMesh(&vData, &pairs);
    normalizeWeights(&vData, 0.01f);

    node->addState(new SkeletalHardwareRendererState());
    
    return node;
}       

}

#endif
