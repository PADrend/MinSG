/*
	This file is part of the MinSG library.
	Copyright (C) 2019 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "GeometrySerialization.h"

#include <Util/GenericAttributeSerialization.h>
#include <Util/GenericAttribute.h>

#include <Geometry/Box.h>
#include <Geometry/Frustum.h>
#include <Geometry/Line.h>
#include <Geometry/Matrix3x3.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Plane.h>
#include <Geometry/PointOctree.h>
#include <Geometry/Quaternion.h>
#include <Geometry/Rect.h>
#include <Geometry/Sphere.h>
#include <Geometry/SRT.h>
#include <Geometry/Tetrahedron.h>
#include <Geometry/Triangle.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec4.h>

namespace MinSG {
using namespace Util;
	
template<typename> static const std::string getGATypeName();

//typedef typename Util::WrapperAttribute<ObjType> attr_t;
template<> const std::string getGATypeName<Geometry::Box>() { return "Box"; }
//template<> const std::string getGATypeName<Geometry::Frustum>() { return "Frustum"; }
template<> const std::string getGATypeName<Geometry::Line3f>() { return "Line3"; }
template<> const std::string getGATypeName<Geometry::Segment3f>() { return "Segment3"; }
template<> const std::string getGATypeName<Geometry::Ray3f>() { return "Ray3"; }
template<> const std::string getGATypeName<Geometry::Matrix3x3>() { return "Matrix3x3"; }
template<> const std::string getGATypeName<Geometry::Matrix4x4>() { return "Matrix4x4"; }
template<> const std::string getGATypeName<Geometry::Plane>() { return "Plane"; }
template<> const std::string getGATypeName<Geometry::Quaternion>() { return "Quaternion"; }
template<> const std::string getGATypeName<Geometry::Rect>() { return "Rect"; }
template<> const std::string getGATypeName<Geometry::Sphere_f>() { return "Sphere"; }
template<> const std::string getGATypeName<Geometry::SRT>() { return "SRT"; }
template<> const std::string getGATypeName<Geometry::Tetrahedron<float>>() { return "Tetrahedron"; }
template<> const std::string getGATypeName<Geometry::Triangle<Geometry::Vec3>>() { return "Triangle"; }
template<> const std::string getGATypeName<Geometry::Vec2>() { return "Vec2"; }
template<> const std::string getGATypeName<Geometry::Vec3>() { return "Vec3"; }
template<> const std::string getGATypeName<Geometry::Vec4>() { return "Vec4"; }

template<typename Type>
GenericAttributeSerialization::serializer_type_t serializeGeometryGA(const GenericAttributeSerialization::serializer_parameter_t & attributeAndContext) {
	auto attribute = dynamic_cast<const WrapperAttribute<Type>*>(attributeAndContext.first);
	std::ostringstream stream;
	auto obj = attribute->get();
	stream << obj;
	return std::make_pair(getGATypeName<Type>(), stream.str());
}

template<typename Type>
WrapperAttribute<Type>* unserializeGeometryGA(const GenericAttributeSerialization::unserializer_parameter_t & contentAndContext) {
	Type obj;
	std::istringstream stream(contentAndContext.first);
	stream >> obj;
	return new WrapperAttribute<Type>(obj);
}

//-------------------

template<typename Type>
static void registerGeometrySerializer() {
	GenericAttributeSerialization::registerSerializer<WrapperAttribute<Type>>(getGATypeName<Type>(), serializeGeometryGA<Type>, unserializeGeometryGA<Type>);
}

//-------------------

static const bool geometrySerializationInitialized = initGeometrySerialization();

//-------------------

bool initGeometrySerialization() {
	if(geometrySerializationInitialized)
		return false;	
	registerGeometrySerializer<Geometry::Box>();
	//registerGeometrySerializer<Geometry::Frustum>();
	registerGeometrySerializer<Geometry::Line3f>();
	registerGeometrySerializer<Geometry::Segment3f>();
	registerGeometrySerializer<Geometry::Ray3f>();
	registerGeometrySerializer<Geometry::Matrix3x3>();
	registerGeometrySerializer<Geometry::Matrix4x4>();
	registerGeometrySerializer<Geometry::Plane>();
	registerGeometrySerializer<Geometry::Quaternion>();
	registerGeometrySerializer<Geometry::Rect>();
	registerGeometrySerializer<Geometry::Sphere_f>();
	registerGeometrySerializer<Geometry::SRT>();
	registerGeometrySerializer<Geometry::Tetrahedron<float>>();
	registerGeometrySerializer<Geometry::Triangle<Geometry::Vec3>>();
	registerGeometrySerializer<Geometry::Vec2>();
	registerGeometrySerializer<Geometry::Vec3>();
	registerGeometrySerializer<Geometry::Vec4>();
	return true;
}
	
} /* MinSG */
