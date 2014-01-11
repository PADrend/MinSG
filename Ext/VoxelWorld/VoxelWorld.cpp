/*
	This file is part of the MinSG library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VOXEL_WORLD
#include "VoxelWorld.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Geometry/VoxelStorage.h>
#include <Util/Graphics/Color.h>
#include <iostream>
#include <unordered_map>

namespace std{
template <> struct hash<Geometry::_Vec3<int32_t>> {
	size_t operator()(const Geometry::_Vec3<int32_t> & v) const noexcept {	return static_cast<uint64_t>(v.x())*(129731+v.y())^v.z();	}
};
}

namespace MinSG {

struct VoxelGrid{
	const uint32_t wx,wy,wz,wxy;
	std::vector<uint32_t> voxels; // voxel, edgeflags, occlusion value
	std::unordered_map<Geometry::_Vec3<int32_t>,Util::Color4f> ambientLightValues;
	
	VoxelGrid(uint32_t _wx, uint32_t _wy, uint32_t _wz) : 
		wx(_wx), wy(_wy), wz(_wz), wxy(wx*wy), voxels(wx*wy*wz){
	}
	
	void set(uint32_t x,uint32_t y,uint32_t z,uint32_t value){
		voxels[ x+y*wx+z*wxy ] = value;
	}
	void setAmbientLightValue(const Geometry::_Vec3<int32_t>& pos, const Util::Color4f & c){
		ambientLightValues[pos] = c;
	}
	void updateAmbientLightValue(const Geometry::_Vec3<int32_t>& pos, const Util::Color4f & c){
		ambientLightValues[pos] += c;
	}
	
	inline uint32_t get(int32_t x,int32_t y,int32_t z)const{
		return (x<0||x>=static_cast<int32_t>(wx)||y<0||y>=static_cast<int32_t>(wy)||z<0||z>=static_cast<int32_t>(wz)) ? 0 : voxels[ x+y*wx+z*wxy ];
	}
	inline uint32_t get(const Geometry::_Vec3<int32_t>& v)const{
		return get(v.x(),v.y(),v.z());
	}
	const Util::Color4f & getAmbientLightValue(const Geometry::_Vec3<int32_t>& pos)const{
		static const  Util::Color4f black;
		const auto it = ambientLightValues.find(pos);
		return it==ambientLightValues.end() ? black : it->second;
	}
	
	uint32_t index(const Geometry::_Vec3<int32_t>& v)const{
		return v.x()+v.y()*wx+v.z()*wxy;
	}
	Geometry::_Vec3<int32_t> clamp(const Geometry::_Vec3<int32_t>& v)const{
		return Geometry::_Vec3<int32_t>(
				std::max( std::min(v.x(),static_cast<int32_t>(wx)),0 ),
				std::max( std::min(v.y(),static_cast<int32_t>(wy)),0 ),
				std::max( std::min(v.z(),static_cast<int32_t>(wz)),0 )
		);
	}

	// (internal)
	void addLocalLight(uint32_t value, int32_t x,int32_t y,int32_t z, Util::Color4f & localLight,uint8_t dist,uint8_t pass)const{
		if( (x%7)==0 && (y%7)==0 &&  (z%7)==0 ){
			if(pass==0){
				if(value!=0)
					localLight += Util::Color4f(0.0, std::max( 0.0f,1.0f/(dist*dist+1)),0,0);
				else
					localLight += Util::Color4f(0.0, 0.0,std::max( 0.0f,0.2f/(dist*dist+1)),0);
			}
		}else{
			if(isTransparent(value)){
				if(pass==1){
					localLight += getAmbientLightValue( Geometry::_Vec3<int32_t>(x,y,z) )/(dist*dist+1)*0.4;
														
					// omni ambient
					float f = 0.007f/(dist*dist+1);
					localLight += Util::Color4f(f,f,f,0.0);
				}
					
			}
		}
	}
	inline bool isTransparent(uint32_t value)const{
		return value==0;
	}
	
	// (internal)
	void _collectLocalData(int32_t x,int32_t y,int32_t z,int32_t dx,int32_t dy,int32_t dz, Util::Color4f & localLight,uint8_t pass)const{
		uint32_t value = get(x,y,z);
		if(pass==0)
			addLocalLight(value,x,y,z,localLight,0,pass);
		if(isTransparent(value)){
			int32_t xt = x+dx;
			for(int32_t i=1; i<4 ;++i,xt+=dx){ // x
				value = get(xt,y,z);
				addLocalLight(value,xt,y,z,localLight,i,pass);
				if(!isTransparent(value))
					break;
			}
			int32_t yt = y+dy;
			for(int32_t i=1; i<4 ; ++i,yt+=dy){ // y
				value = get(x,yt,z);
				addLocalLight(value,x,yt,z,localLight,i,pass);
				if(!isTransparent(value))
					break;
			}
			int32_t zt = z+dz;
			for(int32_t i=1; i<4 ;++i,zt+=dz){  // z
				value = get(x,y,zt);
				addLocalLight(value,x,y,zt,localLight,i,pass);
				if(!isTransparent(value))
					break;
			}
			
			xt = x+dx, yt = y+dy;
			for(int32_t i=1; i<4; ++i,xt+=dx,yt+=dy){ //xy
				value = get(xt,yt,z);
				addLocalLight(value,xt,yt,z,localLight,i*2,pass);
				if(!isTransparent(value))
					break;
			} 
			
			xt = x+dx,  zt = z+dz;
			for(int32_t i=1; i<4; ++i,xt+=dx,zt+=dz){ //xz
				value = get(xt,y,zt);
				addLocalLight(value,xt,y,zt,localLight,i*2,pass);
				if(!isTransparent(value))
					break;
			} 
			
			yt = y+dy, zt = z+dz;
			for(int32_t i=1; i<4; ++i,yt+=dy,zt+=dz){ //yz
				value = get(x,yt,zt);
				addLocalLight(value,x,yt,zt,localLight,i*2,pass);
				if(!isTransparent(value))
					break;
			} 
			
			
			
			xt = x+dx, yt = y+dy, zt = z+dz;
			for(int32_t i=1; i<4; ++i,xt+=dx,yt+=dy,zt+=dz){
				value = get(xt,yt,zt);
				addLocalLight(value,xt,yt,zt,localLight,i*3,pass);
				if(!isTransparent(value))
					break;
			}
		}
		
	}
	//! \return [freeVolume,localLight]
	std::pair<uint32_t,Util::Color4f> collectLocalData(int32_t x,int32_t y,int32_t z,const Geometry::Vec3& normal,uint8_t pass){
		Util::Color4f localLight(0,0,0,1);
		
		if(normal.x()>0 || normal.y()>0 || normal.z()>0)
			_collectLocalData(x,  y,  z,	 1, 1, 1, localLight,pass);

		if(normal.x()>0 || normal.y()>0 || normal.z()<0)
			_collectLocalData(x,  y,  z-1,	 1, 1,-1, localLight,pass);

		if(normal.x()>0 || normal.y()<0 || normal.z()>0)
			_collectLocalData(x,  y-1,z,	 1,-1, 1, localLight,pass);

		if(normal.x()>0 || normal.y()<0 || normal.z()<0)
			_collectLocalData(x,  y-1,z-1,	 1,-1,-1, localLight,pass);
			
		if(normal.x()<0 || normal.y()>0 || normal.z()>0)
			_collectLocalData(x-1,y,  z,	-1, 1, 1, localLight,pass);
			
		if(normal.x()<0 || normal.y()>0 || normal.z()<0)
			_collectLocalData(x-1,y,  z-1,	-1, 1,-1, localLight,pass);
			
		if(normal.x()<0 || normal.y()<0 || normal.z()>0)
			_collectLocalData(x-1,y-1,z,	-1,-1, 1, localLight,pass);
			
		if(normal.x()<0 || normal.y()<0 || normal.z()<0)
			_collectLocalData(x-1,y-1,z-1,	-1,-1,-1, localLight,pass);
		return std::make_pair(0,localLight);
	}
	
	/*! Cast a ray from @p source to @p target.
		If no block was hit, the distance is returned;
		Otherwise, the negative distance to the first intersection is returned.
		\todo support casting beyond the grid's boundaries.	
	*/
	float cast(const Geometry::Vec3& source, const Geometry::Vec3& target)const{
		Geometry::Vec3 dir = target-source;
		const float distance=dir.length();
		if(distance==0)
			return -1;
		dir/=distance;
		
		const float stepSize = 0.13;
		const Geometry::Vec3 step(dir*stepSize);
		
		Geometry::Vec3 v = source + dir*0.01f;
		int32_t x=source.x()-10/*arbitrary invalid value*/, y=0, z=0;
		
		for(float currentRayLength = 0.01f; currentRayLength<distance; currentRayLength+=stepSize){
			if(static_cast<int32_t>(v.x())!=x || static_cast<int32_t>(v.y())!=y || static_cast<int32_t>(v.z())!=z){
				x = static_cast<int32_t>(v.x());
				y = static_cast<int32_t>(v.y());
				z = static_cast<int32_t>(v.z());
				if(get(x,y,z)!=0 ){
					return -currentRayLength;
				}
			}
			v += step;
		}
		return distance;
		
	}
};


