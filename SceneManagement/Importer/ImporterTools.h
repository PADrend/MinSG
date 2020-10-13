/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef IMPORTERTOOLS_H
#define IMPORTERTOOLS_H

#include "ImportContext.h"
#include <deque>
#include <string>

namespace Geometry {
template<typename T_> class _SRT;
typedef _SRT<float> SRT;
}
namespace Util {
class GenericAttributeList;
class GenericAttributeMap;
}
namespace MinSG {
class Node;
class State;
namespace SceneManagement {
typedef Util::GenericAttributeList DescriptionArray;
typedef Util::GenericAttributeMap DescriptionMap;
class MeshImportHandler;

namespace ImporterTools {

MINSGAPI std::deque<const DescriptionMap *> filterElements(const std::string & type, 
												   const DescriptionArray * subDescriptions);

/*! Helper function that adds standard data to a node.
	- register named nodes
	- set transformation
	- add attributes
	- add states
	- add behaviours
	- add children (if node is a group node)	*/
MINSGAPI void finalizeNode(ImportContext & ctxt, Node * node,const DescriptionMap & d);

/*! Helper function that adds standard data to state.
	- register named state
	- add attributes
	more to come...	*/
MINSGAPI void finalizeState(ImportContext & ctxt, State * state,const DescriptionMap & d);

MINSGAPI Geometry::SRT getSRT(const DescriptionMap & d) ;

MINSGAPI void addAttributes(ImportContext & ctxt, const DescriptionArray * subDescriptions, Util::AttributeProvider * attrProvider) ;

typedef std::function<bool (ImportContext & ctxt,const std::string & type, const DescriptionMap & description, GroupNode * parent)> NodeImport_Fn_t;
typedef std::function<bool (ImportContext & ctxt,const std::string & type, const DescriptionMap & description, Node * parent)> StateImport_Fn_t;
typedef std::function<bool (ImportContext & ctxt,const std::string & type, const DescriptionMap & description, Node * parent)> BehaviourImport_Fn_t;
typedef std::function<bool (ImportContext & ctxt,const std::string & type, const DescriptionMap & description)> AdditionalDataImport_Fn_t;

MINSGAPI void registerNodeImporter(NodeImport_Fn_t);
MINSGAPI void registerStateImporter(StateImport_Fn_t);
MINSGAPI void registerBehaviourImporter(BehaviourImport_Fn_t);
MINSGAPI void registerAdditionalDataImporter(AdditionalDataImport_Fn_t);

MINSGAPI MeshImportHandler * getMeshImportHandler();
MINSGAPI void setMeshImportHandler(std::unique_ptr<MeshImportHandler> handler);

MINSGAPI void buildSceneFromDescription(ImportContext & importContext,const DescriptionMap * d);

}
}
}

#endif // IMPORTERTOOLS_H
