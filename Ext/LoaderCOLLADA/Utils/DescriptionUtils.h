/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_UTILS_DESCRIPTIONUTILS_H
#define MINSG_LOADERCOLLADA_UTILS_DESCRIPTIONUTILS_H

#include "LoaderCOLLADAConsts.h"

namespace Util {
class GenericAttributeMap;
class GenericAttributeList;
}
namespace COLLADABU {
namespace Math {
class Matrix4;
}
}
namespace COLLADAFW {
class Node;
}
namespace Geometry {
template<typename _T> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4;
}
namespace Util {
class StringIdentifier;
class GenericAttribute;
}
namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap DescriptionMap;
typedef Util::GenericAttributeList DescriptionArray;
}

namespace LoaderCOLLADA {

void addToMinSGChildren(SceneManagement::DescriptionMap * parent, SceneManagement::DescriptionMap * child);
void addToParentAsChildren(SceneManagement::DescriptionMap * parent, SceneManagement::DescriptionMap * child, const Util::StringIdentifier & childrenName);
void copyAttributesToNodeDescription(SceneManagement::DescriptionMap * parent, SceneManagement::DescriptionMap * description, bool force = false);
void finalizeNodeDescription(const COLLADAFW::Node * object, SceneManagement::DescriptionMap * description);
void addTransformationDataIntoNodeDescription(SceneManagement::DescriptionMap & sourceDescription, const COLLADABU::Math::Matrix4 & colladaMatrix);
void addDAEFlag(referenceRegistry_t & referenceRegistry, const Util::StringIdentifier & identifier, Util::GenericAttribute * attr);
std::vector<SceneManagement::DescriptionMap *> findDescriptions(const referenceRegistry_t & referenceRegistry, const Util::StringIdentifier & key, const std::string & name);

}
}

#endif /* MINSG_LOADERCOLLADA_UTILS_DESCRIPTIONUTILS_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
