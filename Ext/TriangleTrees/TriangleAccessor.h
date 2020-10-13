/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef TRIANGLEACCESSOR_H_
#define TRIANGLEACCESSOR_H_

#include <algorithm>
#include <array>
#include <cstdint>

namespace Geometry {
template<typename T_> class _Vec3;
template<typename T_> class Triangle;
typedef Triangle<_Vec3<float>> Triangle_f;
}
namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace TriangleTrees {

/**
 * Reference to a triangle inside a mesh.
 *
 * @author Benjamin Eikel
 * @date 2011-07-25
 */
class TriangleAccessor {
	public:
		/**
		 * Create a reference to the triangle specified by the given index inside the given mesh.
		 *
		 * @param _mesh Mesh containing the referenced triangle.
		 * @param _triangleIndex Index of the triangle inside the given mesh.
		 */
		MINSGAPI TriangleAccessor(const Rendering::Mesh * _mesh, uint32_t _triangleIndex);

		/**
		 * Return the minimum coordinate of all three vertices in the
		 * requested dimension.
		 *
		 * @param dim Dimension of coordinate.
		 * @return Minimum coordinate value.
		 */
		float getMin(uint_fast8_t dim) const {
			return std::min(vertexPositions[0][dim], std::min(vertexPositions[1][dim], vertexPositions[2][dim]));
		}

		/**
		 * Return the maximum coordinate of all three vertices in the
		 * requested dimension.
		 *
		 * @param dim Dimension of coordinate.
		 * @return Maximum coordinate value.
		 */
		float getMax(uint_fast8_t dim) const {
			return std::max(vertexPositions[0][dim], std::max(vertexPositions[1][dim], vertexPositions[2][dim]));
		}

		/**
		 * Return the data pointer of the requested vertex.
		 *
		 * @param num Number of vertex in {0, 1, 2}
		 * @return Beginning of vertex data
		 */
		MINSGAPI const uint8_t * getVertexData(unsigned char num) const;

		/**
		 * Return the coordinates of the requested vertex.
		 *
		 * @param num Number of vertex in {0, 1, 2}
		 * @return Coordinates of vertex as three values
		 */
		const float * getVertexPosition(unsigned char num) const {
			return vertexPositions[num];
		}

		/**
		 * Return a triangle structure that can be used for calculations.
		 * 
		 * @return Geometric description of the triangle
		 */
		MINSGAPI Geometry::Triangle_f getTriangle() const;

		/**
		 * Return the index of the triangle inside the mesh.
		 * 
		 * @return The index of the triangle inside the mesh.
		 */
		uint32_t getTriangleIndex() const {
			return triangleIndex;
		}

	private:
		//! Mesh that contains the referenced triangle.
		const Rendering::Mesh * mesh;
		//! Index of the triangle inside the mesh.
		uint32_t triangleIndex;
		//! Cached pointers to the location of the three vertex positions.
		std::array<const float *, 3> vertexPositions;
};

}
}

#endif /* TRIANGLEACCESSOR_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
