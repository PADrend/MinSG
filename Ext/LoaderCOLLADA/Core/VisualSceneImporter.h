/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_CORE_VISUALSCENEIMPORTER_H
#define MINSG_LOADERCOLLADA_CORE_VISUALSCENEIMPORTER_H

#include "../Utils/LoaderCOLLADAConsts.h"

#include <map>
#include <string>

namespace COLLADAFW {
class VisualScene;
class Node;
}

namespace Util {
class GenericAttributeMap;
}

namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap DescriptionMap;
}


namespace LoaderCOLLADA {
//! @see Writer::visualFunc_t
bool visualSceneCoreImporter(const COLLADAFW::VisualScene * visualScene, referenceRegistry_t & referenceRegistry, SceneManagement::DescriptionMap * sceneDesc, const sceneNodeFunc_t & sceneNodeFunc);

// helper for extracting DAE child nodes inside visual description tree.
SceneManagement::DescriptionMap * sceneNodeImporter(const COLLADAFW::Node * childNode, referenceRegistry_t & referenceRegistry);
void extractInstancesFromNodes(SceneManagement::DescriptionMap * parent, referenceRegistry_t & referenceRegistry, const COLLADAFW::Node * node, const sceneNodeFunc_t & sceneNodeFunc);

void resolveReference(SceneManagement::DescriptionMap * child, const referenceRegistry_t & referenceRegistry);
}
}

#endif /* MINSG_LOADERCOLLADA_CORE_VISUALSCENEIMPORTER_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
