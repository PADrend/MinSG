/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_UTILS_LOADERCOLLADACONSTS_H
#define MINSG_LOADERCOLLADA_UTILS_LOADERCOLLADACONSTS_H

#include <Util/StringIdentifier.h>

#include <Util/GenericAttribute.h>

#include <unordered_map>
#include <string>

#include <Util/Macros.h>

COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wignored-qualifiers)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
COMPILER_WARN_OFF_GCC(-Wzero-as-null-pointer-constant)
COMPILER_WARN_OFF_GCC(-pedantic)
COMPILER_WARN_OFF_CLANG(-Wgnu)
COMPILER_WARN_OFF_GCC(-Wcast-qual)
COMPILER_WARN_OFF_GCC(-Wshadow)
#include <COLLADAFWUniqueId.h>
COMPILER_WARN_POP

namespace COLLADAFW {
class Node;
}

namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap NodeDescription;
}

namespace LoaderCOLLADA {
typedef Util::WrapperAttribute<COLLADAFW::UniqueId> daeReferenceData;
typedef Util::WrapperAttribute<std::vector<COLLADAFW::UniqueId> > daeReferenceList;
typedef Util::WrapperAttribute<COLLADAFW::MaterialId> MaterialReference;

typedef std::unordered_map<size_t, SceneManagement::NodeDescription *> referenceRegistry_t;
typedef std::function<SceneManagement::NodeDescription *(const COLLADAFW::Node *, referenceRegistry_t &)> sceneNodeFunc_t;

namespace Consts {
extern const Util::StringIdentifier DAE_REFERENCE;
extern const Util::StringIdentifier DAE_REFERENCE_LIST;
extern const Util::StringIdentifier EFFECT_LIST;

extern const std::string DAE_FLAGS;
extern const Util::StringIdentifier DAE_FLAG_USE_TRANSPARENCY_RENDERER;

extern const Util::StringIdentifier DAE_SUB_MATERIALID;

extern const std::string DAE_GEOMETRY_LIST;

// indices to reproduze dae vertex order.
extern const Util::StringIdentifier DAE_MESH_VERTEXORDER;
}
}
}


#endif /* MINSG_LOADERCOLLADA_UTILS_LOADERCOLLADACONSTS_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
