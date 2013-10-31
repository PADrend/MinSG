/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "MatrixPose.h"
#include "../Joints/AbstractJoint.h"

#include <Geometry/Interpolation.h>


using namespace Geometry;
using namespace std;

namespace MinSG {

MatrixPose::MatrixPose(AbstractJoint *joint) : AbstractPose(joint)
{
    init(std::deque<double>(), std::deque<double>(), std::deque<uint32_t>(), 0.0);
}

MatrixPose::MatrixPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, AbstractJoint *joint) :
    AbstractPose(joint)
{
	init(_values, _timeline, _interpolationTypes, 0.0);
}

MatrixPose::MatrixPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime,
                       AbstractJoint *joint) : AbstractPose(joint)
{
	init(_values, _timeline, _interpolationTypes, _startTime);
}

void MatrixPose::init(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime)
{    
    timeline.clear();
    
	startTime = _startTime;
    
	status = STOPPED;
    
	maxPoseCount = 0;
    
    if(getBindetJoint() != nullptr && keyframes.size() > 0)
        getBindetJoint()->setMatrix(keyframes[0]);
    
	setValues(_values, _timeline, _interpolationTypes);
}

MatrixPose *MatrixPose::split(uint32_t start, uint32_t end)
{
	if(start > end)
		return nullptr;

	std::deque<double> splitVal;
	std::deque<double> splitTime;
	std::deque<uint32_t> splitInterpol;

	for(uint32_t i=start; i<=end; ++i)
	{
		for(uint32_t j=0; j<16; ++j)
			splitVal.push_back(keyframes[i].at(j));

		splitTime.push_back(timeline[i]);
		splitInterpol.push_back(interpolationTypes[i]);
	}

	return new MatrixPose(splitVal, splitTime, splitInterpol, 0, getBindetJoint());
}

void MatrixPose::setValues(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes)
{
    if(_values.empty() || _timeline.empty() || _interpolationTypes.empty())
        return;
    
    double timeOffset = _timeline[0];
    for(auto & elem : _timeline)
        timeline.push_back(elem-timeOffset);
    
    for(uint32_t i=0; i<_values.size()/16; ++i)
    {
        Geometry::Matrix4x4 mat;
        for(uint32_t j=0; j<16; ++j)
            mat[j] = _values[i*16+j];
        keyframes.push_back(mat);
    }

	std::copy(_interpolationTypes.begin(), _interpolationTypes.end(), std::back_inserter(interpolationTypes));

	maxPoseCount = std::max(maxPoseCount, getSize());
}

void MatrixPose::setValues(std::deque<Geometry::Matrix4x4> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes)
{
    if(_values.empty() || _timeline.empty() || _interpolationTypes.empty())
        return;
    
    for(const auto mat : _values)
        keyframes.emplace_back(Matrix4x4(mat));

    float timeOffset = _timeline[0];
    for(const auto item : _timeline)
        timeline.push_back(item-timeOffset);
    
    for(const auto inter : _interpolationTypes)
        interpolationTypes.emplace_back(inter);
}

void MatrixPose::addValue(Geometry::Matrix4x4 _value, double _timeline, uint32_t _interpolationType, uint32_t _index)
{
    if(_index == keyframes.size())
        addValue(_value, _timeline, _interpolationType);
    
    uint32_t i=0;
    keyframes.emplace_back(Geometry::Matrix4x4());
    timeline.emplace_back(0.0);
    interpolationTypes.emplace_back(0);
    for(i=keyframes.size()-2; i>=_index; --i)
    {
        keyframes[i+1] = keyframes[i];
        timeline[i+1] = timeline[i];
        if(i>0)
            interpolationTypes[i] = interpolationTypes[i-1];
    }
    ++i;
    
    keyframes[i] = _value;
    timeline[i] = _timeline;
    interpolationTypes[i] = _interpolationType;
}

void MatrixPose::addValue(Geometry::Matrix4x4 _value, double _timeline, uint32_t _interpolationType)
{    
    keyframes.emplace_back(_value);
    timeline.emplace_back(_timeline);
    interpolationTypes.emplace_back(_interpolationType);
}

void MatrixPose::updateValue(Geometry::Matrix4x4 _value, uint32_t index)
{
    if(index >= keyframes.size())
        return;
    
    keyframes[index] = _value;
}

void MatrixPose::removeValue(uint32_t _index)
{
    if(_index == keyframes.size())
        interpolationTypes.erase(interpolationTypes.begin()+_index-1);
    
    keyframes.erase(keyframes.begin()+_index);
    timeline.erase(timeline.begin()+_index);
}

MatrixPose *MatrixPose::clone() const
{
    std::deque<double> values;
    for(auto mat : keyframes)
        for(uint32_t i=0; i<16; ++i)
            values.emplace_back(mat.at(i));
        
	MatrixPose *myClone = new MatrixPose(values, timeline, interpolationTypes, startTime, getBindetJoint());

	return myClone;
}

void MatrixPose::update(double timeSec)
{
	if(status == STOPPED || getBindetJoint() == nullptr)
		return;

    timeSec -= startTime;
    
    if(timeSec > timeline.back())
    {
        status = STOPPED;
        return;
    }
    
    uint32_t timeIndex = 0;
    for(uint32_t i=0; i<timeline.size()-1; ++i)
    {
        if(timeSec > timeline[i] && timeSec <= timeline[i+1])
            timeIndex = i;
    }
    
    startMat = keyframes[timeIndex];
    endMat = keyframes[timeIndex+1];

    currentInterpolationType = interpolationTypes[timeIndex];

    double interpolFactor = (100/(timeline[timeIndex+1]-timeline[timeIndex]) * (timeSec-timeline[timeIndex]))/100;

    Geometry::Matrix4x4 tmpMat;
    for(uint32_t i=0; i<16; ++i)
    {
        if(startMat[i] != endMat[i])
        {
            if(currentInterpolationType == CONSTANT)
                tmpMat[i] = endMat[i];
            else if(currentInterpolationType == BEZIER)
                tmpMat[i] = Geometry::Interpolation::quadraticBezier(startMat[i], (startMat[i]+endMat[i])/2, endMat[i], interpolFactor);
            else
                tmpMat[i] = Geometry::Interpolation::linear(startMat[i], endMat[i], interpolFactor);
        }
        else
            tmpMat[i] = startMat[i];
    }
    
    getBindetJoint()->setMatrix(tmpMat);
}

void MatrixPose::restart()
{
	if(timeline.empty())
		return;
    
	startMat = keyframes[0];
	endMat = keyframes[1];
    
	currentInterpolationType = interpolationTypes[0];
    
    if(getBindetJoint() != nullptr)
        getBindetJoint()->setMatrix(startMat);
}

}

#endif
