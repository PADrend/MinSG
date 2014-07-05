/*
	This file is part of the MinSG library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VOXEL_WORLD
#include "VoxelWorld.h"
#include "MaterialLib.h"
#include "Material.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Geometry/PointOctree.h>
#include <Geometry/Sphere.h>
#include <Geometry/VoxelStorage.h>
#include <Util/Graphics/Color.h>
#include <iostream>
#include <unordered_map>

#include <Util/Timer.h>
#include <map>


typedef Geometry::_Vec3<int32_t>	vec3i;
typedef Geometry::Vec3 				vec3f;
typedef uint32_t					value_t;


namespace std{
template <> struct hash<vec3i> {
	size_t operator()(const vec3i & v) const noexcept {	return static_cast<uint64_t>(v.x())*(129731+v.y())^v.z();	}
};
template <> struct hash<std::pair<vec3i,vec3i>> {
	size_t operator()(const std::pair<vec3i,vec3i> & p) const noexcept {	
		return hash<vec3i>()(p.first) - hash<vec3i>()(p.second);	}
};
}

namespace MinSG {

struct LightProbeData{
	struct LightProbeOctreePoint{
		const vec3f position; //redundant!
		const size_t lightProbeIndex;
		LightProbeOctreePoint(const vec3f & p,size_t idx) : position(p),lightProbeIndex(idx){}
		const vec3f& getPosition()const{	return position;	}
	};
	struct LightProbe{
		Util::Color4f color, lastPassColor;
		vec3f position,normal;
		std::vector<std::pair<const LightProbe*,float>> connectedProbes; //(probe,1/distanceSquared)
		LightProbe(const Util::Color4f& c) : color(c){}
		LightProbe(const Util::Color4f& c,const vec3f& pos) : color(c),position(pos){}
		LightProbe(const Util::Color4f& c,const vec3f& pos,const vec3f& n) : color(c),position(pos),normal(n){}
	};
	
	std::unordered_map<std::pair<vec3i,vec3i>,size_t> vertexLightProbeIndices; // pos,normal -> index
	std::vector<size_t> orderedVertexLightProbeIndices;
	Geometry::PointOctree<LightProbeOctreePoint> lightProbeOctree; // pos ->index
	std::vector<LightProbe> lightProbes;
		
	LightProbeData(uint32_t _wx, uint32_t _wy, uint32_t _wz) : 
			lightProbeOctree(Geometry::Box( vec3f(0,0,0),vec3f(_wx,_wy,_wz)),4,8){
	}
	
	Util::Color4f getVertexLightProbe(const vec3i& pos,const vec3i& normal)const{
		static const  Util::Color4f black;
		const auto& it = vertexLightProbeIndices.find(std::make_pair(pos,normal));
		return it==vertexLightProbeIndices.end() ? black : lightProbes.at( it->second ).color;
	}
	// create a lightProbe for pos,normal and return it, or return nullptr if it exists
	void initVertexLightProbe(const vec3i& pos,const vec3i& normal,	const VoxelWorld::Material& surfaceMaterial){
		size_t index = lightProbes.size();
		auto result = vertexLightProbeIndices.emplace(std::make_pair(vec3i(pos),vec3i(normal)),index);
		if(result.second){
			lightProbes.emplace_back(Util::Color4f(surfaceMaterial.getLight()),vec3f(pos),vec3f(normal));
			lightProbeOctree.insert(LightProbeOctreePoint(vec3f(pos),index));
			orderedVertexLightProbeIndices.push_back(index);
		}else{ //			update existing light probe reflectance and light color...
			lightProbes[result.first->second].color += surfaceMaterial.getLight();

		}
	}
	Util::Color4f& createFreeLightProbe(const vec3f& pos){
		const size_t index = lightProbes.size();
		lightProbes.emplace_back(Util::Color4f());
		lightProbeOctree.insert(LightProbeOctreePoint(pos,index));
		return lightProbes[index].color;
	}
};
using namespace VoxelWorld;

struct VoxelGrid{
	const uint32_t wx,wy,wz,wxy;
	std::vector<const Material*> voxels; // voxel, edgeflags, occlusion value
	const Material emptyMaterial;
	
	VoxelGrid(uint32_t _wx, uint32_t _wy, uint32_t _wz) : 
			wx(_wx), wy(_wy), wz(_wz), wxy(wx*wy), voxels(wx*wy*wz){
		std::fill(voxels.begin(),voxels.end(),&emptyMaterial);
	}
	
	void set(uint32_t x,uint32_t y,uint32_t z,Material* value)	{	voxels[ x+y*wx+z*wxy ] = value;	}

	
	inline const Material& get(int32_t x,int32_t y,int32_t z)const{
		return (x<0||x>=static_cast<int32_t>(wx)||y<0||y>=static_cast<int32_t>(wy)||z<0||z>=static_cast<int32_t>(wz)) ? emptyMaterial : *voxels[ x+y*wx+z*wxy ];
	}
	inline const Material& get(const vec3i& v)const				{	return get(v.x(),v.y(),v.z());	}

	
	uint32_t getIndex(const vec3i& v)const						{	return v.x()+v.y()*wx+v.z()*wxy;	}
	vec3i clamp(const vec3i& v)const{
		return vec3i(
				std::max( std::min(v.x(),static_cast<int32_t>(wx)),0 ),
				std::max( std::min(v.y(),static_cast<int32_t>(wy)),0 ),
				std::max( std::min(v.z(),static_cast<int32_t>(wz)),0 )
		);
	}


	/*! Cast a ray from @p source to @p target.
		If no block was hit, the distance is returned;
		Otherwise, the negative distance to the first intersection is returned.
		\todo support casting beyond the grid's boundaries.	
	*/
	float cast(const vec3f& source, const vec3f& target)const{
		vec3f dir = target-source;
		const float distance=dir.length();
		if(distance==0)
			return -1;
		dir/=distance;

		const float stepSize = 0.13f;
		const vec3f step(dir*stepSize);
		
		vec3f v = source + dir*0.01f;
		int32_t x=source.x()-10/*arbitrary invalid value*/, y=0, z=0;
		
		for(float currentRayLength = 0.01f; currentRayLength<distance-0.01f; currentRayLength+=stepSize){
			if(static_cast<int32_t>(v.x())!=x || static_cast<int32_t>(v.y())!=y || static_cast<int32_t>(v.z())!=z){
				x = static_cast<int32_t>(v.x());
				y = static_cast<int32_t>(v.y());
				z = static_cast<int32_t>(v.z());
				if(get(x,y,z).isSolid() ){
					return -currentRayLength;
				}
			}
			v += step;
		}
		return distance;
		
	}
};