static void createVertex(Rendering::MeshUtils::MeshBuilder&mb, VoxelGrid& grid,int32_t x,int32_t y,int32_t z, const Geometry::Vec3& normal){
//	mb.color( grid.getOcclusionColor(x,y,z) );
	const auto localData = grid.collectLocalData(x,y,z,normal,1);
	
	const float freeVolume = 0.0; //localData.first/128.0f;
	
	const Geometry::Vec3 pos(x,y,z);
	float l = -1;

	// collect long range lighting 
	static const Geometry::Vec3 light( 7.49,7.57,7.55);
	if( normal.dot( light-pos )>0)
		l = grid.cast(pos,light);

//	mb.color( Util::Color4f( s+std::max(0.0f, 1.0f/l),s,s,1.0  ) );
	Util::Color4f vertexColor =  Util::Color4f( freeVolume+std::max(0.0f,2.0f/l),freeVolume,freeVolume,1.0  )+localData.second;
//	vertexColor = Util::Color4f( std::max(0.0f,vertexColor.getR()),std::max(0.0f,vertexColor.getG()),std::max(0.0f,vertexColor.getB()),1.0 );
	mb.color( vertexColor );
	mb.position(Geometry::Vec3(x,y,z));
	mb.addVertex();
}

static void calculateAmbientLightingValue(VoxelGrid& grid,const Geometry::_Vec3<int32_t> &pos,const Geometry::Vec3& normal){
	Util::Color4f c;
	
	const auto localData = grid.collectLocalData(pos.x(),pos.y(),pos.z(),normal,0);
	c+=localData.second;
	
	const Geometry::Vec3 pos2(pos);
		// collect long range lighting 
	static const Geometry::Vec3 light( 7.49,7.57,7.55);
	float l = -1;
	if( normal.dot( light-pos2 )>0)
		l = grid.cast(pos2,light);
	
	c += Util::Color4f( std::max(0.0f,2.0f/l),0,0,0.0  );
	
	// collect local light
	
	grid.updateAmbientLightValue( pos, c);
}

