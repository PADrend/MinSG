/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_MULTIALGORENDERING

#include "AlgoSelector.h"

#include "SampleStorage.h"
#include "LP.h"

#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"

#include <Geometry/Vec3.h>
#include <Geometry/VecN.h>

#include <Util/GenericAttribute.h>
#include <Util/Timer.h>

#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/FBO.h>
#include <Rendering/RenderingContext/RenderingContext.h>

#include <cassert>

namespace MinSG {
namespace MAR {
	
struct ASContext {
    ASContext():initialized(false){}
	~ASContext()								= default;
	
	ASContext(const ASContext & o) 				= delete;
	ASContext(ASContext && o) 					= default;
	
	ASContext & operator=(const ASContext & o) 	= delete;
	ASContext & operator=(ASContext && o) 		= default;
	
	Util::Timer frameTimer;
	
	float frameTime;
	double lpTime;
	
	Util::Reference<SampleStorage> storage;
	uint32_t algoCount;
	uint32_t nodeCount;
	
	std::vector<bool> nodesInFrustum;
	std::vector<bool> lastNodesInFrustum;
	
	std::unique_ptr<LP> lp;
    std::unique_ptr<LP> lastLp;
	
    void init(Util::Reference<SampleContext> & sContext, FrameContext & ){
		if(!initialized){
			initialized = true;
			storage = sContext->getSampleStorage();
			algoCount = storage->getAlgoCount();
			nodeCount = storage->getNodeCount();
            nodesInFrustum.resize(nodeCount, 0);
            lastNodesInFrustum.resize(nodeCount, 0);
		}
	}
	
	void swap(){
		std::swap(lp,lastLp);
		std::swap(nodesInFrustum,lastNodesInFrustum);
	}
    void onEnableBegin(){
	}
    State::stateResult_t onEnableEnd(FrameContext & ){
        swap();

		frameTimer.reset();
		return State::STATE_OK;
	}
	void onDisable(FrameContext & fc){
		fc.getRenderingContext().finish();
		frameTimer.stop();
        frameTime = frameTimer.getSeconds();
    }