Util::Reference<Rendering::Mesh> VoxelWorld::generateMesh( const simpleVoxelStorage_t& voxelStorage, const Geometry::_Box<int32_t>& boundary){
	/*
	0. fill grid
	
	1. create probes
		for all blocks
			create surface probes with global lighting
			collect mid range light emitters
	
	2. mid range lighting
		for all mid range light emitters
			find surface probes and add mid range lighting
		
	3. create connections
		for all surface probes
			connect to reachable surface probes
		
	4. indirect lighting
		distribute reflected light
		
	5. create mesh
		for all blocks
			create mesh
	
	*/
	
	std::map<std::string,float> timings;
	Util::Timer t;
	
	MaterialLib matLib;
	{
		auto& m1 = matLib.access(1);
		m1.setSolid(true);
		m1.setColor( Util::Color4f(0.5,0.5,0.5,1.0));
	}
	{
		auto& m1 = matLib.access(2);
		m1.setSolid(true);
		m1.setColor( Util::Color4f(1.0,1.0,1.0,1.0));
	}
	{
		auto& m1 = matLib.access(3);
		m1.setSolid(true);
		m1.setColor( Util::Color4f(0.1,0.1,0.1,1.0));
	}
	{
		auto& m1 = matLib.access(4);
		m1.setSolid(true);
		m1.setColor( Util::Color4f(0.2,0.1,0.1,1.0));
		m1.setLight( Util::Color4f(1.2,0.1,0.1,1.0));
	}
	{
		auto& m1 = matLib.access(10);
		m1.setSolid(false);
		m1.setLight( Util::Color4f(10.1,0.1,0.1,1.0));
	}
	{
		auto& m1 = matLib.access(11);
		m1.setSolid(false);
		m1.setLight( Util::Color4f(0.1,10.1,0.1,1.0));
	}
	{
		auto& m1 = matLib.access(12);
		m1.setSolid(false);
		m1.setLight( Util::Color4f(0.1,0.1,10.1,1.0));
	}
	
	VoxelGrid grid(boundary.getExtentX(),boundary.getExtentY(),boundary.getExtentZ());
	LightProbeData lighting(boundary.getExtentX(),boundary.getExtentY(),boundary.getExtentZ());

	// ---------------------------------------------------------------------------
	{ //	0. fill grid
		const auto data=voxelStorage.serialize(boundary);
		// fill uniform blocks
		for(const auto & area : data.first){
			const auto sidelength = std::get<1>(area);
			const auto min = grid.clamp( std::get<0>(area)-boundary.getMin() );
			const auto max = grid.clamp( std::get<0>(area)-boundary.getMin()+vec3i(sidelength,sidelength,sidelength) );
			const auto mat = &matLib.access( std::get<2>(area) );
					
			for(int32_t z=min.z(); z<max.z(); ++z){
				for(int32_t y=min.y(); y<max.y(); ++y){
					for(int32_t x=min.x(); x<max.x(); ++x){
						grid.set(x,y,z,mat);
					}
				}
			}
		}
		// fill leaf node blocks
		for(const auto & leafNode : data.second){
			const auto block = std::get<1>(leafNode);
			const uint32_t startIndex = grid.getIndex( std::get<0>(leafNode)- boundary.getMin() );

			for(uint32_t blockIndex=0; blockIndex<simpleVoxelStorage_t::blockSize; ++blockIndex){
				const value_t value = block[blockIndex];
				if(value!=0){
					const uint32_t voxelIndex = startIndex + blockIndex%simpleVoxelStorage_t::blockSideLength +
												grid.wx * ((blockIndex/simpleVoxelStorage_t::blockSideLength)%simpleVoxelStorage_t::blockSideLength) +
												grid.wxy * ((blockIndex/(simpleVoxelStorage_t::blockSideLength*simpleVoxelStorage_t::blockSideLength))%simpleVoxelStorage_t::blockSideLength);
					if(voxelIndex<grid.voxels.size()){
						grid.voxels[voxelIndex] = &matLib.access( value );
					}
				}
			}
		}
		timings["10_fill grid"] = t.getMilliseconds();
		t.reset();
	}
	
	// ---------------------------------------------------------------------------
	//	1. create probes

	std::vector<LightProbeData::LightProbe> midRangeLightEmitters;
	
	{
		const auto initQuadLightProbes = [&](const vec3i&pos, uint8_t xMod, uint8_t yMod, uint8_t zMod, const vec3i& normal, const Material& m){
			lighting.initVertexLightProbe(vec3i(pos.x()+(xMod&1)		,pos.y()+(yMod&1)		,pos.z()+(zMod&1))		, normal,	m);
			lighting.initVertexLightProbe(vec3i(pos.x()+(xMod&2?1:0)	,pos.y()+(yMod&2?1:0)	,pos.z()+(zMod&2?1:0))	, normal,	m);
			lighting.initVertexLightProbe(vec3i(pos.x()+(xMod&4?1:0)	,pos.y()+(yMod&4?1:0)	,pos.z()+(zMod&4?1:0))	, normal,	m);
			lighting.initVertexLightProbe(vec3i(pos.x()+(xMod&8?1:0)	,pos.y()+(yMod&8?1:0)	,pos.z()+(zMod&8?1:0))	, normal,	m);
		};
		// Traverse the grid in a zig-zag-manner to improve locality of orderedVertexLightProbeIndices
		for(uint32_t z2=0; z2<grid.wz; z2+=3){
		for(uint32_t y2=0; y2<grid.wy; y2+=3){
		for(uint32_t x2=0; x2<grid.wx; x2+=3){
		for(uint32_t z=z2; z<z2+3; ++z){
		for(uint32_t y=y2; y<y2+3; ++y){
		for(uint32_t x=x2; x<x2+3; ++x){
			const vec3i pos(x,y,z);
			const auto& m = grid.get(pos);
			if( m.isSolid() )
				continue;
			if( m.getLight().a()>0.0 ){
				midRangeLightEmitters.emplace_back( m.getLight(),vec3f(x+0.5f,y+0.5f,z+0.5f),vec3f(0,0,0) ); // light emitter
			}
			const Material* material;
			if( (material = &grid.get(x-1,y,z))->isSolid() )
				initQuadLightProbes(pos, 0 ,2|4, 4|8,		vec3i(1,0,0),	*material);
			if( (material = &grid.get(x+1,y,z))->isSolid() )
				initQuadLightProbes(pos, 1|2|4|8 ,4|8, 2|4,	vec3i(-1,0,0),	*material);
			if( (material = &grid.get(x,y+1,z))->isSolid() )
				initQuadLightProbes(pos, 2|4, 1|2|4|8, 4|8,	vec3i(0,-1,0),	*material);
			if( (material = &grid.get(x,y-1,z))->isSolid() )
				initQuadLightProbes(pos, 4|8 ,0, 2|4,		vec3i(0,1,0),	*material);
			if( (material = &grid.get(x,y,z+1))->isSolid() )
				initQuadLightProbes(pos, 4|8 ,2|4, 1|2|4|8,	vec3i(0,0,-1),	*material);
			if( (material = &grid.get(x,y,z-1))->isSolid() )
				initQuadLightProbes(pos, 2|4 ,4|8, 0,		vec3i(0,0,1),	*material);
		}}}}}}
				
		timings["20_init light probes"] = t.getMilliseconds();
		t.reset();
	}
	// ---------------------------------------------------------------------------

	
	{ //	2. connect surface probes to light sources
		
		// long range emitter
		static const vec3f globalLight( 7.49,30.57,7.55);

		for(const auto index : lighting.orderedVertexLightProbeIndices){
			auto& probe = lighting.lightProbes[ index ];
			if( probe.normal.dot( globalLight-probe.position )>0){
				const float l = grid.cast( probe.position ,globalLight);
				if(l>0)
					probe.color += Util::Color4f( 5.0, 5.1 , 5.0,1.0  );
			}
		}
		
		// mid range emitters
		std::deque<LightProbeData::LightProbeOctreePoint> localProbes;
		for(const auto & emitter : midRangeLightEmitters){
			const auto& pos = emitter.position;
			lighting.lightProbeOctree.collectPointsWithinBox(Geometry::Box(pos,12,12,12),localProbes);
			for(const auto& probePoint : localProbes){

				const auto dir = probePoint.position-pos;
				
				if( dir.lengthSquared()<16 ){

					auto& vertexProbe = lighting.lightProbes.at( probePoint.lightProbeIndex );
					if(	(vertexProbe.normal.isZero() || vertexProbe.normal.dot(dir)<0) ) { //&& vertexProbe.lastPassColor.getR()>0.01 
						const float dist = grid.cast(pos, probePoint.position);
						if(dist>0)
							vertexProbe.color += emitter.color * (1.0f/(1.0f+dist*dist) );
	//						probe.connectedProbes.push_back(&vertexProbe);
					}
				}
			}
		}
	}

	
	//--------------------------------------------------------------------------------------------------------
	// 3. create connections (vertexProbe <-> vertexProbe)
	{
		std::deque<LightProbeData::LightProbeOctreePoint> localProbes;
		int octreeQueries = 0;
		int collectedPoints = 0;
		int casts = 0;
		
		vec3f probesAtPos(-100000,0,0);

		for(const auto index : lighting.orderedVertexLightProbeIndices){
			auto& probe = lighting.lightProbes[ index ];
			const vec3f pos(probe.position);
			const vec3f normal(probe.normal);
			const vec3f castSourcePos(pos - probe.normal * 0.0001); // use a slightly set back position to fix light bleeding on planes
			
			if(pos.distanceSquared(probesAtPos)>10){
				localProbes.clear();
				lighting.lightProbeOctree.collectPointsWithinBox(Geometry::Box(pos,10,10,10),localProbes);
				probesAtPos = pos;
				++octreeQueries;
				collectedPoints+=localProbes.size();
			}

			for(const auto& probePoint : localProbes){
				if(probePoint.lightProbeIndex == index) // point itself
					continue;
				if(probePoint.lightProbeIndex > index) 
					continue;
					
				const auto dir = probePoint.position-pos;
				const auto distanceSquared = dir.lengthSquared();
				
				float connectionPower = -1.0f;
				auto& otherProbe = lighting.lightProbes.at( probePoint.lightProbeIndex );
				
				if(distanceSquared<0.001){
					if(normal.dot(otherProbe.normal)>0 )
						connectionPower = 0.05;
				}else if( //distanceSquared<16 && 
							normal.dot(dir)>0.001f && otherProbe.normal.dot(pos-otherProbe.position)>0.001f ){
					const float dist = grid.cast(castSourcePos, probePoint.position);
					++casts;
					if(dist>0)
						connectionPower  = 0.05f/(1.0f+dist*dist);
				}
				
				if(connectionPower>0){
					probe.connectedProbes.emplace_back(&otherProbe, connectionPower);
					otherProbe.connectedProbes.emplace_back(&probe, connectionPower);
				}

			}
			casts += probe.connectedProbes.size();
		}
		
		timings["31_queries"] = octreeQueries;
		timings["32_collected points"] = collectedPoints;
		timings["33_casts"] = casts;
		timings["30_light distribution"] = t.getMilliseconds();
		t.reset();
	}
	
	
	//--------------------------------------------------------------------------------------------------------
	// 4. indirect lighting
	
	for(uint8_t i=0;i<10;++i){
		for(auto& p : lighting.lightProbes){
			p.lastPassColor = p.color;
		}
		for(auto& probe : lighting.lightProbes){
			for(const auto& link : probe.connectedProbes){
				probe.color += link.first->lastPassColor * link.second;
			}
		}
	}
	
	//--------------------------------------------------------------------------------------------------------

	Rendering::MeshUtils::MeshBuilder mb;

	{
		const auto createVertex = [&](int32_t x,int32_t y,int32_t z, const Material& mat,const vec3i& normal) -> Util::Color4f {
			Util::Color4f vertexColor = mat.getColor();
			{ // calculate color
				const Util::Color4f light = lighting.getVertexLightProbe(vec3i(x,y,z),normal);
				
				Util::Color4f rgb( vertexColor.getR() * light.getR(),
										vertexColor.getG() * light.getG(),
										vertexColor.getB() * light.getB(), vertexColor.getA() );
				
				// perform gamma-correction
				const float gamma = 2.1;
				const float r2 = std::pow(rgb.r(),1.0f/gamma)*0.2;
				const float g2 = std::pow(rgb.g(),1.0f/gamma)*0.2;
				const float b2 = std::pow(rgb.b(),1.0f/gamma)*0.2;
				vertexColor = Util::Color4f( r2,g2,b2, vertexColor.getA() );
			}
			mb.color( vertexColor );

			mb.position(vec3f(x,y,z));
			mb.addVertex();
			return vertexColor;
		};
		const auto createQuad = [&](const vec3i&pos, uint8_t xMod, uint8_t yMod, uint8_t zMod, const vec3i& normal, const Material& mat){
			const uint32_t idx = mb.getNextIndex();
			mb.normal( vec3f(normal) );
			const auto c0 = createVertex(pos.x()+(xMod&1)		,pos.y()+(yMod&1)		,pos.z()+(zMod&1)		,mat, normal);
			const auto c1 = createVertex(pos.x()+(xMod&2?1:0)	,pos.y()+(yMod&2?1:0)	,pos.z()+(zMod&2?1:0)	,mat, normal);
			const auto c2 = createVertex(pos.x()+(xMod&4?1:0)	,pos.y()+(yMod&4?1:0)	,pos.z()+(zMod&4?1:0)	,mat, normal);
			const auto c3 = createVertex(pos.x()+(xMod&8?1:0)	,pos.y()+(yMod&8?1:0)	,pos.z()+(zMod&8?1:0)	,mat, normal);
			const auto c02 = (c0-c2).abs();
			const auto c13 = (c1-c3).abs();
			if(c02.r()+c02.g()+c02.b() > c13.r()+c13.g()+c13.b()){
				mb.addQuad(idx,idx+1,idx+2,idx+3);
			}else{
				mb.addQuad(idx+3,idx,idx+1,idx+2);
			}
		};
		
		mb.color( Util::Color4f(0.5,0.5,0.5) );
		for(uint32_t z=0; z<grid.wz; ++z){
			for(uint32_t y=0; y<grid.wy; ++y){
				for(uint32_t x=0; x<grid.wx; ++x){
					if(grid.get(x,y,z).isSolid())
						continue;
					const vec3i pos(x,y,z);
					
					const Material* material;
					if( material = &grid.get(x-1,y,z), material->isSolid() )
						createQuad( pos, 0, 2|4 , 4|8,			vec3i(1,0,0),	*material );
					if( material = &grid.get(x+1,y,z), material->isSolid() )
						createQuad( pos, 1|2|4|8, 4|8 , 2|4,	vec3i(-1,0,0),	*material );
					if( material = &grid.get(x,y+1,z), material->isSolid() )
						createQuad( pos, 2|4, 1|2|4|8 , 4|8,	vec3i(0,-1,0),	*material );
					if( material = &grid.get(x,y-1,z), material->isSolid() )
						createQuad( pos, 4|8, 0 , 2|4,			vec3i(0,1,0),	*material );
					if( material = &grid.get(x,y,z+1), material->isSolid() )
						createQuad( pos, 4|8, 2|4, 1|2|4|8,		vec3i(0,0,-1),	*material );
					if( material = &grid.get(x,y,z-1), material->isSolid() )
						createQuad( pos, 2|4, 4|8, 0,			vec3i(0,0,1),	*material );
				}
			}
		}
		timings["40_build mesh"] = t.getMilliseconds();
		t.reset();
	}

	for(auto& it:timings){
		std::cout << it.first<<"\t"<<it.second<<std::endl;
	}
	
	return mb.buildMesh();

}


}
#endif /* MINSG_EXT_VOXEL_WORLD */
