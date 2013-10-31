/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */


#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "AbstractPose.h"

namespace MinSG {

AbstractPose::AbstractPose(AbstractJoint *joint) : node(joint), keyframes(std::deque<Geometry::Matrix4x4>()), timeline(std::deque<double>()),
                                                        interpolationTypes(std::deque<uint32_t>()), currentInterpolationType(0), status(STOPPED),
                                                        startTime(0), maxPoseCount(0)
{
    
}

bool AbstractPose::setTimeline(std::deque<double> _timeline, bool relative)
{
    if(_timeline.empty())
        return false;
    
    if(_timeline.size() != timeline.size())
        return false;
    
    double offset = 0.0;
    if(relative)
        offset = _timeline[0];
    
    timeline.clear();
    for(const auto time : _timeline)
        timeline.emplace_back(time - offset);    
    
    return true;
}

void AbstractPose::bindToJoint(AbstractJoint *_node)
{
    if(_node != nullptr)
        node = _node;
}

void AbstractPose::play()
{
    restart();
    
	status = RUNNING;
}

void AbstractPose::stop()
{
    restart();
    
    status = STOPPED;
}

}

#endif

