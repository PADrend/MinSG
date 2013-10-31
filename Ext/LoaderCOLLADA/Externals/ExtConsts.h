/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_EXTERNALS_EXTCONSTS_H
#define MINSG_LOADERCOLLADA_EXTERNALS_EXTCONSTS_H

#include <Util/StringIdentifier.h>

namespace MinSG {
namespace LoaderCOLLADA {
namespace Consts {

#ifdef MINSG_EXT_SKELETAL_ANIMATION
extern const Util::StringIdentifier DAE_ARMATURE_BINDMATRIX;

extern const Util::StringIdentifier DAE_JOINT_IDS;
extern const Util::StringIdentifier DAE_JOINT_COUNT;
extern const Util::StringIdentifier DAE_JOINT_INVERSEBINDMATRIX;

extern const Util::StringIdentifier DAE_WEIGHT_ARRAY;
extern const Util::StringIdentifier DAE_WEIGHT_INDICES;
extern const Util::StringIdentifier DAE_WEIGHT_JOINTSPERVERTEX;
extern const Util::StringIdentifier DAE_WEIGHT_JOINTINDICES;

extern const Util::StringIdentifier DAE_ANIMATION_NAME;
extern const Util::StringIdentifier DAE_ANIMATION_ADDED;
#endif /* MINSG_EXT_SKELETAL_ANIMATION */
}

}
}

#endif /* MINSG_LOADERCOLLADA_EXTERNALS_EXTCONSTS_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
