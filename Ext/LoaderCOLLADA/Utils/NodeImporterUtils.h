/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_UTILS_NODEIMPORTERUTILS_H
#define MINSG_LOADERCOLLADA_UTILS_NODEIMPORTERUTILS_H

#include <cstdint>
#include <deque>
#include <vector>

namespace Rendering {
class Mesh;
}
namespace Util {
class GenericAttributeMap;
}
namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap NodeDescription;
}
namespace LoaderCOLLADA {

struct VertexPart {
	enum type_t {
		POSITION, NORMAL, COLOR, TEXCOORD
	} type;
	uint16_t indexOffset;
	uint16_t stride;
	std::vector<float> data;
};

Rendering::Mesh * createSingleMesh(const std::deque<VertexPart> & vertexParts, const std::vector<uint32_t> & indices, const uint32_t triangleCount,
								   const uint16_t maxIndexOffset, std::vector<uint32_t> & vertexIds);

}
}

#endif /* MINSG_LOADERCOLLADA_UTILS_NODEIMPORTERUTILS_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
