/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2013 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "FileDescriptionImporter.h"

#include "../../../SceneManagement/SceneDescription.h"

#include <Util/Macros.h>
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_GCC(-Wzero-as-null-pointer-constant)
COMPILER_WARN_OFF(-Wignored-qualifiers)
#include <COLLADAFWFileInfo.h>
COMPILER_WARN_POP

namespace MinSG {
namespace LoaderCOLLADA {

SceneManagement::NodeDescription * fileInformationImporter(const COLLADAFW::FileInfo * asset, referenceRegistry_t & /*referenceRegistry*/) {
	auto sceneDesc = new SceneManagement::NodeDescription;

	if(asset->getUpAxisType() == COLLADAFW::FileInfo::X_UP) {
		sceneDesc->setString(SceneManagement::Consts::ATTR_SRT_UP, "-1.0 0.0 0.0");
		sceneDesc->setString(SceneManagement::Consts::ATTR_SRT_DIR, "0.0 0.0 1.0");
	} else if(asset->getUpAxisType() == COLLADAFW::FileInfo::Z_UP) {
		sceneDesc->setString(SceneManagement::Consts::ATTR_SRT_UP, "0.0 0.0 -1.0");
		sceneDesc->setString(SceneManagement::Consts::ATTR_SRT_DIR, "0.0 1.0 0.0");
	}

	return sceneDesc;
}

}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
