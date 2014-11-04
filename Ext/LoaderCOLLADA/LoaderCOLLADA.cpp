/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include <iostream>

#include "LoaderCOLLADA.h"
#include "Writer.h"
#include "../../SceneManagement/SceneDescription.h"
#include <Util/IO/FileName.h>


COMPILER_WARN_PUSH
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
COMPILER_WARN_OFF_GCC(-Wzero-as-null-pointer-constant)
COMPILER_WARN_OFF_GCC(-Wswitch-default)
COMPILER_WARN_OFF_CLANG(-W#warnings)
#include <COLLADASaxFWLIError.h>
#include <COLLADASaxFWLIErrorHandler.h>
#include <COLLADASaxFWLLoader.h>
#include <COLLADASaxFWLPrerequisites.h>
#include <COLLADAFWRoot.h>
COMPILER_WARN_POP

#include <string>
#include <functional>

#include <Util/Macros.h>
#include <Util/Timer.h>

namespace MinSG {
namespace LoaderCOLLADA {

class ErrorHandler : public COLLADASaxFWL::IErrorHandler {
	private:
		std::ostream & out;

	public:
		ErrorHandler(std::ostream & outStream) :
			out(outStream) {
		}
		virtual ~ErrorHandler() {
		}

		bool handleError(const COLLADASaxFWL::IError * error) override {
			switch(error->getSeverity()) {
				case COLLADASaxFWL::IError::SEVERITY_CRITICAL:
					out << "Critical";
					break;
				case COLLADASaxFWL::IError::SEVERITY_ERROR_NONCRITICAL:
					out << "Non-critical";
					break;
				default:
					break;
			}
			out << ' ';
			switch(error->getErrorClass()) {
				case COLLADASaxFWL::IError::ERROR_SAXPARSER:
					out << "SaxParser";
					break;
				case COLLADASaxFWL::IError::ERROR_SAXFWL:
					out << "SaxFWL";
					break;
				default:
					out << "Unknown";
					break;
			}
			out << error->getFullErrorMessage() << '\n';
			// Continue loading.
			return true;
		}
};

static std::string getVersionString(const COLLADASaxFWL::COLLADAVersion & version) {
	switch(version) {
		case COLLADASaxFWL::COLLADAVersion::COLLADA_14:
			return "1.4";
		case COLLADASaxFWL::COLLADAVersion::COLLADA_15:
			return "1.5";
		case COLLADASaxFWL::COLLADAVersion::COLLADA_UNKNOWN:
		default:
			break;
	}
	return "Unknown";
}

const SceneManagement::DescriptionMap * loadScene(const Util::FileName & fileName, bool invertTransparency) {

	Util::Timer timer;
	timer.reset();

	ErrorHandler errorHandler(std::cerr);
	COLLADASaxFWL::Loader loader(&errorHandler);

	Writer & writer = Writer::instance();
	writer.beginLoadingProcess();
	writer.setInvertTransparency(invertTransparency);

	COLLADAFW::Root root(&loader, &writer);

	const bool success = root.loadDocument(fileName.getPath());
	if(!success) {
		return nullptr;
	}

	timer.stop();

	std::cout << "Loaded COLLADA " << getVersionString(loader.getCOLLADAVersion()) << " document in " << timer.getSeconds() << " seconds." << std::endl;

	auto scene = writer.releaseSceneDescription();
	writer.endLoadingProcess();
	return scene;
}

}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
