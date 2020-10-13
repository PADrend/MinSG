/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShaderUniformState.h"
#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/Node.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Macros.h>

namespace MinSG {

//! (ctor)
ShaderUniformState::ShaderUniformState() :
	State() {
}

//! (ctor)
ShaderUniformState::ShaderUniformState(const ShaderUniformState & source) :
	State(source), uMap(source.uMap) {
}

//! (dtor)
ShaderUniformState::~ShaderUniformState() {
	uMap.clear();
}

//! ---|> [State]
ShaderUniformState * ShaderUniformState::clone() const {
	return new ShaderUniformState(*this);
}

void ShaderUniformState::setUniform(const Rendering::Uniform & value) {
	uMap[value.getNameId()] = value;
}


const Rendering::Uniform & ShaderUniformState::getUniform(const Util::StringIdentifier nameId) const {
	auto it = uMap.find(nameId);
	return it == uMap.end() ? Rendering::Uniform::nullUniform : it->second;
}

//! ---|> [State]
State::stateResult_t ShaderUniformState::doEnableState(FrameContext & context, Node *, const RenderParam & /*rp*/) {
	priorValueStack.push( std::vector<Rendering::Uniform>() );

	auto shader = context.getRenderingContext().getActiveShader();

	if(shader!=nullptr){
		// assure that possible changes in the renderingContext are applied to the sg-uniforms before querying the uniforms
		context.getRenderingContext().applyChanges();

		for(const auto & uniformEntry : uMap) {
			const Rendering::Uniform & uniform(uniformEntry.second);
			const Rendering::Uniform & priorUniform(shader->getUniform(uniform.getNameId()));
			if(priorUniform.isNull())
				continue;
			priorValueStack.top().push_back(priorUniform);
			shader->setUniform(context.getRenderingContext(),uniform,true); // warn if unused--> should not happen
		}
	}
	return State::STATE_OK;
}

//! ---|> [State]
void ShaderUniformState::doDisableState(FrameContext & context, Node *, const RenderParam & /*rp*/) {
	auto shader = context.getRenderingContext().getActiveShader();
	if(shader!=nullptr){
		for(const auto & uniform : priorValueStack.top()) {
			shader->setUniform(context.getRenderingContext(), uniform, true); // warn if unused--> should not happen
		}
	}
	priorValueStack.pop();
}

}
