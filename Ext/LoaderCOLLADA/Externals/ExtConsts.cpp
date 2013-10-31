/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "ExtConsts.h"

namespace MinSG {
namespace LoaderCOLLADA {
namespace Consts {

#ifdef MINSG_EXT_SKELETAL_ANIMATION
extern const Util::StringIdentifier DAE_ARMATURE_BINDMATRIX("_DAE_ARMATURE_BINDMATRIX_");

extern const Util::StringIdentifier DAE_JOINT_IDS("_DAE_JOINT_IDS");
extern const Util::StringIdentifier DAE_JOINT_COUNT("_DAE_JOINT_COUNT_");
extern const Util::StringIdentifier DAE_JOINT_INVERSEBINDMATRIX("_DAE_JOINT_INVERSEBINDMATRIX_");

extern const Util::StringIdentifier DAE_WEIGHT_ARRAY("_DAE_WEIGHT_ARRAY_");
extern const Util::StringIdentifier DAE_WEIGHT_INDICES("_DAE_WEIGHT_INDICES_");
extern const Util::StringIdentifier DAE_WEIGHT_JOINTSPERVERTEX("_DAE_WEIGHT_JOINTSPERVERTEX_");
extern const Util::StringIdentifier DAE_WEIGHT_JOINTINDICES("_DAE_WEIGHT_JOINTINDICES_");

extern const Util::StringIdentifier DAE_ANIMATION_NAME("_DAE_ANIMATION_NAME_");
extern const Util::StringIdentifier DAE_ANIMATION_ADDED("_DAE_ANIMATION_ADDED_");
#endif /* MINSG_EXT_SKELETAL_ANIMATION */

}
}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
