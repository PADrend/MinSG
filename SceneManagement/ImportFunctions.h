/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef IMPORT_FUNCTIONS_H_
#define IMPORT_FUNCTIONS_H_

#include <Util/IO/FileName.h>
#include <Util/References.h>
#include <vector>

namespace MinSG {
class Node;
class GroupNode;

namespace SceneManagement {
class SceneManager;
class ImportContext;


typedef uint32_t importOption_t;
static const importOption_t IMPORT_OPTION_NONE = 0;
static const importOption_t IMPORT_OPTION_REUSE_EXISTING_STATES = 1<<0;
static const importOption_t IMPORT_OPTION_DAE_INVERT_TRANSPARENCY = 1<<2;
static const importOption_t IMPORT_OPTION_USE_TEXTURE_REGISTRY = 1<<3;
static const importOption_t IMPORT_OPTION_USE_MESH_REGISTRY = 1<<4;
static const importOption_t IMPORT_OPTION_USE_MESH_HASHING_REGISTRY = 1<<5;


/**
 * Load MinSG nodes from a file.
 * 
 * @param fileName Path to a MinSG XML file
 * @param importOptions Options controlling the import procedure
 * @return Array of MinSG nodes. In case of an error, an empty array will be returned.
 */
MINSGAPI std::vector<Util::Reference<Node>> loadMinSGFile(SceneManager& sm,const Util::FileName & fileName, const importOption_t importOptions = IMPORT_OPTION_NONE);

/**
 * Load MinSG nodes from a file.
 * 
 * @param importContext Context that is used for the import procedure
 * @param fileName Path to a MinSG XML file
 * @return Array of MinSG nodes. In case of an error, an empty array will be returned.
 */
MINSGAPI std::vector<Util::Reference<Node>> loadMinSGFile(ImportContext & importContext, const Util::FileName & fileName);

/**
 * Load MinSG nodes from a stream.
 * 
 * @param importContext Context that is used for the import procedure
 * @param in Input stream providing MinSG XML data
 * @return Array of MinSG nodes. In case of an error, an empty array will be returned.
 */
MINSGAPI std::vector<Util::Reference<Node>> loadMinSGStream(ImportContext & importContext, std::istream & in);

MINSGAPI GroupNode * loadCOLLADA(SceneManager & sm,const Util::FileName & fileName,const importOption_t importOptions=IMPORT_OPTION_NONE);
MINSGAPI GroupNode * loadCOLLADA(ImportContext & importContext, const Util::FileName & fileName);

MINSGAPI ImportContext createImportContext(SceneManager & sm,const importOption_t importOptions=IMPORT_OPTION_NONE);

}
}

#endif // IMPORT_FUNCTIONS_H_