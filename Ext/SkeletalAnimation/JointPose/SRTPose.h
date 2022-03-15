/*
 This file is part of the MinSG library extension SkeletalAnimation.
 Copyright (C) 2011-2012 Lukas Kopecki
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef PADrend_SRTPose_h
#define PADrend_SRTPose_h

#include "AbstractPose.h"

#include <vector>

namespace Geometry {
	template<typename _T> class _SRT;
	typedef _SRT<float> SRT;
}

namespace MinSG {
	class ArmatureNode;
	
	class SRTPose : public AbstractPose
	{
	private:        
		std::vector<Geometry::SRT> animationData;
		
	protected:
		
		void init(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime) override;
		
	public:
		MINSGAPI SRTPose(AbstractJoint *joint);
		
		MINSGAPI SRTPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, AbstractJoint *joint);
		MINSGAPI SRTPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime,
				AbstractJoint *joint);
		
		/****************************************************************
		 *              ---|> AbstractPose
		 ****************************************************************/
		void setValues(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes) override;
		void setValues(std::deque<Geometry::Matrix4x4> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes) override;
		
		MINSGAPI void addValue(Geometry::Matrix4x4 _value, double _time, uint32_t _interpolationType);
		void addValue(Geometry::Matrix4x4 _value, double _time, uint32_t _interpolationType, uint32_t _index) override;
		
		void removeValue(uint32_t _index) override;
	
		void update(double timeSec) override;
		void restart() override;
		
		SRTPose* split(uint32_t start, uint32_t end) override;
		
		void bindToJoint(AbstractJoint *_node) override;
		
		/****************************************************************
		 *              ---|> State
		 ****************************************************************/
		SRTPose *clone()const override;
	};
}

#endif
#endif
