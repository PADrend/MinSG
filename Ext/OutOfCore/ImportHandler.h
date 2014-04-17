/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_IMPORTHANDLER_H_
#define OUTOFCORE_IMPORTHANDLER_H_

#include "../../SceneManagement/Importer/MeshImportHandler.h"

namespace Util {
class FileName;
}
namespace MinSG {
class Node;
namespace OutOfCore {

/**
 * Class that registers itself at the SceneManager and then takes notice whenever a new mesh is to be loaded.
 *
 * @author Benjamin Eikel
 * @date 2011-02-18
 */
class ImportHandler : public SceneManagement::MeshImportHandler {
	public:
		virtual ~ImportHandler() {
		}

		/**
		 * Add a new cache object that is a representative for the mesh.
		 * Only meshes from ".mmf" and ".ply" files are supported by this class.
		 * Other meshes will be loaded with the function of the superclass.
		 * This function is called by the StdImporter when a Mesh has to be loaded form a file system or network location.
		 *
		 * @param url Location of the mesh file.
		 * @param description Description of the Node to which the mesh belongs.
		 * @return Arbitrary node or tree of nodes that represents the mesh inside the scene graph.
		 */
		 Node * handleImport(const Util::FileLocator& locator, const std::string & url, const SceneManagement::NodeDescription * description) override;
};

}
}

#endif /* OUTOFCORE_IMPORTHANDLER_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
