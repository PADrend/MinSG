/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/


#ifdef MINSG_EXT_MULTIALGORENDERING

#include "RegionEvaluator.h"

#include <Geometry/BoxHelper.h>

#include <Util/Graphics/Color.h>
#include <Util/Graphics/ColorLibrary.h>

#include <algorithm>

using Util::Color4ub;
using Util::GenericAttribute;

namespace MinSG {
namespace MAR {

/// RegionEvaluator


/// PolygonDensityEvaluator

const Util::Color4ub RegionEvaluator::colorFinished = Util::ColorLibrary::GREEN;
const Util::Color4ub RegionEvaluator::colorScheduled = Util::ColorLibrary::BLUE;
const Util::Color4ub RegionEvaluator::colorActive = Util::ColorLibrary::RED;

/// true --> b before a
bool PolygonDensityEvaluator::compare(const Region* a, const Region* b)
{
	float ret = a->getAttribute(Util::StringIdentifier("PolygonDensityPrio"))->toFloat() - b->getAttribute(Util::StringIdentifier("PolygonDensityPrio"))->toFloat();
	return ret==0 ? a<b : ret>0;
}

void PolygonDensityEvaluator::init(Region * r)
{
	if(!r->getParent())
		regionCount=0;
	if(r->hasChildren())
	{
		auto children = r->getChildren();
		for(const auto & child : children)
			init(child.get());
		r->setColor(colorFinished);
	}
	else
	{
		Util::GenericAttribute * ga = r->getAttribute(Util::StringIdentifier("PolygonDensityPrio"));
		if(!ga)
			calcPriority(r);
		evalQueue->insert(r);
		r->setColor(colorScheduled);
	}
}

void PolygonDensityEvaluator::evaluate(Region * r)
{

	float maxRegionCount = getAttribute(Util::StringIdentifier("#Regions to create"))->toUnsignedInt();

	if(regionCount >= maxRegionCount) {
		for(auto & x : *evalQueue)
			x->setColor(colorFinished);
		evalQueue->clear();
		return;
	}

	r->split(r->getAttribute(Util::StringIdentifier("PolygonDensitySplit"))->toInt(),r->getAttribute(Util::StringIdentifier("PolygonDensityRatio"))->toFloat());

	if(r->hasChildren()){
		auto children = r->getChildren();
		for(const auto & child : children)
		{
			calcPriority(child.get());
			child->setColor(colorScheduled);
			evalQueue->insert(child.get());
			regionCount++;
		}
	}

	r->setColor(colorFinished);
	if(!evalQueue->empty())
		(*evalQueue->begin())->setColor(colorActive);
}

void PolygonDensityEvaluator::calcPriority(Region * r)
{
	int testsPerAxis = getAttribute(Util::StringIdentifier("#Tests per axis"))->toUnsignedInt();
	Geometry::Box box = r->getBounds();

	PrioSplit px = calcPriority(Geometry::Helper::splitUpBox(box, testsPerAxis,1,1));
	PrioSplit py = calcPriority(Geometry::Helper::splitUpBox(box, 1,testsPerAxis,1));
	PrioSplit pz = calcPriority(Geometry::Helper::splitUpBox(box, 1,1,testsPerAxis));

	float volumePower = pow(box.getVolume(), getAttribute(Util::StringIdentifier("Volume Power"))->toFloat());
	px.prio *= volumePower;
	py.prio *= volumePower;
	pz.prio *= volumePower;

	float p = std::max(std::max(px.prio,py.prio),pz.prio);
	r->setAttribute(Util::StringIdentifier("PolygonDensityPrio"), GenericAttribute::createNumber<float>(p));

	float f = getAttribute(Util::StringIdentifier("Extent Power"))->toFloat();
	px.prio *= pow(box.getExtentX(),f);
	py.prio *= pow(box.getExtentY(),f);
	pz.prio *= pow(box.getExtentZ(),f);


	p = std::max(std::max(px.prio,py.prio),pz.prio);
	if(p==px.prio) {
		r->setAttribute(Util::StringIdentifier("PolygonDensitySplit"), GenericAttribute::createNumber<int>(0));
		r->setAttribute(Util::StringIdentifier("PolygonDensityRatio"), GenericAttribute::createNumber<float>(px.ratio));
	}
	else if(p==py.prio) {
		r->setAttribute(Util::StringIdentifier("PolygonDensitySplit"), GenericAttribute::createNumber<int>(1));
		r->setAttribute(Util::StringIdentifier("PolygonDensityRatio"), GenericAttribute::createNumber<float>(py.ratio));
	}
	else if(p==pz.prio) {
		r->setAttribute(Util::StringIdentifier("PolygonDensitySplit"), GenericAttribute::createNumber<int>(2));
		r->setAttribute(Util::StringIdentifier("PolygonDensityRatio"), GenericAttribute::createNumber<float>(pz.ratio));
	}
	else            FAIL();
}

RegionEvaluator::PrioSplit PolygonDensityEvaluator::calcPriority(const std::vector<Geometry::Box> & boxes) {
	std::vector<float> values;
	float prio=0;
	float sum=0;
	bool isConst = true;

	for(const auto & box : boxes){
		float f = calcDensity(box);
		values.push_back(f);
		if(f!=values.front())
			isConst = false;
		sum += f;
	}
	if(isConst)
		return PrioSplit(0.0,0.5);

	float avg = sum/values.size();

	for (auto & value : values)
		value -= avg;

	float left = 0;
	float maxDiff = 0;
	size_t maxDiffIndex = values.size() / 2;
	for (size_t i = 1; i < values.size(); i++) {
		left += values[i];
		if (std::abs(left) > maxDiff) {
			maxDiff = std::abs(left);
			maxDiffIndex = i;
		}
	}

	switch(getAttribute(Util::StringIdentifier("Test Mode"))->toUnsignedInt()) {
	case 0: // absolute difference
		prio = maxDiff;
		break;
	case 1: // relative difference
		prio = maxDiff / avg;
		break;
	default:
		FAIL();
	}

	return PrioSplit(prio,static_cast<double>(maxDiffIndex)/static_cast<double>(values.size()));
}

float PolygonDensityEvaluator::calcDensity(const Geometry::Box & b)
{
	return countPolygons(b) / b.getVolume();
}

/// RegionSizeEvaluator

/// true --> b before a
bool RegionSizeEvaluator::compare(const Region * a, const Region * b) {
	int x = a->getDepth() - b->getDepth();

	return x != 0 ? x > 0 : a > b;
}

void RegionSizeEvaluator::evaluate(Region * r)
{

	float maxRegionSize = getAttribute(Util::StringIdentifier("maxRegionSize"))->toFloat();
	uint32_t minTreeDepth = getAttribute(Util::StringIdentifier("minTreeDepth"))->toUnsignedInt();


	if(!r->hasChildren() && (r->getBounds().getExtentMax() > maxRegionSize || r->getDepth() < minTreeDepth))
	{
		r->splitCubeLike();
	}

	if(r->hasChildren()){
		auto children = r->getChildren();
		for(const auto & child : children)
		{
			if(child->getBounds().getExtentMax() > maxRegionSize || child->getDepth() < minTreeDepth) {
				evalQueue->insert(child.get());
				child->setColor(colorScheduled);
			}
			else {
				child->setColor(colorFinished);
			}
		}
	}

	r->setColor(colorFinished);

	if(!evalQueue->empty())
		(*evalQueue->begin())->setColor(colorActive);
}

void RegionSizeEvaluator::init(Region * r)
{
	if(r->hasChildren())
	{
		auto children = r->getChildren();
		for(const auto & child : children)
			init(child.get());
		r->setColor(colorFinished);
	}
	else if( r->getBounds().getExtentMax() > getAttribute(Util::StringIdentifier("maxRegionSize"))->toFloat() || r->getDepth() < getAttribute(Util::StringIdentifier("minTreeDepth"))->toUnsignedInt())
	{
		evalQueue->insert(r);
		r->setColor(colorScheduled);
	}
	else {
		r->setColor(colorFinished);
	}

}

/// PolygonCountEvaluator

/// true --> b before a
bool PolygonCountEvaluator::compare(const Region* a, const Region* b)
{
	return a<b;
}

void PolygonCountEvaluator::init(Region * r)
{
	if(r->hasChildren())
	{
		auto children = r->getChildren();
		for(const auto & child : children)
			init(child.get());
		r->setColor(colorFinished);
	}
	else
	{
		evalQueue->insert(r);
		r->setColor(colorScheduled);
	}
}

void PolygonCountEvaluator::evaluate(Region * r)
{
	if(r->hasChildren())
	{
		auto children = r->getChildren();
		for(const auto & child : children){
			evalQueue->insert(child.get());
			child->setColor(colorScheduled);
		}
		r->setColor(colorFinished);
		return;
	}

	float maxPolyCount = getAttribute(Util::StringIdentifier("maxPolyCount"))->toFloat();
	GenericAttribute * ga = r->getAttribute(Util::StringIdentifier("polyCount"));
	float polyCount;
	if(ga)
		polyCount = ga->toFloat();
	else
	{
		polyCount = countPolygons(r->getBounds());
	}

	if(polyCount < maxPolyCount)
	{
		r->setAttribute(Util::StringIdentifier("polyCount"), GenericAttribute::createNumber<float>(polyCount));
		r->setColor(colorFinished);
		return;
	}

	r->splitCubeLike();
	r->unsetAttribute(Util::StringIdentifier("polyCount"));
	auto children = r->getChildren();
	for(const auto & child : children)
	{
		evalQueue->insert(child.get());
		child->setColor(colorScheduled);
	}
	r->setColor(colorFinished);
}

}
}

#endif// MINSG_EXT_MULTIALGORENDERING
