/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef PADrend_AbstractPose_h
#define PADrend_AbstractPose_h

#include <string>
#include <deque>

#include <Geometry/Vec3.h>
#include <Geometry/Matrix4x4.h>

namespace MinSG {
	class AbstractJoint;
}

namespace MinSG {
	/*
	 *  @Brief entry point for animation data.
	 *
	 *  Animation data is described by keyframes, timeline and interpolation type.
	 *  - Keyframe is a 4x4 matrix describing the joint position at a given time.
	 *  - Timeline the relative times for each keyframe.
	 *  - Interpolationtype describing the interpolation type between each keyframe.
	 *
	 *  Keyframe size and timeline size have to be the same. If no interpolationtype is
	 *  given a linear interpolation will be used.
	 *
	 */
	class AbstractPose
	{
	private:
		mutable AbstractJoint *node;
		
	protected:
		mutable std::deque<Geometry::Matrix4x4> keyframes;
		std::deque<double> timeline;
		std::deque<uint32_t> interpolationTypes;
		
		uint32_t currentInterpolationType;
		
		int status;
		double startTime;
		
		uint32_t maxPoseCount;
		
		/*
		 *  Do all initializatoin here.
		 */
		virtual void init(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime) = 0;
		
	public:
		/****************************************************************
		 * enumeration representing the current playback state.
		 ****************************************************************/
		enum poseStatus { STOPPED, RUNNING};
		
		/****************************************************************
		 * Interpolation types
		 * - Linear, linear interpolation using slices between keyframes
		 * - Constant, jumps after the middle to the target keyframe
		 * - Bezier, Currently not implemented. Should smoothly interpolate
		 *   using bezier curves.
		 ****************************************************************/
		static const uint32_t LINEAR = 0;
		static const uint32_t CONSTANT = 1;
		static const uint32_t BEZIER = 2;
		
		MINSGAPI AbstractPose(AbstractJoint *joint);
		virtual ~AbstractPose(){ };
		
		/****************************************************************
		 * value access.
		 ****************************************************************/
		virtual void setValues(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes) = 0;
		virtual void setValues(std::deque<Geometry::Matrix4x4> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes) = 0;
	
		virtual void addValue(Geometry::Matrix4x4 _value, double _timeline, uint32_t _interpolationType, uint32_t _index) = 0;
		
		virtual void removeValue(uint32_t _index) = 0;
		
		virtual void update(double timeSec) = 0;
		virtual void restart() = 0;
		
		virtual AbstractPose* split(uint32_t start, uint32_t end) = 0;
		
		uint32_t getSize() { return timeline.size(); }
		
		std::deque<double> &getTimeline() { return timeline; }
		MINSGAPI bool setTimeline(std::deque<double> _timeline, bool relative=true);
		
		std::deque<uint32_t> *getInterpolationTypes() { return &interpolationTypes; }

		uint32_t getMaxPoseCount() { return maxPoseCount; }

		/****************************************************************
		 * play access.
		 ****************************************************************/
		MINSGAPI void play();
		MINSGAPI void stop();
		
		void setStartTime(double _startTime) { startTime = _startTime; }
		double getStartTime() { return startTime; }
		
		double getMinTime() const { return timeline.front(); }
		double getMaxTime() const { return timeline.back(); }
		double getDuration() const { return timeline.back() - timeline.front(); }

		/****************************************************************
		 * Joint connection.
		 ****************************************************************/
		virtual void bindToJoint(AbstractJoint *_node);
		AbstractJoint *getBindetJoint() const { return node; }        
		int getStatus() { return status; }
		
		std::deque<Geometry::Matrix4x4> & getKeyframes() { return keyframes; }
		
		/****************************************************************
		 *              ---|> State
		 ****************************************************************/
		virtual AbstractPose *clone() const = 0;
	};
}

#endif
#endif
