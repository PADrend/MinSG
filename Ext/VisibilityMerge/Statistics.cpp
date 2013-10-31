/*
	This file is part of the MinSG library extension VisibilityMerge.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITYMERGE

#include "Statistics.h"
#include "Helper.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../ValuatedRegion/ValuatedRegionNode.h"
#include "../VisibilitySubdivision/VisibilityVector.h"
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/GenericAttribute.h>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace MinSG {
using namespace VisibilitySubdivision;
namespace VisibilityMerge {

Statistics::Statistics(const std::string & fileName) :
	perPassOutput(false), perListOutput(false), perCellOutput(false), perObjectOutput(false), pass(0) {
	out.open(fileName.c_str());
	if(out.fail()) {
		throw std::runtime_error("Error: Cannot open file " + fileName);
	}
}

Statistics::~Statistics() {
	out.close();
}

void Statistics::perRun(const std::string & fileName, const std::string & timeStamp, const std::string & description, size_t wsSize, costs_t trianglesMinLimit,
						costs_t objectCountLimit, costs_t cellCountLimit, unsigned int passBoundary) {
	std::unique_ptr<std::ostream> out(Util::FileUtils::openForWriting(Util::FileName(fileName)));
	if(out.get() == nullptr) {
		throw std::runtime_error("Error: Cannot open file " + fileName);
	}
	*out << "Time\t" << "Description\t" << "Working_set_size\t" << "Minimum_number_of_triangles\t" << "Object_limit\t" << "Cell_limit\t" << "Pass_boundary\n";
	*out << timeStamp << "\t\"" << description << "\"\t" << wsSize << '\t' << trianglesMinLimit << '\t' << objectCountLimit << '\t' << cellCountLimit << '\t'
			<< passBoundary << '\n';
}

void Statistics::perPass(double duration, size_t numMerges) {
	if(perListOutput || perCellOutput || perObjectOutput) {
		throw std::runtime_error("Error: Incompatible data was written before");
	}
	if(!perPassOutput) {
		perPassOutput = true;
		out << "Pass\t" << "Duration\t" << "Number_of_merges\n";
	}
	out << pass << '\t' << duration << '\t' << numMerges << '\n';
	++pass;
}

void Statistics::perList(const visibility_sharer_map_t & lists) {
	if(perCellOutput || perObjectOutput || perPassOutput) {
		throw std::runtime_error("Error: Incompatible data was written before");
	}
	if(!perListOutput) {
		perListOutput = true;
		out << "Pass\t" << "Visibility_list\t" << "Number_of_visible_objects\t" << "Number_of_visible_triangles\t" << "Number_of_visible_pixels\n";
	}
	for(const auto & element : lists) {
		const auto & list = element.first;
		size_t numObjects = 0;
		VisibilityVector::costs_t totalCosts = 0;
		VisibilityVector::benefits_t totalBenefits = 0;
		for(const auto & attrib : *list) {
			const auto & vv = Helper::getVV(attrib.get());
			numObjects += vv.getVisibleNodeCount();
			totalCosts += vv.getTotalCosts();
			totalBenefits += vv.getTotalBenefits();
		}
		out << pass << '\t' << list << '\t' << numObjects << '\t' << totalCosts << '\t' << totalBenefits << '\n';
	}
	++pass;
}

void Statistics::perCell(const cell_set_t & cells) {
	if(perListOutput || perObjectOutput || perPassOutput) {
		throw std::runtime_error("Error: Incompatible data was written before");
	}
	if(!perCellOutput) {
		perCellOutput = true;
		out << "Pass\t" << "Visibility_cell\t" << "Number_of_visible_objects\t" << "Number_of_visible_triangles\t" << "Number_of_visible_pixels\t"
				<< "Volume\n";
	}
	for(const auto & cell : cells) {
		size_t numObjects = 0;
		VisibilityVector::costs_t totalCosts = 0;
		VisibilityVector::benefits_t totalBenefits = 0;
		list_ptr gal = Helper::getVVList(cell);
		for(const auto & attrib : *gal) {
			const auto & vv = Helper::getVV(attrib.get());
			numObjects += vv.getVisibleNodeCount();
			totalCosts += vv.getTotalCosts();
			totalBenefits += vv.getTotalBenefits();
		}
		out << pass << '\t' << cell << '\t' << numObjects << '\t' << totalCosts << '\t' << totalBenefits << '\t' << cell->getBB().getVolume() << '\n';
	}
	++pass;
}

void Statistics::perObject(const sorted_object_set_t & objects) {
	if(perListOutput || perCellOutput || perPassOutput) {
		throw std::runtime_error("Error: Incompatible data was written before");
	}
	if(!perObjectOutput) {
		perObjectOutput = true;
		out << "Pass\t" << "Object\t" << "Number_of_triangles\t" << "Volume\n";
	}

	for(const auto & o : objects) {
		const Geometry::Box & bb = o->getWorldBB();
		out << pass << '\t' << o << '\t' << o->getTriangleCount() << '\t' << bb.getVolume() << '\n';
	}
	++pass;
}

}
}

#endif /* MINSG_EXT_VISIBILITYMERGE */
