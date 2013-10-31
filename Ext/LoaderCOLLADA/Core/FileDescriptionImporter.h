/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2013 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_CORE_FILEDESCRIPTIONIMPORTER_H
#define MINSG_LOADERCOLLADA_CORE_FILEDESCRIPTIONIMPORTER_H

#include "../Utils/LoaderCOLLADAConsts.h"

namespace COLLADAFW {
class FileInfo;
}

namespace Util {
class GenericAttributeMap;
}
namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap NodeDescription;
}
namespace LoaderCOLLADA {

SceneManagement::NodeDescription * fileInformationImporter(const COLLADAFW::FileInfo * asset, referenceRegistry_t & referenceRegistry);

}
}

#endif /* MINSG_LOADERCOLLADA_CORE_FILEDESCRIPTIONIMPORTER_H */
#endif /* ifdef MINSG_EXT_LOADERCOLLADA */
