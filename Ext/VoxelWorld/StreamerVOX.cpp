/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VOXEL_WORLD

#include "StreamerVOX.h"
#include <Util/Macros.h>
#include <Util/IO/FileUtils.h>
#include <Util/StringUtils.h>
#include <memory>


using namespace Util;

namespace MinSG {


const static uint32_t VOX_VERSION = 0x01;
const static uint32_t VOX_HEADER = 0x0d786f76; // = "vox "

const static uint32_t VOX_AREA_DATA = 0x56000000;
const static uint32_t VOX_4X4X4LEAF_DATA = 0x56000001;
const static uint32_t VOX_END = 0xFFFFFFFF;


struct Reader{
	Reader(std::istream & _in) : in(_in){}
	std::istream & in;
	uint32_t read_uint32() {
		uint32_t x;
		in.read(reinterpret_cast<char *> (&x), sizeof(x));
		return x;
	}
	int32_t read_int32() {
		int32_t x;
		in.read(reinterpret_cast<char *> (&x), sizeof(x));
		return x;
	}
	uint8_t read_uint8() {
		uint8_t x;
		in.read(reinterpret_cast<char *> (&x), sizeof(x));
		return x;
	}
	void read(uint8_t * data,size_t count){
		in.read(reinterpret_cast<char *> (data), count);
	}
	void skip(uint32_t size){
		in.seekg(std::ios_base::cur+size);
	}

};

static void readAreaData(Reader & in,std::vector<std::tuple<Geometry::_Vec3<int32_t>,uint32_t,uint32_t>> &areas);
static void read4x4x4LeafData(Reader & in,std::vector<std::tuple<Geometry::_Vec3<int32_t>, std::array<uint32_t, 64>>> &blocks);

VoxelWorld::serializationData_t  VoxelWorld::loadVoxels(const Util::FileName & filename){
	std::unique_ptr<std::istream> input( Util::FileUtils::openForReading(filename) );
	return std::move(loadVoxels(*input));
}

VoxelWorld::serializationData_t VoxelWorld::loadVoxels(std::istream & input) {
	Reader reader(input);

	const uint32_t format = reader.read_uint32();
	if(format!=VOX_HEADER)
		throw std::runtime_error(std::string("wrong  format: ") + Util::StringUtils::toString(format));
	const uint32_t version = reader.read_uint32();
	if(version>VOX_VERSION)
		throw std::runtime_error(std::string("can't read voxels, version to high: ") + Util::StringUtils::toString(version));

	VoxelWorld::serializationData_t data;
	
	uint32_t blockType = reader.read_uint32();
	while(blockType != VOX_END && input.good()) {
		// blocksize is discarded.
		uint32_t blockSize = reader.read_uint32();
		switch(blockType) {
			case VOX_AREA_DATA:
				std::cout<< "read VOX_AREA_DATA";
				readAreaData(reader, data.first);
				break;
			case VOX_4X4X4LEAF_DATA:
				std::cout<< "read VOX_4X4X4LEAF_DATA";
				read4x4x4LeafData(reader, data.second);
				break;
			default:
				WARN("LoaderVOX::loadVoxels: unknown data block found.");
				std::cout << "blockSize:"<<blockSize<<" \n";
				reader.skip(blockSize);
				break;
		}
		blockType = reader.read_uint32();
	}
	return data;
}

void readAreaData(Reader& in, std::vector<std::tuple<Geometry::_Vec3<int32_t>,uint32_t,uint32_t>> &areas){
	for(uint32_t areasToRead = in.read_uint32(); areasToRead>0 ; --areasToRead){
		const auto x = in.read_int32();
		const auto y = in.read_int32();
		const auto z = in.read_int32();
		const auto sideLength = in.read_uint32();
		const auto value = in.read_uint32();
		areas.emplace_back(std::make_tuple(Geometry::_Vec3<int32_t>(x,y,z),sideLength,value)  );
	}
}

void read4x4x4LeafData(Reader & in,std::vector<std::tuple<Geometry::_Vec3<int32_t>, std::array<uint32_t, 64>>>&blocks) {
	for(uint32_t blocksToRead = in.read_uint32(); blocksToRead>0 ; --blocksToRead){
		const auto x = in.read_int32();
		const auto y = in.read_int32();
		const auto z = in.read_int32();

		blocks.emplace_back(std::make_tuple(Geometry::_Vec3<int32_t>(x,y,z),std::array<uint32_t, 64>()));
		auto& block = std::get<1>(blocks.back());
		
		uint32_t i = 0;
		for(auto numberOfRunsToRead = in.read_uint8(); numberOfRunsToRead>0; --numberOfRunsToRead){
			const auto value = in.read_uint32();
			for(uint8_t runLength=in.read_uint8(); runLength>0; --runLength){
				if(i>=64)
					throw std::runtime_error("Invalid data format");
				block[i]=value;
				++i;
			}
		}
		
		while(i<64)
			block[i++]=0;
		
	}
}


static void write(std::ostream & out, uint32_t x) {
	out.write(reinterpret_cast<char *> (&x), sizeof(x));
}
static void write(std::ostream & out, int32_t x) {
	out.write(reinterpret_cast<char *> (&x), sizeof(x));
}
static void write(std::ostream & out, uint8_t x) {
	out.write(reinterpret_cast<char *> (&x), sizeof(x));
}



void VoxelWorld::saveVoxels(const Util::FileName & filename, const serializationData_t&data){
	std::unique_ptr<std::ostream> output( Util::FileUtils::openForWriting(filename) );
	saveVoxels(*output, data);
}

void  VoxelWorld::saveVoxels(std::ostream & output, const VoxelWorld::serializationData_t& data){
	// Write Header
	write(output, VOX_HEADER);
	write(output, VOX_VERSION);

	if(!data.first.empty()){// Write AreaBlock
		write(output, VOX_AREA_DATA); // block header
		
		const auto areaData = data.first;
		const uint32_t numberOfAreas = areaData.size();
		// data size
		write(output,static_cast<uint32_t>(sizeof(numberOfAreas) + numberOfAreas*( 3*sizeof(int32_t) + 2*sizeof(uint32_t))));
		write(output,numberOfAreas);
		for(const auto& area : areaData ){
			write(output,std::get<0>(area).x() );
			write(output,std::get<0>(area).y() );
			write(output,std::get<0>(area).z() );
			write(output,std::get<1>(area) ); // sideLength
			write(output,std::get<2>(area) ); // value
		}
	}

/*

	4x4x4LeafsBlock ::= AreaBlock-dataType (uint32 0x56000001),
					uint32 dataSize,
					uint32 numberOfLeafBlocks
					[int32 x,int32 y,int32 z, uint8_t nrOfRuns, [uint32_t value,uint8_t RunLength]* ]*
*/
	{// Write LeafData
		write(output, VOX_4X4X4LEAF_DATA); // block header
		
		const auto blockData = data.second;

		const uint32_t numberOfBlocks = blockData.size();
		uint32_t dataSize = sizeof(numberOfBlocks);
		std::vector<uint8_t> numbersOfRuns;
		
		// firstPass, check the data size and number of runs
		for(const auto& block : blockData ){
			dataSize += 3*sizeof(int32_t) + sizeof(uint8_t); // x,y,z,numberOfRuns
			const auto& values = std::get<1>(block);
			uint32_t value = std::get<1>(block)[0];
			uint8_t numberOfRuns = 1;
			for(uint8_t i=1; i<values.size(); ++i){
				if(value!=values[i]){
					value = values[i];
					++numberOfRuns;
				}
			}
			dataSize += numberOfRuns*(sizeof(value)+sizeof(uint8_t)); // last run: value,length
			numbersOfRuns.emplace_back(numberOfRuns);
		}
		write(output, dataSize); // data size
		write(output, numberOfBlocks); // number of blocks
		uint32_t blockIndex = 0;
		for(const auto& block : blockData ){
			const auto& origin = std::get<0>(block);
			write(output, origin.x());
			write(output, origin.y());
			write(output, origin.z());
			write(output, numbersOfRuns[blockIndex]); // nr of runs
			const auto& values = std::get<1>(block);
			uint32_t value = values[0];
			uint8_t runLength = 1;
			for(uint8_t i=1; i<values.size(); ++i){
				if(value!=values[i]){
					write(output, value);
					write(output, runLength);
					value = values[i];
					runLength = 1;
				}else{
					++runLength;
				}
			}
			write(output, value);
			write(output, runLength);
			++blockIndex;
		}
				
	}

	// Write END
	write(output, VOX_END);
}





}

#endif /* MINSG_EXT_VOXEL_WORLD */
