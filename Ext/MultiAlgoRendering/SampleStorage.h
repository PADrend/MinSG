/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef MAR_SAMPLESTORAGE_H_
#define MAR_SAMPLESTORAGE_H_

#include "MultiAlgoGroupNode.h"
#include "Utils.h"

#include <vector>
#include <limits>

#include "../../Helper/StdNodeVisitors.h"

#include <Util/BidirectionalMap.h>

#include <Geometry/Point.h>
#include <Geometry/PointOctree.h>

#include <Rendering/Draw.h>

namespace MinSG {
namespace MAR {

struct SampleResult : public Util::ReferenceCounter<SampleResult> {

	MultiAlgoGroupNode::ref_t magn;
	MultiAlgoGroupNode::AlgoId algo;
	float pixel;
	float time;
	float error;

	SampleResult(MultiAlgoGroupNode * _magn, MultiAlgoGroupNode::AlgoId _algo, float _pixel, float _time, float _error) :
		ReferenceCounter_t(), magn(_magn) ,algo(_algo), pixel(_pixel), time(_time), error(_error) {
	}

private:
	
	MINSGAPI SampleResult();
	MINSGAPI SampleResult(const SampleResult &);
	MINSGAPI SampleResult & operator= (const SampleResult &);
};

struct SamplePointData : public Util::ReferenceCounter<SamplePointData> {

	typedef std::vector<float> container_t;

	container_t times;
	container_t errors;
	container_t pixels;

    MINSGAPI static int32_t counterSPD;

    explicit SamplePointData(uint32_t size): ReferenceCounter_t(), times(size, 0.0f), errors(size, 0.0f), pixels(size, 0.0f) {
    }

	//! @name Serialization
	//@{
		static Util::Reference<SamplePointData> create(std::istream & in){
			Util::Reference<SamplePointData> sd = new SamplePointData(0);
			sd->times = MAR::read< std::vector< float > >(in);
			sd->errors = MAR::read< std::vector< float > >(in);
			sd->pixels = MAR::read< std::vector< float > >(in);
			return sd;
		}
		
		void write(std::ostream & out)const{
			MAR::write(out, times);
			MAR::write(out, errors);
			MAR::write(out, pixels);
		}
	//@}

    void debug()const{
        std::cerr << "\nTimes: ";
        for(const auto & x : times)
            std::cerr << x*1000 << ", ";
        std::cerr << "\nError: ";
        for(const auto & x : errors)
            std::cerr << x << ", ";
        std::cerr << std::endl;
    }

private:
	
	MINSGAPI SamplePointData();
    SamplePointData(const SamplePointData &) = delete;
    SamplePointData & operator= (const SamplePointData &) = delete;
    SamplePointData(SamplePointData &&) = delete;
    SamplePointData & operator= (SamplePointData &&) = delete;
};

struct SamplePoint {

private:

    Geometry::Vec3f position;
    uint32_t id;

public:

    Util::Reference<SamplePointData> data;

    explicit SamplePoint (Geometry::Vec3f pos, uint32_t _id, SamplePointData * _data) : position(std::move(pos)), id(_id), data(_data) {
    }

	//! @name Serialization
	//@{
	static SamplePoint create(std::istream & in){
        uint32_t id = MAR::read< uint32_t >(in); // set this to 0 when loading old scenes
		Geometry::Vec3f pos = MAR::read< Geometry::Vec3f >(in);
		Util::Reference<SamplePointData> data = SamplePointData::create(in);
        return SamplePoint(pos, id, data.get());
	}
	void write(std::ostream & out)const{
        MAR::write(out, id);
        MAR::write(out, position);
		data->write(out);
	}
	//@}

    SamplePoint(const SamplePoint &) = default;
    SamplePoint(SamplePoint &&) = default;
    SamplePoint & operator= (SamplePoint &&) = default;
    SamplePoint & operator= (const SamplePoint &) = delete;

    const Geometry::Vec3f & getPosition() const {return position;}
    uint32_t getId() const {return id;}
};



class SampleStorage : public Util::ReferenceCounter<SampleStorage> {

public:
	SampleStorage(const Geometry::Box & bounds): ReferenceCounter_t(), storage(bounds, bounds.getExtentMax() / 100.0, 10) {

	}

	~SampleStorage() {}

	//! @name Serialization
	//@{
		static SampleStorage * create(std::istream & in) {
			
			Geometry::Box bounds = read<Geometry::Box>(in);
//			std::cerr << "box read: " << bounds << std::endl;
// 			float minBoxSize = read<float>(in);
// 			uint32_t maxNumPoints = read<uint32_t>(in);
			SampleStorage * ss = new SampleStorage(bounds);
			
			uint64_t size = MAR::read< uint64_t >(in);
			while(size-- > 0)
				ss->storage.insert(SamplePoint::create(in));
			
			size = MAR::read< uint64_t >(in);
			while(size-- > 0){
				MultiAlgoGroupNode::AlgoId id = MAR::read< MultiAlgoGroupNode::AlgoId >(in);
// 				std::cerr << "found algoid: " << id << std::endl;
				uint32_t index = MAR::read< uint32_t >(in);
				ss->algoIndices.insert(id, index);
			}
			return ss;
		}
		
		void write(std::ostream & out)const{
			MAR::write(out, storage.getBox());
//			std::cerr << "box written: " << storage.getBox() << std::endl;
// 			MAR::write<float>(out, storage.getMinBoxSize());
// 			MAR::write<uint32_t>(out, storage.getMaxNumPoints());
			std::deque<SamplePoint> sps;
			storage.collectPoints(sps);
			MAR::write<uint64_t>(out, sps.size());
			for(const auto & sp : sps)
				sp.write(out);
			MAR::write< uint64_t >(out, algoIndices.size());
			for(const auto & p : algoIndices){
				MAR::write< MultiAlgoGroupNode::AlgoId >(out, p.first);
				MAR::write< uint32_t >(out, p.second);
			}
		}
		//@}

private:
	
