/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "DirectionalInterpolator.h"
#include "ValuatedRegionNode.h"
#include <Geometry/Frustum.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Rendering/OcclusionQuery.h>
#include <Util/GenericAttribute.h>

using namespace Util;
using namespace Geometry;

namespace MinSG {

DirectionalInterpolator::DirectionalInterpolator(){
	assignement_sides_valueIndice[LEFT_SIDE]=0;
	assignement_sides_valueIndice[BOTTOM_SIDE]=1;
	assignement_sides_valueIndice[FRONT_SIDE]=2;
	assignement_sides_valueIndice[RIGHT_SIDE]=3;
	assignement_sides_valueIndice[TOP_SIDE]=4;
	assignement_sides_valueIndice[BACK_SIDE]=5;
	//ctor
}

DirectionalInterpolator::~DirectionalInterpolator(){
	//dtor
}
/**
 * (internal)
 * \note Do not alter or delete the returned value - it's only a reference.
 */
GenericAttribute * DirectionalInterpolator::getValueForSide(ValuatedRegionNode * node,side_t side){
	auto i= assignement_sides_valueIndice.find(side);
	if(i==assignement_sides_valueIndice.end())
		return nullptr;
	int sideIndex=(*i).second;
	if(sideIndex<0)
		return nullptr;

	GenericAttributeList * allValues=dynamic_cast<GenericAttributeList * >(node->getValue());
	if(!allValues)
		return nullptr;

	return allValues->at(sideIndex);
}
/**
 * This implemetation: Interpolate(=blend) Values according to the projected size of the cube sides.
 * \note The returned value is owned by the caller - you have to remove it!
 */
GenericAttribute * DirectionalInterpolator::calculateValue(Rendering::RenderingContext & renderingContext,
		ValuatedRegionNode * vNode,
		const Frustum & frustum,
		float measurementApertureAngle_deg/*=90.0*/) {

	// get values
	GenericAttribute * values[6];
	for(int i=0;i<6;i++){
		values[i]=getValueForSide(vNode,static_cast<side_t>(i));
	}

	// get side ratio
	float ratio[6];
	calculateRatio(renderingContext, ratio,frustum,measurementApertureAngle_deg);
	float sum=0;
	for(int i=0;i<6;i++){
		if(values[i]!=nullptr)
			sum+=ratio[i];
	}

	// calculate value
	// todo? blend(genericAttributeList , factors);
	float value=0;
	if(sum>0){
		for(int i=0;i<6;i++){
			if(values[i] == nullptr)
				continue;
			value+=values[i]->toFloat()*ratio[i];
		}
		value/=sum;
	}
	return GenericAttribute::createNumber(value);
}


void DirectionalInterpolator::calculateRatio(Rendering::RenderingContext & renderingContext, float ratio[6],const Frustum & frustum,float measurementApertureAngle_deg/*=90.0*/ ){

	{ // Setup OpenGL
		renderingContext.pushMatrix_cameraToClipping();
		renderingContext.setMatrix_cameraToClipping(frustum.getProjectionMatrix());

		Matrix4x4 m;
		m.lookAt(Vec3(0,0,0), frustum.getDir(), frustum.getUp());
		renderingContext.pushMatrix_modelToCamera();
		renderingContext.setMatrix_modelToCamera(m);

		renderingContext.pushAndSetColorBuffer(Rendering::ColorBufferParameters(false, false, false, false));
		renderingContext.pushAndSetLighting(Rendering::LightingParameters(false));
		renderingContext.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::LESS));
		renderingContext.pushAndSetCullFace(Rendering::CullFaceParameters());
	}

	// generate queries
	Rendering::OcclusionQuery queries[6];

	{ // start tests
		float tang = static_cast<float>(std::tan(measurementApertureAngle_deg*0.5 * (M_PI/180.0) ));
		float a=5.0f;
		float b=a*tang;
		float sides[6][12]={
			{-a,+b,+b, -a,+b,-b, -a,-b,-b, -a,-b,+b }, // (left)
			{+b,+b,-a, -b,+b,-a, -b,-b,-a, +b,-b,-a }, // (front)
			{+a,+b,+b, +a,+b,-b, +a,-b,-b, +a,-b,+b }, // (right)
			{+b,+b,+a, -b,+b,+a, -b,-b,+a, +b,-b,+a }, // (back)
			{+b,-a,+b, -b,-a,+b, -b,-a,-b, +b,-a,-b }, // (bottom)
			{+b,+a,+b, -b,+a,+b, -b,+a,-b, +b,+a,-b }  // (top)
		};
//        std::list<ValuatedRegionNode::Color>::iterator cit=vNode->colors.begin();
		Rendering::OcclusionQuery::enableTestMode(renderingContext);
		for(int i=0;i<6;i++){
//            if(values[i] == nullptr)
//                continue;
			queries[i].begin();
//            if(cit!=vNode->colors.end()){
//                glColor4f(cit->r, cit->g, cit->b, cit->a);
//                cit++;
//            }
			Rendering::drawQuad(
				renderingContext,
				Geometry::Vec3f(sides[i] + 0),
				Geometry::Vec3f(sides[i] + 3),
				Geometry::Vec3f(sides[i] + 6),
				Geometry::Vec3f(sides[i] + 9)
			);
			queries[i].end();
		}
		Rendering::OcclusionQuery::disableTestMode(renderingContext);
	}

	{ // restore gl matrices and state
		renderingContext.popCullFace();
		renderingContext.popDepthBuffer();
		renderingContext.popLighting();
		renderingContext.popColorBuffer();
		renderingContext.popMatrix_modelToCamera();
		renderingContext.popMatrix_cameraToClipping();
	}

	 // query results
//    std::cout <<"\r";
	unsigned int samples[6]={0,0,0,0,0,0};
	unsigned int sum=0;
	for(int i=0;i<6;i++){
//        if(values[i] == nullptr)
//            continue;
		samples[i] = queries[i].getResult();
		sum+=samples[i];
//        std::cout << samples[i]<<" , ";
	}
//    std::cout << sum<< "     ";

	if(sum>0){
		for(int i=0;i<6;i++)
			ratio[i]= static_cast<float>(samples[i])/static_cast<float>(sum);
	}else{
		for(int i=0;i<6;i++)
			ratio[i]=0;
	}
}

}
