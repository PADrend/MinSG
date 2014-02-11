/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_CORE_STATEIMPORTER_H
#define MINSG_LOADERCOLLADA_CORE_STATEIMPORTER_H

#include "../Utils/LoaderCOLLADAConsts.h"

#include <map>

namespace COLLADAFW {
class Material;
class Effect;
class Texture;
class Image;
}
namespace Util {
class GenericAttributeMap;
}
namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap NodeDescription;
}

namespace LoaderCOLLADA {
//! @see Writer::materialFunc_t
bool materialCoreImporter(const COLLADAFW::Material * material, referenceRegistry_t & referenceRegistry);

//! @see Writer::effectFunc_t
bool effectCoreImporter(const COLLADAFW::Effect * effect, referenceRegistry_t & referenceRegistry, bool invertTransparency = false);

//! @see Writer::imageFunc_t
bool imageCoreImporter(const COLLADAFW::Image * image, referenceRegistry_t & referenceRegistry);
}

}

#endif /* MINSG_LOADERCOLLADA_CORE_STATEIMPORTER_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