	typedef Util::BidirectionalUnorderedMap<MultiAlgoGroupNode::ref_t, uint32_t, Util::BidirectionalMapPolicies::hashByGet> nodeIndices_t;
	nodeIndices_t nodeIndices;
	
public:
	
	static const uint32_t INVALID_INDEX = 0xffffffff;
	
	void initNodeIndices(Node * root){
		assert(nodeIndices.empty());
		const auto magns = collectNodes<MultiAlgoGroupNode>(root);
		for(const auto & magn : magns)
			nodeIndices.insert(magn, magn->getNodeId());
	}
	
	void addNode(MultiAlgoGroupNode * node) {
		assert(storage.empty());
		node->setNodeId(nodeIndices.size());
		nodeIndices.insert(node, node->getNodeId());
	}
	
	uint32_t getIndex(MultiAlgoGroupNode * node) const {
		nodeIndices_t::const_iterator_left i = nodeIndices.findLeft(node);
		if(i != nodeIndices.endLeft())
			return i->second;
		return INVALID_INDEX;
	}
	
	MultiAlgoGroupNode * getNode(uint32_t index) const {
		nodeIndices_t::const_iterator_right i = nodeIndices.findRight(index);
		if(i != nodeIndices.endRight())
			return i->second.get();
		return nullptr;
	}

	std::vector<MultiAlgoGroupNode*> getNodes() const{
		std::vector<MultiAlgoGroupNode*> vec;
		for(const auto & p : nodeIndices)
			vec.push_back(p.first.get());
		return vec;
	}
	
	size_t getNodeCount() { return nodeIndices.size(); }

private:
	
	typedef Util::BidirectionalUnorderedMap<MultiAlgoGroupNode::AlgoId, uint32_t, Util::BidirectionalMapPolicies::hashEnum> algoIndices_t;
	algoIndices_t algoIndices;
	
public:
	
	void addAlgorithm(MultiAlgoGroupNode::AlgoId algo) {
		assert(storage.empty());
		algoIndices.insert(algo, algoIndices.size());
	}
	
	uint32_t getIndex(MultiAlgoGroupNode::AlgoId algo) const {
		algoIndices_t::const_iterator_left i = algoIndices.findLeft(algo);
		if(i != algoIndices.endLeft())
			return i->second;
		return INVALID_INDEX;
	}
	
	MultiAlgoGroupNode::AlgoId getAlgoId(uint32_t index) const {
		algoIndices_t::const_iterator_right i = algoIndices.findRight(index);
		if(i != algoIndices.endRight())
			return i->second;
		throw std::logic_error("SampleStorage::getAlgoId: unknown algorithm");
	}
	
	std::vector<MultiAlgoGroupNode::AlgoId> getAlgorithms() const {
		std::vector<MultiAlgoGroupNode::AlgoId> vec;
		for(const auto & p : algoIndices)
			vec.push_back(p.first);
		return vec;
	}
	
	size_t getAlgoCount() { return algoIndices.size(); }

private:
	
	typedef Geometry::PointOctree<SamplePoint> storage_t;
	storage_t storage;

public:
	
    void addResults(const Geometry::Vec3f & position, uint32_t id, const std::deque<SampleResult::ref_t> & results) {
        SamplePoint p (position, id, new SamplePointData(algoIndices.size() * nodeIndices.size()));
		for(const auto & result : results) {
			uint32_t index = nodeIndices.findLeft(result->magn)->second;
			index *= algoIndices.size();
			index += algoIndices.findLeft(result->algo)->second;
			p.data->errors[index] += result->error;
			p.data->times[index] += result->time;
			p.data->pixels[index] += result->pixel;
		}

//        std::cerr << "insert " << position << " :\n";
//        p.data->debug();
        storage.insert(p);
//        std::cerr << "request " << position << " :\n";
//        storage.getSortedClosestPoints(position, 4)[0].data->debug();
	}
	
	const storage_t & getStorage() const {
		return storage;
	}

	const Geometry::Box & getBounds() const{
		return storage.getBox();
	}
		
	size_t getMemoryUsage()const{
		size_t mem = 0;
		std::deque<SamplePoint> samples;
		storage.collectPoints(samples);
		mem += sizeof(SamplePoint);
		mem += sizeof(SamplePointData) ;
		mem += (*samples.begin()).data->times.size() * sizeof(float) + sizeof(std::vector<float>);
		mem += (*samples.begin()).data->errors.size() * sizeof(float) + sizeof(std::vector<float>);
		mem += (*samples.begin()).data->pixels.size() * sizeof(float) + sizeof(std::vector<float>);
		return mem * samples.size();
	}

	void displaySamples(FrameContext & fc) const {

		std::deque<SamplePoint> l;
		storage.collectPoints(l);

	for(const auto & p : l) {
			Rendering::drawAbsBox(fc.getRenderingContext(), Geometry::Box(p.getPosition(), 0.2));
		}
	}

    void keepSamples(uint32_t amount){
        std::deque<SamplePoint> q;
        storage.collectPoints(q);
        storage.clear();
        for(const SamplePoint & x : q)
            if(x.getId() < amount){
                std::cerr << x.getId() << "\t";
                storage.insert(SamplePoint(x));
            }
    }

};

}

}

#endif /* SAMPLESTORAGE_H_ */
#endif // MINSG_EXT_MULTIALGORENDERING
