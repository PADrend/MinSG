/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ImportFunctions.h"
#include "SceneDescription.h"

#include "Importer/ImportContext.h"
#include "Importer/ReaderMinSG.h"
#include "Importer/ImporterTools.h"
#include "Importer/ReaderDAE.h"

#include "../Core/Nodes/ListNode.h"
#include "../Helper/StdNodeVisitors.h"

#ifdef MINSG_EXT_LOADERCOLLADA
#include "../Ext/LoaderCOLLADA/LoaderCOLLADA.h"
#endif

#include <Util/IO/FileUtils.h>
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <memory>

namespace MinSG{

namespace SceneManagement {
	
ImportContext createImportContext(SceneManager & sm,const importOption_t importOptions) {
	return {sm, nullptr, importOptions, Util::FileName("")};
}


std::vector<Util::Reference<Node>> loadMinSGFile(SceneManager& sm,const Util::FileName & fileName, const importOption_t importOptions/*=IMPORT_OPTION_NONE*/) {
	auto importContext = createImportContext(sm,importOptions);
	return loadMinSGFile(importContext, fileName);
}

std::vector<Util::Reference<Node>> loadMinSGFile(ImportContext & importContext,const Util::FileName & fileName) {
	importContext.setFileName(fileName);

	auto in = Util::FileUtils::openForReading(fileName);
	if(!in) {
		WARN(std::string("Could not load file: ") + fileName.toString());
		return std::vector<Util::Reference<Node>>();
	}

	return loadMinSGStream(importContext, *(in.get()));
}

std::vector<Util::Reference<Node>> loadMinSGStream(ImportContext & importContext, std::istream & in) {
	if(!in.good()) {
		WARN("Cannot load MinSG nodes from the given stream.");
		return std::vector<Util::Reference<Node>>();
	}

	// parse xml and create description
	std::unique_ptr<const NodeDescription> sceneDescription(ReaderMinSG::loadScene(in));

	// create MinSG scene tree from description with dummy root node
	Util::Reference<ListNode> dummyContainerNode=new ListNode;
	importContext.setRootNode(dummyContainerNode.get());

	ImporterTools::buildSceneFromDescription(importContext, sceneDescription.get());

	// detach nodes from dummy root node
	std::vector<Util::Reference<Node>> nodes;
	const auto nodesTmp = getChildNodes(dummyContainerNode.get());
	for(const auto & node : nodesTmp) {
		nodes.push_back(node);
		node->removeFromParent();
	}
	importContext.setRootNode(nullptr);
	return nodes;
}

GroupNode * loadCOLLADA(SceneManager& sm,const Util::FileName & fileName, const importOption_t importOptions) {
	auto importContext = createImportContext(sm,importOptions);
	return loadCOLLADA(importContext, fileName);
}

GroupNode * loadCOLLADA(ImportContext & importContext, const Util::FileName & fileName) {
	Util::Reference<GroupNode> container = new ListNode;
	importContext.setFileName(fileName);
	importContext.setRootNode(container.get());

	Util::Timer timer;
	timer.reset();
    
	const bool invertTransparency = (importContext.getImportOptions() & IMPORT_OPTION_DAE_INVERT_TRANSPARENCY) > 0;
#ifdef MINSG_EXT_LOADERCOLLADA
    const NodeDescription * sceneDescription = LoaderCOLLADA::loadScene(fileName, invertTransparency);
#else  
    auto in = Util::FileUtils::openForReading(fileName);
	if(!in) {
		WARN(std::string("Could not load file: ") + fileName.toString());
		return nullptr;
	}

    std::unique_ptr<const NodeDescription> sceneDescriptionPtr(ReaderDAE::loadScene(*(in.get()), invertTransparency));
	auto sceneDescription = sceneDescriptionPtr.get();
#endif
    
    Util::info << timer.getSeconds() << "s";
	timer.reset();

	Util::info << "\nBuilding scene graph...";
	ImporterTools::buildSceneFromDescription(importContext, sceneDescription);

	Util::info << timer.getSeconds() << "s";
	Util::info << "\tdone.\n";

	importContext.setRootNode(nullptr);

	return container.detachAndDecrease();
}

}
}
