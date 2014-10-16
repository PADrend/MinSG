/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_LOADERCOLLADA_H
#define MINSG_LOADERCOLLADA_LOADERCOLLADA_H

namespace Util {
class FileName;
class GenericAttributeMap;
}
namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap DescriptionMap;
}
namespace LoaderCOLLADA {
class Writer;
/**
 * Load a COLLADA scene from the file system.
 *
 * @param fileName Path to the COLLADA file to load
 * @return Valid scene description if successful,
 * @c nullptr if an error occurred.
 */
const SceneManagement::DescriptionMap * loadScene(const Util::FileName & fileName, bool invertTransparency);

}
}

#endif /* MINSG_LOADERCOLLADA_LOADERCOLLADA_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
