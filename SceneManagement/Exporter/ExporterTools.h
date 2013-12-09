/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef EXPORTERTOOLS_H
#define EXPORTERTOOLS_H

#include "ExporterContext.h"

#include <Util/GenericAttribute.h>
#include <deque>
#include <set>
#include <string>

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
namespace ExporterTools {

typedef std::function<void (ExporterContext & ctxt,NodeDescription&,Node * node)> NodeExport_Fn_t;
typedef std::function<void (ExporterContext & ctxt,NodeDescription&,State * state)> StateExport_Fn_t;
typedef std::function<NodeDescription *(ExporterContext & ctxt,AbstractBehaviour * behaviour)> BehaviourExport_Fn_t;
void registerNodeExporter(const Util::StringIdentifier & classId,NodeExport_Fn_t);
void registerStateExporter(const Util::StringIdentifier & classId,StateExport_Fn_t);
void registerBehaviourExporter(BehaviourExport_Fn_t);



/*! Helper function that adds standard data to a description.
	- set string TYPE = TYPE_BEHAVIOUR */
void finalizeBehaviourDescription(ExporterContext & ctxt,NodeDescription & description, AbstractBehaviour * behaviour);

void addAttributesToDescription(ExporterContext & ctxt, NodeDescription & description, const Util::GenericAttribute::Map * attribs);
void addSRTToDescription(NodeDescription & description, const Geometry::SRT & srt);
void addTransformationToDescription(NodeDescription & description, Node * node);
void addChildEntry(NodeDescription & description, NodeDescription && childDescription);
void addDataEntry(NodeDescription & description, NodeDescription && dataDescription);

void addChildNodesToDescription(ExporterContext & ctxt,NodeDescription & description, Node * node);
void addStatesToDescription(ExporterContext & ctxt,NodeDescription & description, Node * node);
void addBehavioursToDescription(ExporterContext & ctxt,NodeDescription & description, Node * node);

std::unique_ptr<NodeDescription> createDescriptionForBehaviour(ExporterContext & ctxt,AbstractBehaviour * behaviour);
std::unique_ptr<NodeDescription> createDescriptionForNode(ExporterContext & ctxt,Node * node);
std::unique_ptr<NodeDescription> createDescriptionForScene(ExporterContext & ctxt, const std::deque<Node *> &nodes);
std::unique_ptr<NodeDescription> createDescriptionForState(ExporterContext & ctxt,State * state);

}
}
}

#endif // EXPORTERTOOLS_H
