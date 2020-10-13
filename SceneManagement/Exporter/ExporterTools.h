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
typedef Util::GenericAttributeMap DescriptionMap;
namespace ExporterTools {

typedef std::function<void (ExporterContext & ctxt,DescriptionMap&,Node * node)> NodeExport_Fn_t;
typedef std::function<void (ExporterContext & ctxt,DescriptionMap&,State * state)> StateExport_Fn_t;
typedef std::function<DescriptionMap *(ExporterContext & ctxt,AbstractBehaviour * behaviour)> BehaviourExport_Fn_t;
MINSGAPI void registerNodeExporter(const Util::StringIdentifier & classId,NodeExport_Fn_t);
MINSGAPI void registerStateExporter(const Util::StringIdentifier & classId,StateExport_Fn_t);
MINSGAPI void registerBehaviourExporter(BehaviourExport_Fn_t);



/*! Helper function that adds standard data to a description.
	- set string TYPE = TYPE_BEHAVIOUR */
MINSGAPI void finalizeBehaviourDescription(ExporterContext & ctxt,DescriptionMap & description, AbstractBehaviour * behaviour);

MINSGAPI void addAttributesToDescription(ExporterContext & ctxt, DescriptionMap & description, const Util::GenericAttribute::Map * attribs);
MINSGAPI void addSRTToDescription(DescriptionMap & description, const Geometry::SRT & srt);
MINSGAPI void addTransformationToDescription(DescriptionMap & description, Node * node);
MINSGAPI void addChildEntry(DescriptionMap & description, std::unique_ptr<DescriptionMap> childDescription);
MINSGAPI void addDataEntry(DescriptionMap & description, std::unique_ptr<DescriptionMap> dataDescription);

MINSGAPI void addChildNodesToDescription(ExporterContext & ctxt,DescriptionMap & description, Node * node);
MINSGAPI void addStatesToDescription(ExporterContext & ctxt,DescriptionMap & description, Node * node);
MINSGAPI void addBehavioursToDescription(ExporterContext & ctxt,DescriptionMap & description, Node * node);

MINSGAPI std::unique_ptr<DescriptionMap> createDescriptionForBehaviour(ExporterContext & ctxt,AbstractBehaviour * behaviour);
MINSGAPI std::unique_ptr<DescriptionMap> createDescriptionForNode(ExporterContext & ctxt,Node * node);
MINSGAPI std::unique_ptr<DescriptionMap> createDescriptionForScene(ExporterContext & ctxt, const std::deque<Node *> &nodes);
MINSGAPI std::unique_ptr<DescriptionMap> createDescriptionForState(ExporterContext & ctxt,State * state);

}
}
}

#endif // EXPORTERTOOLS_H
