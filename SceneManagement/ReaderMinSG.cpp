/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ReaderMinSG.h"
#include "SceneDescription.h"
#include <Util/Macros.h>
#include <Util/GenericAttribute.h>
#include <Util/MicroXML.h>
#include <memory>
#include <stack>

namespace MinSG {
namespace SceneManagement {
namespace ReaderMinSG {

struct VisitorContext {
	std::stack<NodeDescription *> elements;
	std::unique_ptr<NodeDescription> scene;
};

static bool visitorEnter(VisitorContext & ctxt,
						 const std::string & tagName,
						 const Util::MicroXML::attributes_t & attributes) {
	NodeDescription * parent = ctxt.elements.empty() ? nullptr : ctxt.elements.top();

	auto desc = new NodeDescription;
	desc->setString(Consts::TYPE, tagName);
	ctxt.elements.push(desc);

	if(tagName == "scene") {
		if(ctxt.scene) {
			FAIL();
		}
		ctxt.scene.reset(desc);
	}

	for(const auto & attrEntry : attributes) {
		desc->setString(attrEntry.first, attrEntry.second);// TODO! (manager.parseString(it->second)));
	}

	if(!parent) {
		return true;
	}
	if(tagName == "defs") {
		parent->setValue(Consts::DEFINITIONS, desc);
		return true;
	}
	auto * children = dynamic_cast<NodeDescriptionList *>(parent->getValue(Consts::CHILDREN));
	if(!children) {
		children = new NodeDescriptionList;
		parent->setValue(Consts::CHILDREN, children);
	}
	children->push_back(desc);
	return true;
}

static bool visitorLeave(VisitorContext & ctxt, const std::string & /*tagName*/) {
	NodeDescription * currentElement = ctxt.elements.top();
	if(!currentElement) {
		FAIL();
	}
	ctxt.elements.pop();
	return true;
}

static bool visitorData(VisitorContext & ctxt, const std::string & /*tag*/, const std::string & data) {
	if(!ctxt.elements.empty()) {
		ctxt.elements.top()->setString(Consts::DATA_BLOCK, data);
	}
	return true;
}

const NodeDescription * loadScene(std::istream & in) {
	VisitorContext context;
	using namespace std::placeholders;
	Util::MicroXML::Reader::traverse(in,
									 std::bind(visitorEnter, std::ref(context), _1, _2),
									 std::bind(visitorLeave, std::ref(context), _1),
									 std::bind(visitorData, std::ref(context), _1, _2));
	return context.scene.release();
}

}
}
}
