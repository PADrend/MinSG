/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#ifndef PADrend_MatrixPose_h
#define PADrend_MatrixPose_h

#include "AbstractPose.h"

#include <Util/TypeNameMacro.h>
#include <Geometry/Matrix4x4.h>
#include <string>
#include <deque>

namespace MinSG {
		class AbstractJoint;
}

namespace MinSG
{
	class MatrixPose : public AbstractPose
	{
		PROVIDES_TYPE_NAME(MatrixPose)

	private:
		Geometry::Matrix4x4 startMat;
		Geometry::Matrix4x4 endMat;

		Geometry::Matrix4x4 oldMat;
				
		protected:
				/****************************************************************
				 *              ---|> AbstractPose
				 ****************************************************************/
				void init(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime) override;

	public:
		MINSGAPI MatrixPose(AbstractJoint *joint);
		MINSGAPI MatrixPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, AbstractJoint *joint);
		MINSGAPI MatrixPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime, AbstractJoint *joint);

				/****************************************************************
				 *              ---|> AbstractPose
				 ****************************************************************/
		void setValues(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes) override;
		void setValues(std::deque<Geometry::Matrix4x4> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes) override;

		MINSGAPI void addValue(Geometry::Matrix4x4 _value, double _timeline, uint32_t _interpolationType);
		void addValue(Geometry::Matrix4x4 _value, double _timeline, uint32_t _interpolationType, uint32_t _index) override;
		
		MINSGAPI void updateValue(Geometry::Matrix4x4 _value, uint32_t index);
		
		void removeValue(uint32_t _index) override;
				
		void update(double timeSec) override;
		void restart() override;

		MatrixPose *split(uint32_t start, uint32_t end) override;
				
				/****************************************************************
				 *              ---|> State
				 ****************************************************************/
		MatrixPose *clone()const override;
	};

}

#endif
#endif
