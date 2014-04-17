/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MESHIMPORTHANDLER_H_
#include <string>

namespace Util {
class FileName;
class GenericAttributeMap;
class FileLocator;
}

namespace MinSG {
class Node;
namespace SceneManagement {
typedef Util::GenericAttributeMap NodeDescription;

/**
 * Class that registers itself at the SceneManager and then is responsible to load meshes.
 *
 * @author Benjamin Eikel
 * @date 2011-02-17
 */
class MeshImportHandler {
	public:
		virtual ~MeshImportHandler() {
		}

		/**
		 * Load the mesh from the given address and create MinSG Nodes for it.
		 * This function is called by the StdImporter when a Mesh has to be loaded form a file system or network location.
		 *
		 * @param locator Used to resolve the actual path to the file.
		 * @param url Location of the mesh file.
		 * @param description Description of the Node to which the mesh belongs.
		 * @return Arbitrary node or tree of nodes that represents the mesh inside the scene graph.
		 */
		virtual Node * handleImport(const Util::FileLocator& locator, const std::string & url, const NodeDescription * description);
};

}
}

#endif /* MESHIMPORTHANDLER_H_ */