Util::Reference<Rendering::Mesh> VoxelWorld::generateMesh( const simpleVoxelStorage_t& voxelStorage, const Geometry::_Box<int32_t>& boundary){
	const auto data=voxelStorage.serialize(boundary);

	Rendering::MeshUtils::MeshBuilder mb;
	
	VoxelGrid grid(boundary.getExtentX(),boundary.getExtentY(),boundary.getExtentZ());
	
	// fill uniform blocks
	for(const auto & area : data.first){
		const auto sidelength = std::get<1>(area);
		const auto min = grid.clamp( std::get<0>(area)-boundary.getMin() );
		const auto max = grid.clamp( std::get<0>(area)-boundary.getMin()+Geometry::_Vec3<int32_t>(sidelength,sidelength,sidelength) );
		const auto value = std::get<2>(area);
				
		for(int32_t z=min.z(); z<max.z(); ++z){
			for(int32_t y=min.y(); y<max.y(); ++y){
				for(int32_t x=min.x(); x<max.x(); ++x){
					grid.set(x,y,z,value);
				}
			}
		}
	}
	// fill leaf node blocks
	for(const auto & leafNode : data.second){
		const auto block = std::get<1>(leafNode);
		const uint32_t startIndex = grid.index( std::get<0>(leafNode)- boundary.getMin() );

		for(uint32_t blockIndex=0; blockIndex<simpleVoxelStorage_t::blockSize; ++blockIndex){
			const uint32_t value = block[blockIndex];
			if(value!=0){
				const uint32_t voxelIndex = startIndex + blockIndex%simpleVoxelStorage_t::blockSideLength +
											grid.wx * ((blockIndex/simpleVoxelStorage_t::blockSideLength)%simpleVoxelStorage_t::blockSideLength) +
											grid.wxy * ((blockIndex/(simpleVoxelStorage_t::blockSideLength*simpleVoxelStorage_t::blockSideLength))%simpleVoxelStorage_t::blockSideLength);
				if(voxelIndex<grid.voxels.size())
					grid.voxels[voxelIndex] = value;
			}
		}
	}

	// collect ambient light data
	for(uint32_t z=0; z<grid.wz; ++z){
		for(uint32_t y=0; y<grid.wy; ++y){
			for(uint32_t x=0; x<grid.wx; ++x){
				const Geometry::_Vec3<int32_t> pos(x,y,z);
				const uint32_t value = grid.get(pos);
				if(value!=0)
					continue;
				if(grid.get(x-1,y,z)!=0){
					static const Geometry::Vec3f normal(1.0f,0,0);
					calculateAmbientLightingValue(grid, pos,normal);
				}
				if(grid.get(x+1,y,z)!=0){
					static const Geometry::Vec3f normal(-1.0f,0,0);
					calculateAmbientLightingValue(grid, pos,normal);
				}
				if(grid.get(x,y+1,z)!=0){
					static const Geometry::Vec3f normal(0,-1.0f,0);
					calculateAmbientLightingValue(grid, pos,normal);
				}
				if(grid.get(x,y-1,z)!=0){
					static const Geometry::Vec3f normal(0,1.0f,0);
					calculateAmbientLightingValue(grid, pos,normal);
				}
				if(grid.get(x,y,z+1)!=0){
					static const Geometry::Vec3f normal(0,0,-1.0f);
					calculateAmbientLightingValue(grid, pos,normal);
				}
				if(grid.get(x,y,z-1)!=0){
					static const Geometry::Vec3f normal(0,0,1.0f);
					calculateAmbientLightingValue(grid, pos,normal);
				}
			}
		}
	}
	
	
	

	const Geometry::Vec3 unit(1,1,1);
	
	mb.color( Util::Color4f(0.5,0.5,0.5) );
	for(uint32_t z=0; z<grid.wz; ++z){
		for(uint32_t y=0; y<grid.wy; ++y){
			for(uint32_t x=0; x<grid.wx; ++x){
				const uint32_t value = grid.get(x,y,z);
				if(value!=0)
					continue;
				if(grid.get(x-1,y,z)!=0){
					static const Geometry::Vec3f normal(1.0f,0,0);
					const uint32_t idx = mb.getNextIndex();
					mb.normal( normal );
					createVertex(mb, grid, x  ,y  ,z  ,normal);
					createVertex(mb, grid, x  ,y+1,z  ,normal);
					createVertex(mb, grid, x  ,y+1,z+1,normal);
					createVertex(mb, grid, x  ,y  ,z+1,normal);
					mb.addQuad(idx,idx+1,idx+2,idx+3);
				}
				if(grid.get(x+1,y,z)!=0){
					static const Geometry::Vec3f normal(-1.0f,0,0);
					const uint32_t idx = mb.getNextIndex();
					mb.normal( normal );
					createVertex(mb, grid, x+1,y  ,z  ,normal);
					createVertex(mb, grid, x+1,y  ,z+1,normal);
					createVertex(mb, grid, x+1,y+1,z+1,normal);
					createVertex(mb, grid, x+1,y+1,z  ,normal);
					mb.addQuad(idx,idx+1,idx+2,idx+3);
				}
				if(grid.get(x,y+1,z)!=0){
					static const Geometry::Vec3f normal(0,-1.0f,0);
					const uint32_t idx = mb.getNextIndex();
					mb.normal( normal );
					createVertex(mb, grid, x  ,y+1,z  ,normal);
					createVertex(mb, grid, x+1,y+1,z  ,normal);
					createVertex(mb, grid, x+1,y+1,z+1,normal);
					createVertex(mb, grid, x  ,y+1,z+1,normal);
					mb.addQuad(idx,idx+1,idx+2,idx+3);
				}
				if(grid.get(x,y-1,z)!=0){
					static const Geometry::Vec3f normal(0,1.0f,0);
					const uint32_t idx = mb.getNextIndex();
					mb.normal( normal );
					createVertex(mb, grid, x  ,y  ,z  ,normal);
					createVertex(mb, grid, x  ,y  ,z+1,normal);
					createVertex(mb, grid, x+1,y  ,z+1,normal);
					createVertex(mb, grid, x+1,y  ,z  ,normal);
					mb.addQuad(idx,idx+1,idx+2,idx+3);
				}
				if(grid.get(x,y,z+1)!=0){
					static const Geometry::Vec3f normal(0,0,-1.0f);
					const uint32_t idx = mb.getNextIndex();
					mb.normal( normal );
					createVertex(mb, grid, x  ,y  ,z+1,normal);
					createVertex(mb, grid, x  ,y+1,z+1,normal);
					createVertex(mb, grid, x+1,y+1,z+1,normal);
					createVertex(mb, grid, x+1,y  ,z+1,normal);
					mb.addQuad(idx,idx+1,idx+2,idx+3);
				}
				if(grid.get(x,y,z-1)!=0){
					static const Geometry::Vec3f normal(0,0,1.0f);
					const uint32_t idx = mb.getNextIndex();
					mb.normal( normal );
					createVertex(mb, grid, x  ,y  ,z  ,normal);
					createVertex(mb, grid, x+1,y  ,z  ,normal);
					createVertex(mb, grid, x+1,y+1,z  ,normal);
					createVertex(mb, grid, x  ,y+1,z  ,normal);
					mb.addQuad(idx,idx+1,idx+2,idx+3);
				}
			}
		}
	}
	
	return mb.buildMesh();

}


}
#endif /* MINSG_EXT_VOXEL_WORLD */
