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
typedef Util::GenericAttributeList NodeDescriptionList;
typedef Util::GenericAttributeMap NodeDescription;
namespace ImporterTools {

std::deque<const NodeDescription *> filterElements(const std::string & type, 
												   const NodeDescriptionList * subDescriptions);

/*! Helper function that adds standard data to a node.
	- register named nodes
	- set transformation
	- add attributes
	- add states
	- add behaviours
	- add children (if node is a group node)	*/
void finalizeNode(ImportContext & ctxt, Node * node,const NodeDescription & d);

/*! Helper function that adds standard data to state.
	- register named state
	- add attributes
	more to come...	*/
void finalizeState(ImportContext & ctxt, State * state,const NodeDescription & d);

Geometry::SRT getSRT(const NodeDescription & d) ;

void setTransformation(const NodeDescription & d, Node * node) ;

void addAttributes(ImportContext & ctxt, const NodeDescriptionList * subDescriptions, Util::AttributeProvider * attrProvider) ;

/**
 * If the given @a description contains a node identifier, the given @a node is registered with that identifier in the local SceneManager.
 *
 * @param description Description, which was used to generate @a node.
 * @param node Generated node.
 */
void registerNamedNode(ImportContext & ctxt,const NodeDescription & description, Node * node);

/**
 * If the given @a description contains a state identifier, the given @a state is registered with that identifier in the local SceneManager.
 *
 * @param description Description, which was used to generate @a state.
 * @param state Generated state.
 */
void registerNamedState(ImportContext & ctxt,const NodeDescription & description, State * state) ;

Util::FileName checkRelativePaths(const ImportContext & ctxt,const Util::FileName & fileName);

}
}
}

#endif // IMPORTERTOOLS_H