	bool initialized;
};

ASContext asContext;

void AlgoSelector::keepSamples(uint32_t amount){
    asContext.storage->keepSamples(amount);
}

//! eval functions

uint32_t magnsInFrustum = 0;
float timReal = 0;
float timCalc = 0;
float timMini = 0;
float timMaxi = 0;
float timLPIn = 0;
float errCalc = 0;
std::map<MultiAlgoGroupNode::AlgoId, uint32_t> algoUsage;

uint32_t AlgoSelector::countMAGNsInFrustum()const{
    return magnsInFrustum;
}
float AlgoSelector::getTimReal()const{
    return timReal;
}
float AlgoSelector::getTimCalc()const{
    return timCalc;
}
float AlgoSelector::getTimMini()const{
    return timMini;
}
float AlgoSelector::getTimMaxi()const{
    return timMaxi;
}
float AlgoSelector::getTimLPIn()const{
    return timLPIn;
}
float AlgoSelector::getTimUser()const{
    return this->getTargetTime()/1000;
}
float AlgoSelector::getErrCalc()const{
    return errCalc;
}
uint32_t AlgoSelector::getAlgoUsage(MultiAlgoGroupNode::AlgoId algo)const{
    return algoUsage[algo];
}

//! main functions

AlgoSelector::AlgoSelector() : State(), regulationMode(ABS), interpolationMode(MAX4), renderMode(MultiAlgoGroupNode::Auto), targetTime(0.1f) {
}

AlgoSelector::~AlgoSelector() {
}

State::stateResult_t AlgoSelector::doEnableState(FrameContext & fc, Node * /*node*/, const RenderParam & /*rp*/) {

    if(sampleContext.isNull()){ std::cerr << "sampleContext not set, skipping state\r"; return STATE_SKIPPED; }
	
    asContext.init(sampleContext, fc);
	asContext.onEnableBegin();

    magnsInFrustum = 0;
    timReal = 0;
    timCalc = 0;
    timMini = 0;
    timMaxi = 0;
    timLPIn = 0;
    errCalc = 0;
    algoUsage.clear();
    for(const auto & aid : asContext.storage->getAlgorithms())
        algoUsage[aid] = 0;

    if(renderMode != MultiAlgoGroupNode::Auto){
		for(size_t i = 0; i < asContext.nodeCount; ++i)
            asContext.storage->getNode(i)->setAlgorithm(renderMode);
        return asContext.onEnableEnd(fc);
	}
	
	// automatic mode
	
	// fetching samples

    auto camPos = fc.getCamera()->getWorldPosition();

    std::deque<SamplePoint> queue = asContext.storage->getStorage().getSortedClosestPoints(camPos, 4);
	// failback 1, no samples found
	if( queue.size()<1 ){
        std::cerr << "camera outside sampled area or no samples found\r";
		for(size_t i = 0; i < asContext.nodeCount; ++i)
            asContext.storage->getNode(i)->setAlgorithm(MultiAlgoGroupNode::AlgoId::ForceSurfels);
        return asContext.onEnableEnd(fc);
	}
	// failback 2 not enough samples found
	if(queue.size()<4 && interpolationMode != NEAREST ){
        std::cerr << "less than 4 samples found, switching to interpolation mode NEAREST\r";
		interpolationMode = NEAREST;
	}
	
	auto a = queue[0].data.get();
	
    std::vector<float> allErrors;
    std::vector<float> allTimes;
	
	switch(interpolationMode){
		case MAX4:
			{
// 				Util::Timer t;
				auto b = queue[1].data.get();
				auto c = queue[2].data.get();
				auto d = queue[3].data.get();
				auto iea = a->errors.cbegin(), ieb = b->errors.cbegin(), iec = c->errors.cbegin(), ied = d->errors.cbegin();
				auto ita = a->times.cbegin(), itb = b->times.cbegin(), itc = c->times.cbegin(), itd = d->times.cbegin();
                allErrors.reserve(a->errors.size());
                allTimes.reserve(a->times.size());
				while(iea != a->errors.cend()){
                    allErrors.push_back(std::max(std::max(*iea, *ieb),std::max(*iec, *ied)));
                    allTimes.push_back(std::max(std::max(*ita, *itb),std::max(*itc, *itd)));
					++iea;++ieb;++iec;++ied;
					++ita;++itb;++itc;++itd;
				}
// 				std::cerr << "max calulation took " << t.getMilliseconds() << " ms --- ";
			}
			break;
		case BARY:
			std::cerr << "BARY INTERPOLATION not implemented yet, using NEAREST\n";
		case NEAREST:
//            std::cerr << "queue size: " << queue.size() << std::endl;
//            std::cerr << "AS: request " << camPos << " :\n";
//            a->debug();
            allErrors.assign(a->errors.cbegin(),a->errors.cend());
            allTimes.assign(a->times.cbegin(),a->times.cend());
            break;
		default:
            throw std::logic_error("the roof is on fire");
    }
	
	uint32_t usedNodeCount = 0;
	
    for(uint32_t i = 0; i < asContext.nodeCount; ++i){
        bool b = fc.getCamera()->getFrustum().isBoxInFrustum(asContext.storage->getNode(i)->getWorldBB()) != Geometry::Frustum::OUTSIDE;
        asContext.nodesInFrustum[i] = b;
        if(b)
            magnsInFrustum++;
    }

//    std::cerr << " nodes (of (" << asContext.nodeCount << " vs " << asContext.nodesInFrustum.size() << ") in frustum: " << std::endl;
//    for(const auto & x : asContext.nodesInFrustum)
//        std::cerr << (x ? "true": "false");
//    std::cerr << std::endl;

    std::vector<float> filteredErrors,filteredTimes;
    for(uint32_t i = 0; i < asContext.nodeCount; i++){
        if(asContext.nodesInFrustum[i]){
            filteredTimes.insert(filteredTimes.end(), allTimes.begin() + i * asContext.algoCount, allTimes.begin() + (i+1) * asContext.algoCount);
            filteredErrors.insert(filteredErrors.end(), allErrors.begin() + i * asContext.algoCount, allErrors.begin() + (i+1) * asContext.algoCount);
            ++usedNodeCount;
        }
    }

    if(asContext.lastLp.get() == nullptr){
        double min = 0;
        for(auto it = filteredTimes.begin(); it != filteredTimes.end(); it += asContext.algoCount){
            min += *std::min_element(it, it + asContext.algoCount);
        }
        asContext.lpTime = min*1.01;
        asContext.lastLp.reset( new LP(usedNodeCount, asContext.algoCount, filteredErrors.data(), filteredTimes.data(), asContext.lpTime) );
    }

    waitForLP();

    float comTime = 0;
    float comError = 0;
    {
        auto l = asContext.lastLp.get()->getResult();
        auto il = l.cbegin();
        for(uint32_t i = 0; i < asContext.nodeCount; ++i){
            if(asContext.lastNodesInFrustum[i]){
                auto aid = asContext.storage->getAlgoId(*il);
                algoUsage[aid]++;
                asContext.storage->getNode(i)->setAlgorithm(aid);
                comTime += allTimes[i*asContext.algoCount + *il];
                comError += allErrors[i*asContext.algoCount + *il];
                il++;
            }
            else{
                asContext.storage->getNode(i)->setAlgorithm(MultiAlgoGroupNode::ForceSurfels);
                algoUsage[MultiAlgoGroupNode::ForceSurfels]++;
            }
        }
    }

    errCalc = comError;
    timCalc = comTime;

    switch(regulationMode){
    case ABS:
        asContext.lpTime = targetTime + (comTime - asContext.frameTime);
        //std::cerr << "in: " << asContext.lpTime*1000 << "\tout: " << comTime*1000 << "\treal: " << asContext.frameTime*1000;
        break;
    case REL:
        asContext.lpTime = targetTime * (comTime / asContext.frameTime);
        break;
    case CYCLE:
        if(asContext.frameTime > targetTime)
            asContext.lpTime /= 1.5;
        else if (asContext.frameTime < targetTime / 1.5)
            asContext.lpTime *= 1.1;
        break;
    default:
        throw std::logic_error("the roof is on fire");
    }

    double min = 0;
    double max = 0;
    for(auto it = filteredTimes.begin(); it != filteredTimes.end(); it += asContext.algoCount){
		auto mima = std::minmax_element(it, it + asContext.algoCount);
		min += *mima.first;
		max += *mima.second;
	}	

	asContext.lpTime = std::max(std::min(asContext.lpTime, max*1.01),min*1.01);

    timMini = min;
    timMaxi = max;
    timLPIn = asContext.lpTime;
    timReal = asContext.frameTime;

    //std::cerr << "\t\tmin: " << min*1000 << "\tmax " << max*1000 << "\tusing: " << asContext.lpTime*1000 << std::endl;

    asContext.lp.reset( new LP(usedNodeCount, asContext.algoCount, filteredErrors.data(), filteredTimes.data(), asContext.lpTime) );
    return asContext.onEnableEnd(fc);
}

void AlgoSelector::doDisableState(FrameContext & context, Node * /*node*/, const RenderParam & /*rp*/){
    asContext.onDisable(context);
}

void AlgoSelector::waitForLP(){
    Util::Timer t;
    bool waited = false;
    while(!asContext.lastLp.get()->hasResult()){  /* wait for result from last frame */
        t.resume();
        waited = true;
    }
    if(waited){
        std::cerr << "MAR: waited " << t.getMilliseconds() << " ms for LP\n";
    }
}

void AlgoSelector::setSampleContext(SampleContext * sc) { 
//    std::cerr << "setting new sample context:\n";
//    std::cerr << "\told:\t" << reinterpret_cast<size_t>(sampleContext.get()) << std::endl;
//    std::cerr << "\ttoset:\t" << reinterpret_cast<size_t>(sc) << std::endl;
	sampleContext = sc; 
    asContext.initialized = false;
//    std::cerr << "\tnew:\t" << reinterpret_cast<size_t>(sampleContext.get()) << std::endl;
	asContext.lp.reset(nullptr);
    asContext.lastLp.reset(nullptr);
}

AlgoSelector * AlgoSelector::clone() const {
	FAIL();
}

}
}

#endif //MINSG_EXT_MULTIALGORENDERING
