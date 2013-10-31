/*
	This file is part of the MinSG library extension Triangulation.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGULATION

#include "Delaunay3d.h"
#include <cstdint>
#include <tuple>
#include <vector>
#include <Util/Macros.h>

COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wredundant-decls)
#include <detri.h>
COMPILER_WARN_POP

namespace MinSG {
namespace Triangulation {

std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> doTetrahedralization(const std::vector<Geometry::Vec3f> & positions) {
	const size_t numPositions = positions.size();
COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wvla)
	float pointsArray[numPositions][3];
COMPILER_WARN_POP
	for(size_t i = 0; i < numPositions; ++i) {
		pointsArray[i][0] = positions[i].getX();
		pointsArray[i][1] = positions[i].getY();
		pointsArray[i][2] = positions[i].getZ();
	}
	Detri_Tetrahedrons * detriResult = detri_extern(pointsArray, numPositions);

	std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> result;
	result.reserve(detriResult->tetrahedronCount);
	for(int j = 0; j < detriResult->tetrahedronCount; ++j) {
		const Detri_Tetrahedron & t = detriResult->tetrahedrons[j];
		result.emplace_back(t.a, t.b, t.c, t.d);
	}
	detri_extern_free(detriResult);
	return result;
}

}
}

#endif /* MINSG_EXT_TRIANGULATION */
