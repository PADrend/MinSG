/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef EXPORTERCONTEXT_H
#define EXPORTERCONTEXT_H

#include <Util/IO/FileName.h>
#include <Util/GenericAttribute.h>
#include <functional>
#include <deque>
#include <set>
#include <string>
#include <memory>

namespace Geometry {
template<typename _T> class _SRT;
typedef _SRT<float> SRT;
}
namespace Util {
class GenericAttributeMap;
}
namespace MinSG {
class Node;
class State;
class AbstractBehaviour;
namespace SceneManagement {
typedef Util::GenericAttributeMap NodeDescription;
class SceneManager;

/*! Helper structure that keeps data for one export process. */
struct ExporterContext {
	SceneManager & sceneManager;
	std::deque<NodeDescription> usedPrototypes;
	std::set<std::string> usedPrototypeIds;
	std::set<std::string> usedStateIds;
	
	int tmpNodeCounter;
	
	//! A function that allows the execution of arbitrary actions at the end of the export process.
	typedef std::function<void (ExporterContext &)> FinalizeAction;
	std::deque<FinalizeAction> finalizeActions;
		
	Util::FileName sceneFile;

	ExporterContext(SceneManager & _m) : sceneManager(_m),tmpNodeCounter(0){}

	void addFinalizingAction(const FinalizeAction & action) {
		finalizeActions.push_back(action);
	}
	void addUsedPrototype(const std::string & nodeId,NodeDescription&& d){
		usedPrototypeIds.insert(nodeId);
		usedPrototypes.emplace_back(std::move(d));
	}

	void executeFinalizingActions(){
		for(auto & action : finalizeActions) 
			action(*this);
		finalizeActions.clear();
	}
		
	bool isPrototypeUsed(const std::string & nodeId)	{
		return usedPrototypeIds.count(nodeId) != 0;
	}

};

}

}
#endif // EXPORTERCONTEXT_H
