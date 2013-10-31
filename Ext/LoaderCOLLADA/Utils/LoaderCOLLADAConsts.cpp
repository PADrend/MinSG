/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "LoaderCOLLADAConsts.h"

namespace MinSG {
namespace LoaderCOLLADA {
namespace Consts {

const Util::StringIdentifier DAE_REFERENCE("_DAE_REFERENCE_");
const Util::StringIdentifier DAE_REFERENCE_LIST("_DAE_REFERENCE_LIST_");
const Util::StringIdentifier EFFECT_LIST("_EFFECT_LIST_");

const std::string DAE_FLAGS("_DAE_FLAGS_");
const Util::StringIdentifier DAE_FLAG_USE_TRANSPARENCY_RENDERER("_USE_TRANSPARENCY_RENDERER_");

const Util::StringIdentifier DAE_SUB_MATERIALID("_DAE_SUB_MATERIALID_");

const std::string DAE_GEOMETRY_LIST("_DAE_GEOMETRY_LIST_");
const Util::StringIdentifier DAE_MESH_VERTEXORDER("_DAE_MESH_VERTEXORDER_");

}
}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
