/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#ifndef MINSG_SVS_HELPER_H_
#define MINSG_SVS_HELPER_H_

#include "Definitions.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Geometry {
template<typename T_> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4f;
template<typename number_t> class _Sphere;
typedef _Sphere<float> Sphere_f;
template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3f;
template<typename _T> class _Vec4;
}
namespace Rendering {
class Texture;
}
namespace MinSG {
class AbstractCameraNode;
class CameraNodeOrtho;
class GroupNode;
class Node;
namespace SVS {
class PreprocessingContext;
class VisibilitySphere;

/**
 * Create an orthographic camera, whose frustum fully contains the given sphere, and which uses the given resolution in pixels.
 * 
 * @param sphere Geometric representation of the sphere surface in local coordinates
 * @param worldMatrix Transformation matrix to convert local to world coordinates
 * @param resolution Width and height of the viewport in pixels
 * @return Orthographic camera
 */
MINSGAPI CameraNodeOrtho * createSamplingCamera(const Geometry::Sphere_f & sphere, const Geometry::Matrix4x4f & worldMatrix, int resolution);

/**
 * Transform the given camera so that it is located on the sphere surface at the given position looking at the center of the sphere.
 *
 * @param camera Camera that will be transformed
 * @param sphere Geometric representation of the sphere surface in local coordinates
 * @param worldMatrix Transformation matrix to convert local to world coordinates
 * @param position Position specified by a unit vector from the center to the surface of the unit sphere
 */
MINSGAPI void transformCamera(AbstractCameraNode * camera, const Geometry::Sphere_f & sphere, const Geometry::Matrix4x4f & worldMatrix, const Geometry::Vec3f & position);

/**
 * Create a color texture from the sample values of the given visibility sphere.
 * The values will be scaled to the range [0, 255] and encoded into the green channel of the texture.
 *
 * @param width Width in pixels of the texture
 * @param height Height in pixels of the texture
 * @param visibilitySphere Sampling sphere containing sample values
 * @param interpolation Type of interpolation to use when requesting values between sample positions
 * @return Color texture containing the encoded values
 */
MINSGAPI Rendering::Texture * createColorTexture(uint32_t width, uint32_t height, const VisibilitySphere & visibilitySphere, interpolation_type_t interpolation);

/**
 * Perform the preprocessing for the given node.
 * First it is checked if the node already has a visibility sphere.
 * If that visibility sphere is valid, the preprocessing is skipped for the node.
 * If it is invalid, the visibility sphere is removed.
 * Then, if the node is without a visibility sphere, a new one is created.
 * When the function returns, the node has a valid visibility sphere.
 *
 * @param preprocessingContext Context object holding required data (e.g. 
 * scene manager, frame context, resolution, sample positions)
 * @param node Node to do the sampling for
 */
MINSGAPI void preprocessNode(PreprocessingContext & preprocessingContext,
					GroupNode * node);

/**
 * Calculate a sphere for the given node, and do a sampling run for the given positions.
 * A VisibilitySphere is created, and stored as attribute at the given node.
 *
 * @param preprocessingContext Context object holding required data (e.g. 
 * scene manager, frame context, resolution, sample positions)
 * @param node Node to do the sampling for
 */
MINSGAPI void createVisibilitySphere(PreprocessingContext & preprocessingContext,
						  GroupNode * node);

/**
 * Check if the given visibility sphere is valid.
 * An invalid visibility sphere contains no samples, contains samples without, or has been cloned.
 * 
 * @param node Root node of the subtree that the visibility sphere is stored at
 * @param visibilitySphere Sampling sphere to check
 * @return @c true if the visibility sphere is valid, @c false otherwise
 */
MINSGAPI bool isVisibilitySphereValid(GroupNode * node, const VisibilitySphere & visibilitySphere);

/**
 * Check if a visibility sphere is stored at the node.
 * 
 * @param node Inner node of a tree structure
 * @return @c true if there is a visibility sphere, @c false otherwise
 */
MINSGAPI bool hasVisibilitySphere(GroupNode * node);

/**
 * Retrieve a visibility sphere from a node.
 *
 * @param node Inner node of a tree structure
 * @return Sampling sphere stored at the given node
 * @throw std::logic_error If the attribute was not found, or has wrong type
 */
MINSGAPI const VisibilitySphere & retrieveVisibilitySphere(GroupNode * node);

/**
 * Store a visibility sphere at a node.
 *
 * @param node Inner node of a tree structure
 * @param visibilitySphere Sampling sphere that will be stored at the given node
 * @throw std::logic_error If the attribute did already exist
 */
MINSGAPI void storeVisibilitySphere(GroupNode * node, VisibilitySphere && visibilitySphere);

/**
 * Remove all visibility spheres stored at nodes on the path from the given node to the root.
 * 
 * @param node Beginning of the path
 */
MINSGAPI void removeVisibilitySphereUpwards(GroupNode * node);

/**
 * Change the coordinate system from world coordinates to local coordinates for
 * all spheres in the given subtree.
 * 
 * @param rootNode Root node of the subtree
 * @throw std::logic_error In case of an error
 */
MINSGAPI void transformSpheresFromWorldToLocal(GroupNode * rootNode);

/**
 * Transform the center and radius of a sphere.
 * 
 * @param sphere Sphere that is transformed
 * @param matrix Matrix specifying the transformation
 * @return Transformed sphere
 * @note To receive valid results, the transformation should be a combination
 * of translation, rotation, and scaling only. Other transformations, e.g.
 * shearing, might lead to invalid results, because the transformed sphere is
 * no sphere anymore.
 */
template<typename number_t>
Geometry::_Sphere<number_t> transformSphere(const Geometry::_Sphere<number_t> & sphere,
											const Geometry::_Matrix4x4<number_t> & matrix) {
	const number_t newRadiusX = (matrix * Geometry::_Vec4<number_t>(sphere.getRadius(), 0, 0, 0)).length();
	const number_t newRadiusY = (matrix * Geometry::_Vec4<number_t>(0, sphere.getRadius(), 0, 0)).length();
	const number_t newRadiusZ = (matrix * Geometry::_Vec4<number_t>(0, 0, sphere.getRadius(), 0)).length();
	return Geometry::_Sphere<number_t>(matrix.transformPosition(sphere.getCenter()), std::max(newRadiusX, std::max(newRadiusY, newRadiusZ)));
}

/**
 * Convert the value of an integer to an enumerator.
 *
 * @param number Value that can be converted to an enumerator
 * @throw std::invalid_argument If the conversion of an unknown value is detected
 * @return Interpolation type enumerator
 */
MINSGAPI interpolation_type_t interpolationFromUInt(uint32_t number);

/**
 * Convert the value of an enumerator to a string.
 *
 * @param interpolation Value that will be converted to a string
 * @throw std::invalid_argument If the conversion of an unknown value is detected
 * @return Human-readable string
 */
MINSGAPI std::string interpolationToString(interpolation_type_t interpolation);

/**
 * Convert the value of a string to an enumerator.
 *
 * @param str Value that can be converted to an enumerator
 * @throw std::invalid_argument If the conversion of an unknown value is detected
 * @return Interpolation type enumerator
 */
MINSGAPI interpolation_type_t interpolationFromString(const std::string & str);

}
}

#endif /* MINSG_SVS_HELPER_H_ */

#endif /* MINSG_EXT_SVS */
