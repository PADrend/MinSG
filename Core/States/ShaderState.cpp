/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShaderState.h"
#include "../FrameContext.h"
#include "../Nodes/Node.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Macros.h>
#include <map>

namespace MinSG {

//! (ctor)
ShaderState::ShaderState(Rendering::Shader * _shader/*=nullptr*/) :
	State(), shader(_shader) {
}

//! (ctor)
ShaderState::ShaderState(const ShaderState & source) :
	State(source), uMap(source.uMap), shader(source.shader) {
}

//! (dtor)
ShaderState::~ShaderState() {
	setShader( nullptr);
	uMap.clear();
}

//! ---|> [Node]
ShaderState * ShaderState::clone() const {
	return new ShaderState(*this);
}

void ShaderState::setUniform(const Rendering::Uniform & value) {
	uMap[value.getName()] = Rendering::Uniform(value);
}

void ShaderState::setUniform(FrameContext & context, const Rendering::Uniform & value) {
	setUniform(value);
	shader->setUniform(context.getRenderingContext(),value,true,true); // warn if uniform is not defined in shader and force at least one try on applying the uniform
}

Rendering::Uniform ShaderState::getUniform(const std::string & name) const {
	auto it = uMap.find(name);
	return it == uMap.end() ? Rendering::Uniform() : Rendering::Uniform(it->second);
}

//! ---|> [State]
State::stateResult_t ShaderState::doEnableState(FrameContext & context, Node *, const RenderParam & rp) {
	if( !shader || rp.getFlag(NO_SHADING))
		return State::STATE_SKIPPED;

	auto & rCtxt = context.getRenderingContext();

	// push current shader to stack
	rCtxt.pushAndSetShader(shader.get());
	
	if( !rCtxt.isShaderEnabled(shader.get()) ){
		rCtxt.popShader();
		deactivate();
		WARN("Enabling ShaderState failed. State has been deactivated!");
		return State::STATE_SKIPPED;
	}
	
	for(const auto & uniformEntry : uMap) 
		shader->setUniform(rCtxt, uniformEntry.second);
	return State::STATE_OK;
}

//! ---|> [State]
void ShaderState::doDisableState(FrameContext & context, Node *, const RenderParam & /*rp*/) {
	// restore old shader
	context.getRenderingContext().popShader();
}

}
