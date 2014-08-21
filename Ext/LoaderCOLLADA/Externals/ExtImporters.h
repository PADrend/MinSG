/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_EXTERNALS_EXTIMPORTERS_H
#define MINSG_LOADERCOLLADA_EXTERNALS_EXTIMPORTERS_H

#include "../Utils/LoaderCOLLADAConsts.h"
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
#include <COLLADAFWController.h>
COMPILER_WARN_POP

#include <unordered_map>
#include <string>

namespace Util {
class GenericAttributeMap;
}
namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap DescriptionMap;
}
namespace LoaderCOLLADA {
class Writer;
namespace ExternalImporter {

void initExternalFunctions(Writer & writer);

}
}
}

#endif /* MINSG_LOADERCOLLADA_EXTERNALS_EXTIMPORTERS_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
