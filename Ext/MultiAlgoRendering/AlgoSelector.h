/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef MAR_ALGOSELECTOR_H
#define MAR_ALGOSELECTOR_H

#include "../../Core/States/State.h"
#include "MultiAlgoGroupNode.h"
#include "SampleContext.h"
#include "Utils.h"

namespace MinSG {

namespace MAR {

class AlgoSelector : public State {
		PROVIDES_TYPE_NAME(AlgoSelector)

	public:
		typedef int32_t INTERPOLATION_MODE_t ;
        enum INTERPOLATION_MODE : INTERPOLATION_MODE_t {MAX4, BARY, NEAREST};

        typedef int32_t REGULATION_MODE_t ;
        enum REGULATION_MODE : REGULATION_MODE_t {ABS, REL, CYCLE};

		//! @name Serialization
		//@{
		static AlgoSelector * create(std::istream & in) {
			AlgoSelector * as = new AlgoSelector();
			as->sampleContext = SampleContext::create(in);
			FAIL_IF(MAR::read<uint32_t>(in)!=142857);
			return as;
		}
		void write(std::ostream & out)const {
			sampleContext->write(out);
			MAR::write<uint32_t>(out, 142857);
		}
		//@}

		MINSGAPI AlgoSelector();
		AlgoSelector(const AlgoSelector & source) = delete;
		MINSGAPI virtual ~AlgoSelector();

		MINSGAPI void setSampleContext(SampleContext * sc);
        SampleContext * getSampleContext() const { return sampleContext.get(); }

        void setRegulationMode(REGULATION_MODE m){ regulationMode = m; }
        REGULATION_MODE getRegulationMode()const{ return regulationMode; }

        void setInterpolationMode(INTERPOLATION_MODE m){ interpolationMode = m; }
        INTERPOLATION_MODE getInterpolationMode()const{ return interpolationMode; }

        void setRenderMode(MultiAlgoGroupNode::AlgoId id){renderMode = id;}
        MultiAlgoGroupNode::AlgoId getRenderMode() const {return renderMode;}

		void setTargetTime(float millis){targetTime = millis/1000;};
		float getTargetTime() const {return targetTime*1000;};

        MINSGAPI void waitForLP();

        MINSGAPI void keepSamples(uint32_t amount);

		MINSGAPI AlgoSelector * clone() const override;

        //! eval functions
        MINSGAPI uint32_t countMAGNsInFrustum()const;
        MINSGAPI float getTimReal()const;
        MINSGAPI float getTimCalc()const;
        MINSGAPI float getTimMini()const;
        MINSGAPI float getTimMaxi()const;
        MINSGAPI float getTimLPIn()const;
        MINSGAPI float getTimUser()const;
        MINSGAPI float getErrCalc()const;
        MINSGAPI uint32_t getAlgoUsage(MultiAlgoGroupNode::AlgoId algo)const;

	private:
        REGULATION_MODE regulationMode;
		INTERPOLATION_MODE interpolationMode;
        MultiAlgoGroupNode::AlgoId renderMode;
		float targetTime;
		Util::Reference<SampleContext> sampleContext;
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};

}
}
#endif // ALGOSELECTOR_H
#endif // MINSG_EXT_MULTIALGORENDERING
