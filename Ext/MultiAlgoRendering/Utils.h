/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MAR_UTILS_H
#define MAR_UTILS_H

#include "../../Core/FrameContext.h"
#include "MultiAlgoGroupNode.h"
#include <Rendering/Draw.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Graphics/Color.h>
#include <Geometry/Box.h>

#include <functional>
#include <type_traits>

namespace MinSG {
namespace MAR {

template<typename T>
inline void debugDisplay(const T & container, FrameContext & frameContext, float alpha, std::function<const Geometry::Box( typename T::value_type )> bounds, std::function<const Util::Color4f( typename T::value_type )> color) {

	Rendering::DepthBufferParameters depth(true, false, Rendering::Comparison::LESS);
	frameContext.getRenderingContext().pushAndSetDepthBuffer(depth);

	Rendering::LightingParameters lighting;
	lighting.disable();
	frameContext.getRenderingContext().pushAndSetLighting(lighting);

	if(alpha < 1.0) {
		for(const auto & element : container) {
			Rendering::drawAbsWireframeBox(frameContext.getRenderingContext(), bounds(element), color(element));
		}

		Rendering::BlendingParameters blend(Rendering::BlendingParameters::CONSTANT_ALPHA, Rendering::BlendingParameters::ONE_MINUS_CONSTANT_ALPHA);
		blend.setBlendColor(Util::Color4f(0,0,0,alpha));
		frameContext.getRenderingContext().pushAndSetBlending(blend);
	}
	Rendering::CullFaceParameters cull;
	cull.disable();
	frameContext.getRenderingContext().pushAndSetCullFace(cull);

	for(const auto & element : container) {
		Rendering::drawAbsBox(frameContext.getRenderingContext(), bounds(element), color(element));
	}

	frameContext.getRenderingContext().popLighting();
	frameContext.getRenderingContext().popCullFace();
	frameContext.getRenderingContext().popDepthBuffer();
	if(alpha < 1.0)
		frameContext.getRenderingContext().popBlending();
}

//! reading from streams

template<typename T>
inline T read(std::istream & in){
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value || ( std::is_enum<T>::value && sizeof(T) == 4 ), "you have to implement a specialization of read for this datatype");
	T t;
	in.read(reinterpret_cast<char *>(&t), sizeof(T));
	return t;
}

template<>
inline Geometry::Vec3f read<Geometry::Vec3f>(std::istream & in){
	float x = read<float>(in);
	float y = read<float>(in);
	float z = read<float>(in);
	return Geometry::Vec3f(x,y,z);
}

template<>
inline Geometry::Box read<Geometry::Box>(std::istream & in){
	Geometry::Vec3f mini = read<Geometry::Vec3f>(in);
	Geometry::Vec3f maxi = read<Geometry::Vec3f>(in);
	return Geometry::Box(mini, maxi);
}

template<>
inline std::vector<float> read<std::vector<float>>(std::istream & in){
	size_t size = read<uint64_t>(in);
	std::vector<float> vec(size);
	in.read(reinterpret_cast<char *>(vec.data()), size * sizeof(float));
	return vec;
}

//! writing to streams

template <typename T>
void write(std::ostream & out, const T & v){
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value || ( std::is_enum<T>::value && sizeof(T) == 4 ), "you have to implement a specialization of write for this datatype");
	out.write(reinterpret_cast<const char *>(&v), sizeof(T));
}

template<>
inline void write<Geometry::Vec3f>(std::ostream & out, const Geometry::Vec3f & vec){
	write(out, vec.x());
	write(out, vec.y());
	write(out, vec.z());
}

template<>
inline void write<Geometry::Box>(std::ostream & out, const Geometry::Box & box){
	write(out, box.getMin());
	write(out, box.getMax());
}

template<>
inline void write<std::vector<float>>(std::ostream & out, const std::vector<float> & vec){
	write<uint64_t>(out, vec.size());
	out.write(reinterpret_cast<const char *>(vec.data()), vec.size() * sizeof(float));
}

}
}

#endif
