/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef PADrend_SkeletalAnimationUtils_h
#define PADrend_SkeletalAnimationUtils_h

#include <vector>
#include <Geometry/Vec3.h>
#include <Geometry/Vec4.h>
#include <Util/Graphics/PixelAccessor.h>

namespace MinSG {
    class SkeletalNode;
    class ArmatureNode;
    class GeometryNode;
    class AnimationBehaviour;
    class AbstractJoint;
    class Node;
}

namespace Rendering {
    class Mesh;
    class MeshVertexData;
}

namespace Geometry {
    template<typename _T> class _Matrix4x4;
    typedef _Matrix4x4<float> Matrix4x4;
    
    template<typename _T> class _Matrix3x3;
    typedef _Matrix3x3<float> Matrix3x3;
    
    template<typename value_t> class _Box;
    typedef _Box<float> Box;
}


namespace MinSG {
    namespace SkeletalAnimationUtils
    {
        struct WeightPair
        {
            Geometry::Vec3 pos;
            std::vector<float> weights;
            std::vector<uint32_t> nodeIds;
            uint32_t vertexId;
            float *link;
        };
        
        bool appendSkeletanDescriptionToMesh(Rendering::MeshVertexData * mesh);
        
        std::vector<WeightPair> getWeightPairs(Rendering::MeshVertexData *mesh);
        
        SkeletalNode * getSkeletalNode(Node *child);
        std::vector<GeometryNode *> getGeometryListOfSkeleton(SkeletalNode *root);
        void removeGeometryFromSkeleton(SkeletalNode *root);
        ArmatureNode *getArmatureFromSkeletalNode(SkeletalNode *_sourceNode);
        std::vector<AbstractJoint *> *collectAllJoints(ArmatureNode *source);
        std::vector<AbstractJoint *> *collectAllJointsSortedById(ArmatureNode *source);
        
        Rendering::Mesh *getAnimatedMesh(SkeletalNode *source);
        
        SkeletalNode *generateSkeletalNode(GeometryNode *mesh, ArmatureNode *armature, float radius, uint32_t precision);
        
        void normalizeAnimationDuration(AnimationBehaviour &animation);
        
        bool normalizeWeights(Rendering::MeshVertexData *mesh, float precision);
        
        bool writeDataIntoMesh(Rendering::MeshVertexData *mesh, std::vector<WeightPair> *pairs);
        
        bool generateUniformTexture(const Geometry::Matrix4x4 bindMatrix, const std::vector<Geometry::Matrix4x4> invBindMatrix, const std::vector<Geometry::Matrix4x4> jointMatrix, Util::Reference<Util::PixelAccessor> *pa);
        bool putMatrixInTexture(uint32_t offset, const Geometry::Matrix4x4 & jointMatrix, Util::Reference<Util::PixelAccessor> *pa);
        bool putMatricesInTexture(uint32_t offset, const std::vector<Geometry::Matrix4x4> jointMatrix, Util::Reference<Util::PixelAccessor> *pa);
        
        bool putVec4InTexture(uint32_t offset, const Geometry::Vec4 vector, Util::Reference<Util::PixelAccessor> *pa);
        
        void convertJointNodesToRigidNodes(ArmatureNode *armature, bool validateRigids=true);
        
        Geometry::Matrix3x3 getRotationMatrix(Geometry::Matrix4x4 mat);
        
        void transformMeshFromWorldSpaceIntoBindSpace(SkeletalNode *root, Geometry::Matrix4x4 bindMatrix);
        
        /*
         * Bounding box related utilities
         */
         Geometry::Box getBoundingBoxOfJoint(AbstractJoint *joint);
         Geometry::Box getBoundingBoxOfJointForMesh(Rendering::MeshVertexData *mesh, AbstractJoint *joint);
        
         std::vector<float *> getDirectlyAffectedVerticesByJoint(AbstractJoint *joint);
        
        /*
         *  Decompose a rotation Matrix to Euler X, Y, Z rotation. 
         *  source: http://www.flipcode.com/documents/matrfaq.html#Q37 
         *  There are at least two solution to represent the rotation (clockwise & countClockwise). This function does not take care of it.
         */
         bool decomposeRotationMatrix(Geometry::Matrix3x3 *source, Geometry::Vec3 *angles, bool testForGimbalLock=true);
        
        /*
         * source:
         * Computing Euler angles from a rotation matrix.
         * Author: Gregory G. Slabaugh
         *
         * returns both possible rotations representing by a 3x3 matrix
         *
         * Programmed: Lukas Kopecki
         */
         bool decomposeRotationMatrix2(Geometry::Matrix3x3 *source, Geometry::Vec3 *angles);
    }
}


#endif
#endif
