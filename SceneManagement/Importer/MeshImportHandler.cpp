/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MeshImportHandler.h"
#include "../../Helper/Helper.h"
#include <Util/IO/FileLocator.h>

namespace MinSG {
namespace SceneManagement {

Node * MeshImportHandler::handleImport(const Util::FileLocator& locator, const std::string & url, const NodeDescription * /*description*/) {
	return loadModel(Util::FileName(url), 0, nullptr,locator);
}

}
}
