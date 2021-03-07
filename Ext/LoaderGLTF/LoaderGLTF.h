/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_EXT_LOADERGLTF_H_
#define MINSG_EXT_LOADERGLTF_H_

#include <Util/References.h>
#include <vector>
#include <cstdint>

namespace Util {
class FileName;
class GenericAttributeMap;
}

namespace MinSG {
class Node;
namespace SceneManagement {
typedef Util::GenericAttributeMap DescriptionMap;
class SceneManager;
class ImportContext;
}

//! @ingroup ext
namespace LoaderGLTF {
	
typedef uint32_t importOption_t;
static const importOption_t IMPORT_OPTION_NONE = 0;

/**
 * Load a GlTF scene from the file system.
 *
 * @param fileName Path to the GlTF file to load
 * @c nullptr if an error occurred.
 */
MINSGAPI const SceneManagement::DescriptionMap* loadScene(const Util::FileName& fileName, const importOption_t importOptions=0);

/**
 * Load MinSG nodes from a glTF file.
 * 
 * @param sm SceneManager
 * @param fileName Path to a MinSG XML file
 * @return Array of MinSG nodes. In case of an error, an empty array will be returned.
 */
MINSGAPI std::vector<Util::Reference<Node>> loadGLTFScene(SceneManagement::SceneManager& sm, const Util::FileName & fileName, const importOption_t importOptions=0);

/**
 * Load MinSG nodes from a glTF file.
 * 
 * @param sm SceneManager
 * @param fileName Path to a MinSG XML file
 * @return Array of MinSG nodes. In case of an error, an empty array will be returned.
 */
MINSGAPI std::vector<Util::Reference<Node>> loadGLTFScene(SceneManagement::ImportContext& importContext, const Util::FileName & fileName);

}

}

#endif // MINSG_EXT_LOADERGLTF_H_