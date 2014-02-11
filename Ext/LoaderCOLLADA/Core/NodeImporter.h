/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_CORE_NODEIMPORTER_H
#define MINSG_LOADERCOLLADA_CORE_NODEIMPORTER_H

#include "../Utils/LoaderCOLLADAConsts.h"

#include <map>

#include <Util/Macros.h>

namespace COLLADAFW {
class Geometry;
class Light;
class MeshPrimitive;
}
namespace Util {
class GenericAttributeMap;
}
namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap NodeDescription;
}

namespace LoaderCOLLADA {

//! @see Writer::geometryFunc_t
bool nodeCoreImporter(const COLLADAFW::Geometry * geometry, referenceRegistry_t & referenceRegistry);

//! @see Writer::lightFunc_t
bool lightCoreImporter(const COLLADAFW::Light * light, referenceRegistry_t & referenceRegistry);

// converts COLLADAPrimitiveType Quad to a Triangle
void triangulateQuadToTri(const COLLADAFW::MeshPrimitive & primitive, const uint32_t index, const bool hasNormals,
						  const bool hasColors, const bool hasUVCoords, std::vector<uint32_t> & indices);

// extracts all primitives inside one mesh.
void extractPrimitive(const COLLADAFW::MeshPrimitive & primitive, const bool hasNormals, const bool hasColors, const bool hasUVCoords,
					  std::vector<uint32_t> & indices, uint32_t & triangleCount);
}

}

#endif /* MINSG_LOADERCOLLADA_CORE_NODEIMPORTER_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
