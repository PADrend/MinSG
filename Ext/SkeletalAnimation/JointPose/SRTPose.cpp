/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "SRTPose.h"

#include <Geometry/Vec3.h>
#include <Geometry/Quaternion.h>

#include "../Joints/ArmatureNode.h"
#include "../Joints/AbstractJoint.h"
#include "../Util/SkeletalAnimationUtils.h"

using namespace Geometry;

namespace MinSG {

SRTPose::SRTPose(AbstractJoint *joint) : AbstractPose(joint), animationData(std::vector<SRT>())
{
    init(std::deque<double>(), std::deque<double>(), std::deque<uint32_t>(), 0);
}

SRTPose::SRTPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, AbstractJoint *joint) :
    AbstractPose(joint), animationData(std::vector<SRT>())
{
    init(_values, _timeline, _interpolationTypes, 0.0);
}

SRTPose::SRTPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime,
                 AbstractJoint *joint) : AbstractPose(joint), animationData(std::vector<SRT>())
{
    init(_values, _timeline, _interpolationTypes, _startTime);
}

void SRTPose::init(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime)
{
    startTime = _startTime;
    
    status = STOPPED;
    
    maxPoseCount = 0;
    
    if(getBindetJoint() != nullptr && keyframes.size() > 0) 
        getBindetJoint()->setRelTransformation(keyframes[0].toSRT());
    
    setValues(_values, _timeline, _interpolationTypes);
}

void SRTPose::setValues(std::deque<Geometry::Matrix4x4> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes)
{
    if(_values.empty() || _timeline.empty() || _interpolationTypes.empty())
        return;
    double timeOffset = _timeline[0];
    for(auto time : _timeline)
        timeline.emplace_back(time-timeOffset);
    
    std::copy(_interpolationTypes.begin(), _interpolationTypes.end(), std::back_inserter(interpolationTypes));
    std::copy(_values.begin(), _values.end(), std::back_inserter(keyframes));
    
    animationData.clear();
    if(keyframes.size() == 0)
        return;
    
    for(const auto keyframe : keyframes) {
        animationData.emplace_back(keyframe.toSRT());
    }
}

void SRTPose::setValues(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes)
{
    if(_values.empty() || _timeline.empty() || _interpolationTypes.empty())
        return;
    
    std::deque<Geometry::Matrix4x4> mats;
    if(_values.size()/16 == _timeline.size())
    {
        for(uint32_t i=0; i<_values.size()/16; ++i)
        {
            Geometry::Matrix4x4 mat;
            for(uint32_t j=0; j<16; ++j)
                mat[j] = static_cast<float>(_values[i*16+j]);
            
            mats.emplace_back(Matrix4x4(mat));
        }
    }
    
    setValues(mats, _timeline, _interpolationTypes);
}

void SRTPose::addValue(Geometry::Matrix4x4 _value, double _time, uint32_t _interpolationType)
{
    addValue(_value, _time, _interpolationType, static_cast<uint32_t>(animationData.size()));
}

void SRTPose::addValue(Geometry::Matrix4x4 _value, double _time, uint32_t _interpolationType, uint32_t _index)
{    
    uint32_t i=0;
    timeline.emplace_back(0.0);
    interpolationTypes.emplace_back(0);
    keyframes.emplace_back(Matrix4x4());
    animationData.emplace_back(SRT());
    
    for(i=static_cast<uint32_t>(timeline.size()-2); i>=_index; --i)
    {
        timeline[i+1] = timeline[i];
        keyframes[i+1] = keyframes[i];
        
        animationData[i+1] = animationData[i];
        
        if(i>0)
            interpolationTypes[i] = interpolationTypes[i-1];
    }
    ++i;
    
    timeline[i] = _time;
    interpolationTypes[i] = _interpolationType;
    keyframes[i] = _value;
    Geometry::SRT _srt = keyframes[i].toSRT();
    animationData[i] = _srt;
}

void SRTPose::removeValue(uint32_t _index)
{
    if(_index > timeline.size())
        return;
    
    timeline.erase(timeline.begin()+_index);
    interpolationTypes.erase(interpolationTypes.begin()+_index);
    
    keyframes.erase(keyframes.begin()+_index);
}

SRTPose *SRTPose::clone() const
{
    std::deque<double> values;
    for(auto & elem : keyframes)
    {
        for(uint32_t j=0; j<16; ++j)
            values.emplace_back(elem.at(j));
    }
    
    SRTPose *myClone = new SRTPose(values, timeline, interpolationTypes, startTime, getBindetJoint());
    
    return myClone;
}

void SRTPose::bindToJoint(AbstractJoint *_node)
{
    if(_node == nullptr)
        return;
    
    AbstractPose::bindToJoint(_node);
}

void SRTPose::update(double timeSec)
{
    if(status == STOPPED || getBindetJoint() == nullptr || timeline.empty())
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
    
    currentInterpolationType = interpolationTypes[timeIndex];
    
    double interpolFactor = (timeSec-timeline[timeIndex]) / (timeline[timeIndex+1] - timeline[timeIndex]);
    getBindetJoint()->setRelTransformation(SRT(animationData[timeIndex], animationData[timeIndex+1], static_cast<float>(interpolFactor)));
}

void SRTPose::restart()
{
    if(timeline.empty())
        return;
    
    currentInterpolationType = interpolationTypes[0];
}

SRTPose* SRTPose::split(uint32_t start, uint32_t end)
{
    if(start > end)
        return nullptr;
    
    std::deque<double> splitVal;
    std::deque<double> splitTime;
    std::deque<uint32_t> splitInterpol;
    
    for(uint32_t i=start; i<=end; ++i)
    {        
        splitTime.emplace_back(timeline[i]);
        splitInterpol.emplace_back(interpolationTypes[i]);
        for(uint32_t j=0; j<16; ++j)
            splitVal.emplace_back(keyframes[i].at(j));
    }
    
    return new SRTPose(splitVal, splitTime, splitInterpol, 0, getBindetJoint());
}

}

#endif
