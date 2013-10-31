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
        void init(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime);

	public:
		MatrixPose(AbstractJoint *joint);
		MatrixPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, AbstractJoint *joint);
		MatrixPose(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes, double _startTime, AbstractJoint *joint);

        /****************************************************************
         *              ---|> AbstractPose
         ****************************************************************/
		void setValues(std::deque<double> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes);
        void setValues(std::deque<Geometry::Matrix4x4> _values, std::deque<double> _timeline, std::deque<uint32_t> _interpolationTypes);

        void addValue(Geometry::Matrix4x4 _value, double _timeline, uint32_t _interpolationType);
        void addValue(Geometry::Matrix4x4 _value, double _timeline, uint32_t _interpolationType, uint32_t _index);
        
        void updateValue(Geometry::Matrix4x4 _value, uint32_t index);
        
        void removeValue(uint32_t _index);
        
		void update(double timeSec);
		void restart();

		MatrixPose *split(uint32_t start, uint32_t end);
        
        /****************************************************************
         *              ---|> State
         ****************************************************************/
		MatrixPose *clone()const;
	};

}

#endif
#endif
