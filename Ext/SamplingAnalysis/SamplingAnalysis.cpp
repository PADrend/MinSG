/*
	This file is part of the MinSG library extension SamplingAnalysis.
	Copyright (C) 2011-2012 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SAMPLING_ANALYSIS

#include "SamplingAnalysis.h"
#include <iostream>
#include <cmath>
#include <Geometry/Vec2.h>
#include <Geometry/Box.h>
#include <Geometry/Point.h>
#include <Geometry/PointOctree.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Graphics/Bitmap.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

namespace MinSG {
namespace SamplingAnalysis{


using namespace Geometry;

Histogram1D * createDistanceHistogram(const std::vector<Geometry::Vec3> & positions,const uint32_t numBuckets,float maxDistance/*=-1*/){
	auto hist=new Histogram1D(numBuckets);

	if(positions.empty())
		return hist;
	// calculate maxDistance
	if(maxDistance<=0){
		Box bb;
		bb.invalidate();
		for(auto & position : positions)
			bb.include(position);
		maxDistance = bb.getDiameter();
		if(maxDistance==0)
			return hist;
	}

	hist->maxValue = maxDistance;
	const float invBucketSize = static_cast<float>(numBuckets) / maxDistance;
	for(auto referenceIt=positions.begin();referenceIt!=positions.end();++referenceIt){
		const Vec3 referencePoint(*referenceIt);
		for(auto it=referenceIt+1;it!=positions.end();++it){
			const uint32_t bucketIndex = static_cast<uint32_t>(((*it)-referencePoint).length() * invBucketSize);
			if(bucketIndex<numBuckets){
				hist->buckets[ bucketIndex ]+=2;
				hist->sum+=2;
			}else{
				hist->overflow+=2;
			}
		}
	}
	return hist;
}

Histogram1D * createAngleHistogram(const std::vector<Geometry::Vec3> & positions,const uint32_t numBuckets){
	auto hist=new Histogram1D(numBuckets);
	hist->maxValue = 2.0f * M_PI;

	if(positions.empty())
		return hist;

	const float invBucketSize = static_cast<float>(numBuckets) / hist->maxValue;
	for(auto referenceIt=positions.begin();referenceIt!=positions.end();++referenceIt){
		const Vec3 referencePoint(*referenceIt);
		for(auto it=referenceIt+1;it!=positions.end();++it){
			const Vec3 v = ((*it)-referencePoint);
			const float length = v.length();
			if(length==0){
				hist->overflow+=2;
				continue;
			}
			const float a = v.x();
			const float b = v.z();

			const float alpha = b>=0 ? acosf(a/length) : -acosf(a/length);

//			std::cout << v.z()/length<< ">>"<<alpha*360.0/(2.0*M_PI) <<" ";
			++hist->buckets[ static_cast<uint32_t>( (alpha+M_PI+M_PI) * invBucketSize) % numBuckets ];
			++hist->buckets[ static_cast<uint32_t>( (alpha+M_PI+M_PI+M_PI) * invBucketSize) % numBuckets ];
			hist->sum+=2;
		}
	}
	return hist;
}



Histogram1D * createClosestPointDistanceHistogram(const std::vector<Geometry::Vec3> & positions,const uint32_t numBuckets){
	auto hist=new Histogram1D(numBuckets);

	if(positions.empty())
		return hist;

	// calculate bb
	Box bb;
	bb.invalidate();
	for(auto & position : positions)
		bb.include(position);
	hist->maxValue =  bb.getDiameter();
	if(hist->maxValue==0)
		return hist;

	// build octree
	typedef Geometry::PointOctree<Geometry::Point<Geometry::Vec3f> > octree_t;
	octree_t octree(bb,bb.getExtentMax()*0.01,20);

	for(auto & position : positions)
		octree.insert(octree_t::point_t(position));


	const float invBucketSize = static_cast<float>(numBuckets) / hist->maxValue;
	std::deque<octree_t::point_t> closestPoints;
	for(auto referencePoint : positions){
		
		closestPoints.clear();
		octree.getClosestPoints(referencePoint,2,closestPoints);
		if(closestPoints.size()!=2)
			continue;
		float distance2 = (referencePoint- closestPoints[0].getPosition()).lengthSquared();
		if(distance2<=0.00001) // skip the point itself
			distance2 = (referencePoint- closestPoints[1].getPosition()).lengthSquared();
		const uint32_t bucketIndex = static_cast<uint32_t>( sqrtf(distance2) * invBucketSize);

		if(bucketIndex<numBuckets){
			++hist->buckets[ bucketIndex ];
			++hist->sum;
		}else{
			++hist->overflow;
		}
	}
	return hist;
}


Util::Bitmap * create2dDistanceHistogram(const std::vector<Geometry::Vec3> & positions,const uint32_t numBuckets){
	Util::Reference<Util::Bitmap> result(new Util::Bitmap(numBuckets,numBuckets,Util::PixelFormat::RGBA));

	if(positions.empty())
		return result.detachAndDecrease();
	// ------------------

	int maxDistance;
	{	// calculate maxDistance
		Box bb;
		bb.invalidate();
		for(auto & position : positions)
			bb.include(position);
		maxDistance = bb.getDiameter();
		if(maxDistance==0)
			return result.detachAndDecrease();
	}

	std::cout << " -1- ";

	// one bucket per pixel
	std::vector<uint32_t> buckets(numBuckets*numBuckets);
	// ------------------

	{	// throw distances into buckets
		const float invBucketSize = static_cast<float>(numBuckets) / maxDistance;
		const Geometry::Vec2i center(numBuckets / 2,numBuckets / 2);
		for(auto referenceIt=positions.begin();referenceIt!=positions.end();++referenceIt){
			const Vec3 referencePoint(*referenceIt);

			for(auto it=referenceIt+1;it!=positions.end();++it){
				const Vec3 diffVec(((*it)-referencePoint) * invBucketSize);

				const Geometry::Vec2i pos1 = Geometry::Vec2i(diffVec.x(),diffVec.z()) + center;
				if(pos1.x()>=0 && pos1.x()<static_cast<int>(numBuckets) && pos1.y()>=0 && pos1.y()<static_cast<int>(numBuckets)){
					++buckets[pos1.x()+pos1.y()*numBuckets];
				}else{
					// overflow
				}
				const Vec2i pos2 = center - Vec2i(diffVec.x(),diffVec.z());
				if(pos2.x()>=0 && pos2.x()<static_cast<int>(numBuckets) && pos2.y()>=0 && pos2.y()<static_cast<int>(numBuckets)){
					++buckets[pos2.x()+pos2.y()*numBuckets];
				}else{
					// overflow
				}
			}
		}
	}
	std::cout << " -2- ";
	// ------------------

	// get maximal bucket size
	uint32_t maxSize = 0;
	for(auto & bucket : buckets){
		if( bucket>maxSize)
			maxSize=bucket;
	}
	std::cout << "maxValue:"<<maxSize << " -3- ";

	// ------------------
	{ 	// fill bitmap
		Util::Reference<Util::PixelAccessor> resultPixels( Util::PixelAccessor::create(result.get()) );
		uint32_t cursor = 0;
		const float colorScale = 1.0 / logf(static_cast<float>(maxSize));
		for(uint32_t row=0;row<numBuckets;++row){
			for(uint32_t column=0;column<numBuckets;++column){
				const float value = logf(buckets[cursor++]) * colorScale;
				resultPixels->writeColor(column,row,Util::Color4f(value,value,value,1.0 ));
			}
		}
	}
	return result.detachAndDecrease();
}


}
}

#endif // MINSG_EXT_SAMPLING_ANALYSIS
