/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "GenericMetaNode.h"
#include "../../Core/FrameContext.h"

#include <Rendering/Draw.h>

#include <Util/Graphics/ColorLibrary.h>

namespace MinSG {

GenericMetaNode::~GenericMetaNode() = default;

//! ---|> Node
void GenericMetaNode::doDisplay(FrameContext & context,const RenderParam & rp){
	if (! (rp.getFlag (SHOW_META_OBJECTS|BOUNDING_BOXES)) )
		return;
	if (rp.getFlag(BOUNDING_BOXES)){
		// BB
		Rendering::drawWireframeBox(context.getRenderingContext(), getBB(),Util::ColorLibrary::GREEN);
	}
	if (rp.getFlag(SHOW_META_OBJECTS) ){
		Rendering::drawBox(context.getRenderingContext(), getBB(),Util::ColorLibrary::GREEN);
	}
}

}
