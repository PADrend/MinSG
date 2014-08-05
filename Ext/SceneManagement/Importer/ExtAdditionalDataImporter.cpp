/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExtAdditionalDataImporter.h"

#include "../../../SceneManagement/SceneDescription.h"
#include "../../../SceneManagement/SceneManager.h"
#include "../../../SceneManagement/Importer/ImporterTools.h"


#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
#include "../../VisibilitySubdivision/VisibilityVector.h"
#include "../../../Core/Nodes/GeometryNode.h"
#include <Util/Macros.h>
#endif

#include <Util/GenericAttribute.h>

#include <functional>
#include <cassert>

namespace MinSG {
namespace SceneManagement {

#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
using namespace VisibilitySubdivision;
static bool importAdditionalVisibilityVectorData(ImportContext & ctxt, const std::string & type, const DescriptionMap & d) {
	// Special case to load a VisibilityVector.
	if(type == "visibility_vector") {
		const std::string valueStr = d.getString(Consts::ATTR_ATTRIBUTE_VALUE);

		VisibilityVector vv;

		const size_t length = valueStr.length();
		if(valueStr.empty() || valueStr.at(0) != '(' || valueStr.at(length - 1) != ')') {
			WARN("Error: VisibilityVector has to begin with \"(\" and end with \")\".");
			FAIL();
		}

		size_t cursor = 1;
		while(cursor < length - 1) {
			// Entries are of the form (key, value, value).
			if(valueStr[cursor] != '(') {
				WARN("Error: Element of VisibilityVector does not begin with \"(\".");
				FAIL();
			}
			++cursor;

			// Read key until comma is found.
			size_t oldCursor = cursor;
			cursor = valueStr.find_first_of(',', cursor);
			if(cursor == std::string::npos) {
				WARN("Error: Invalid key of VisibilityVector element.");
				FAIL();
			}
			Node * node = ctxt.sceneManager.getRegisteredNode(valueStr.substr(oldCursor, cursor - oldCursor));
			// Skip comma.
			++cursor;

			// Read value as number.
			oldCursor = cursor;
			cursor = valueStr.find_first_of(',', cursor);
			if(cursor == std::string::npos) {
				WARN("Error: Invalid second value of VisibilityVector element.");
				FAIL();
			}
			// Do not use the value here.
			// Skip comma.
			++cursor;

			// Read value as number.
			oldCursor = cursor;
			cursor = valueStr.find_first_of(')', cursor);
			if(cursor == std::string::npos) {
				WARN("Error: Invalid third value of VisibilityVector element.");
				FAIL();
			}
			VisibilityVector::benefits_t benefits = strtoul(valueStr.data() + oldCursor, nullptr, 10);

			vv.setNode(dynamic_cast<GeometryNode *>(node), benefits);

			if(valueStr[cursor] != ')') {
				WARN("Error: Element of VisibilityVector does not end with \")\".");
				FAIL();
			}
			++cursor;
			if(valueStr[cursor] == ',') {
				// Skip additionally " ".
				cursor += 2;
			}
		}
		static const Util::StringIdentifier attrName("VisibilityVectors");

		Util::GenericAttributeMap * vvMap = dynamic_cast<Util::GenericAttributeMap *>(ctxt.getAttribute(attrName));
		if(vvMap == nullptr) {
			vvMap = new Util::GenericAttributeMap;
			ctxt.setAttribute(attrName, vvMap);
		}
		vvMap->setValue(d.getString(Consts::ATTR_NODE_ID), new VisibilitySubdivision::VisibilityVectorAttribute(vv));
		return true;
	}
	return false;
}
#endif

void initExtAdditionalDataImporter() {

#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
	ImporterTools::registerAdditionalDataImporter(&importAdditionalVisibilityVectorData);
#endif
}

}
}
