/*
	This file is part of the MinSG library extension ThesisJonas.
	Copyright (C) 2013 Jonas Knoll

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_THESISJONAS

#include "Preprocessor.h"

#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Box.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Point.h>
#include <Geometry/PointOctree.h>
#include <Geometry/Sphere.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec4.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>
#include <Util/Graphics/Color.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <Util/StringIdentifier.h>

#include <cmath>
#include <random>
#include <chrono>

typedef Util::ReferenceAttribute<Rendering::Mesh> SamplingMeshAttribute;

namespace Rendering {
class PositionAttributeAccessor;
}

namespace MinSG {
namespace ThesisJonas {

bool hasSamplingMesh(Node * node, bool checkProto /*=true*/) {
	if(checkProto){
		bool protoSM = node->isInstance() ? node->getPrototype()->isAttributeSet(SAMPLING_MESH_ID) : false;
		return node->isAttributeSet(SAMPLING_MESH_ID) || protoSM;
	} else {
		return node->isAttributeSet(SAMPLING_MESH_ID);
	}
}

Rendering::Mesh * retrieveSamplingMesh(Node * node) {
	if(!hasSamplingMesh(node, true)) {
		throw std::logic_error("Attribute SamplingMesh not found");
	}
	Util::GenericAttribute * attribute = node->findAttribute(SAMPLING_MESH_ID);
	SamplingMeshAttribute * samplingMeshAttribute = dynamic_cast<SamplingMeshAttribute *>(attribute);
	if(samplingMeshAttribute == nullptr) {
		throw std::logic_error("Attribute has wrong type");
	}
	return samplingMeshAttribute->get();
}

void storeSamplingMesh(Node * node, Rendering::Mesh * mesh) {
	Util::GenericAttribute * attribute = node->getAttribute(SAMPLING_MESH_ID);
	if(attribute != nullptr) {
		throw std::logic_error("Attribute already exists");
	}
	auto meshAttribute = new SamplingMeshAttribute(mesh);
	node->setAttribute(SAMPLING_MESH_ID, meshAttribute);
}

void removeSamplingMesh(Node * node, bool checkProto /*=true*/) {
	if(hasSamplingMesh(node, false))
		node->unsetAttribute(SAMPLING_MESH_ID);
	else if(hasSamplingMesh(node, true) && checkProto)
		node->getPrototype()->unsetAttribute(SAMPLING_MESH_ID);
}

void updateSamplingMesh(Node * node, Rendering::Mesh * mesh) {
	removeSamplingMesh(node, false);
	storeSamplingMesh(node, mesh);
}

struct _VisitedMesh{
	Rendering::Mesh * mesh;
	Util::Reference<Rendering::PositionAttributeAccessor> posaa;
	Util::Reference<Rendering::NormalAttributeAccessor> normalaa;
	Util::Reference<Rendering::ColorAttributeAccessor> coloraa;
	uint32_t * indexData;
	std::vector<uint32_t> indexPointer;
	Geometry::Matrix4x4 worldMatrix;

	~_VisitedMesh(){
		/* TODO: this is just a workaround
		 * real problem: opened meshes seem not to be reuploaded/rendered with uploaded data when sample subtree is done (remove *mesh when fixed)
		*/
		mesh->_getIndexData().upload();
		mesh->_getVertexData().upload();
	}
};

static void countUsedNodes(Node * node, unsigned int & count){
	if(node->isAttributeSet(PREPROCESSING_NODE_ID)){
		count++;
	}
}

static void openMesh(Node * displayedNode, Node * processedNode, std::vector<double> parameters, std::vector<_VisitedMesh *> & vm){
	bool useOriginal = parameters[POS_USE_ORIGINAL];
	bool storeAtProto = parameters[POS_STORE_AT_PROTO];
	if(displayedNode->isAttributeSet(PREPROCESSING_NODE_ID)){
		const Util::_NumberAttribute<double> * attribute = dynamic_cast<Util::_NumberAttribute<double> *>(displayedNode->getAttribute(PREPROCESSING_NODE_ID));
		if(attribute != nullptr){
			// node has NODE_ID and therefore is part of the preprocessing step
			unsigned int id = static_cast<unsigned int>(attribute->get());

			if(vm[id]==nullptr){
				auto vmp = new _VisitedMesh();
				// mesh has not been opened yet, init everything
				Rendering::Mesh * m;
				GeometryNode * geoNode = dynamic_cast<GeometryNode *>(displayedNode);
				if((geoNode != nullptr && useOriginal) || (geoNode != nullptr && !useOriginal && !hasSamplingMesh(geoNode))){
					// use original geometry for approximation of parent
					m = geoNode->getMesh();

					// geometry needs to be transformed (TODO: conditions unnecessary, all the same)
					if(geoNode->isInstance()){
						if(processedNode->isInstance() && storeAtProto){
							// mesh comes form prototype and is stored at prototype (geoNode correct)
							vmp->worldMatrix = processedNode->getWorldMatrix().inverse()*displayedNode->getWorldMatrix();
						} else {
							// mesh comes from prototype and is stored at node (listNode correct)
							vmp->worldMatrix = processedNode->getWorldMatrix().inverse()*displayedNode->getWorldMatrix();
						}
					} else {
						if(processedNode->isInstance() && storeAtProto){
							// mesh comes form node and is stored at prototype (case not possible?!?)
							vmp->worldMatrix = processedNode->getWorldMatrix().inverse()*displayedNode->getWorldMatrix();
						} else {
							// mesh comes from node and is stored at node (geoNode correct)
							vmp->worldMatrix = processedNode->getWorldMatrix().inverse()*displayedNode->getWorldMatrix();
						}
					}
				} else {
					// use sampling mesh for approximation of parent
					m = retrieveSamplingMesh(displayedNode);

					// geometry needs to be transformed (TODO: conditions unnecessary, all the same)
					if(!hasSamplingMesh(displayedNode, false)){
						if(processedNode->isInstance() && storeAtProto){
							// sampling mesh comes form prototype and is stored at prototype (listNode correct)
							vmp->worldMatrix = processedNode->getWorldMatrix().inverse()*displayedNode->getWorldMatrix();
						} else {
							// sampling mesh comes from prototype and is stored at node (listNode correct)
							vmp->worldMatrix = processedNode->getWorldMatrix().inverse()*displayedNode->getWorldMatrix();
						}
					} else {
						if(processedNode->isInstance() && storeAtProto){
							// sampling mesh comes form node and is stored at prototype (case not possible?!?)
							vmp->worldMatrix = processedNode->getWorldMatrix().inverse()*displayedNode->getWorldMatrix();
						} else {
							// sampling mesh comes from node and is stored at node (listNode correct)
							vmp->worldMatrix = processedNode->getWorldMatrix().inverse()*displayedNode->getWorldMatrix();
						}
					}
				}

				// store mesh data
				vmp->mesh = m;
				Rendering::MeshVertexData & vertexData = m->openVertexData();
				vmp->posaa = Rendering::PositionAttributeAccessor::create(vertexData,Rendering::VertexAttributeIds::POSITION);

				Util::Reference<Rendering::NormalAttributeAccessor> normalaa;
				try {
					vmp->normalaa = Rendering::NormalAttributeAccessor::create(vertexData,Rendering::VertexAttributeIds::NORMAL);
				} catch(const std::exception & e) {}

				Util::Reference<Rendering::ColorAttributeAccessor> coloraa;
				try {
					vmp->coloraa = Rendering::ColorAttributeAccessor::create(vertexData,Rendering::VertexAttributeIds::COLOR);
				} catch(const std::exception & e) {}

				vmp->indexData = m->openIndexData().data();
				vmp->indexPointer.assign(m->openIndexData().getIndexCount(), std::numeric_limits<uint32_t>::max()); // max means not initialized yet

				vm[id] = vmp;
			} else {
				// mesh has been opened before, open vertex and index data again
				_VisitedMesh * vmp = vm[id];

				// open mesh data
				Rendering::MeshVertexData & vertexData = vmp->mesh->openVertexData();
				vmp->posaa = Rendering::PositionAttributeAccessor::create(vertexData,Rendering::VertexAttributeIds::POSITION);

				Util::Reference<Rendering::NormalAttributeAccessor> normalaa;
				try {
					vmp->normalaa = Rendering::NormalAttributeAccessor::create(vertexData,Rendering::VertexAttributeIds::NORMAL);
				} catch(const std::exception & e) {}

				Util::Reference<Rendering::ColorAttributeAccessor> coloraa;
				try {
					vmp->coloraa = Rendering::ColorAttributeAccessor::create(vertexData,Rendering::VertexAttributeIds::COLOR);
				} catch(const std::exception & e) {}

				vmp->indexData = vmp->mesh->openIndexData().data();
			}
		}
	}
}

void RGBtoCIELab(const Util::Color4f & c_in, float c_out[3]) {
	// http://www.easyrgb.com/index.php?X=MATH
	// RGB to XYZ
	float R = c_in.getR();
	float G = c_in.getG();
	float B = c_in.getB();

	if(R>0.04045f )
		R = pow((R+0.055f)/1.055f , 2.4f);
	else
		R = R/12.92f;

	if(G>0.04045f)
		G = pow((G+0.055f)/1.055f, 2.4f);
	else
		G = G/12.92f;

	if(B>0.04045f)
		B = pow((B+0.055f)/1.055f, 2.4);
	else
		B = B/12.92f;

	R *= 100;
	G *= 100;
	B *= 100;

	//Observer. = 2deg, Illuminant = D65
	float X = R * 0.4124f + G * 0.3576f + B * 0.1805f;
	float Y = R * 0.2126f + G * 0.7152f + B * 0.0722f;
	float Z = R * 0.0193f + G * 0.1192f + B * 0.9505f;


	// XYZ to CIE-L*ab
	X = X / 95.047f;
	Y = Y / 100.000;
	Z = Z / 108.883;

	if(X>0.008856f)
		X = pow(X, 1.0f/3.0f);
	else
		X = (7.787f * X) + (16.0f / 116.0f);
	if(Y>0.008856f)
		Y = pow(Y, 1.0f/3.0f);
	else
		Y = (7.787f * Y) + (16.0f / 116.0f);
	if(Z>0.008856f)
		Z = pow(Z, 1.0f/3.0f);
	else
		Z = (7.787f * Z) + (16.0f / 116.0f);

	float CIE_L = (116.0f * Y ) - 16.0f;
	float CIE_a = 500.0f * (X-Y);
	float CIE_b = 200.0f * (Y-Z);

	c_out[0] = CIE_L;
	c_out[1] = CIE_a;
	c_out[2] = CIE_b;
}

void extractTrianglesFromImage( std::vector<Util::PixelAccessor *> imgs,
								Node * node,
								std::vector<double> parameters,
								bool & usingNormal,
								bool & usingColor,
								uint32_t & maxSingleTrianglePixels,
								uint32_t & totalUsedPixels,
								std::vector<ExtractedTriangle> & triangles) {
	// open meshes
	unsigned int nodeCount = 0;
	forEachNodeBottomUp<Node>(node, std::bind(&countUsedNodes, std::placeholders::_1, std::ref(nodeCount)));
	std::vector<_VisitedMesh *> vm(nodeCount, nullptr);
	forEachNodeBottomUp<Node>(node, std::bind(&openMesh, std::placeholders::_1, node, parameters, std::ref(vm)));

	// update indicator variables
	usingNormal = false;
	usingColor = false;
	for(unsigned int i=0; i<nodeCount && (!usingNormal || !usingColor); ++i){
		if(vm[i]->normalaa.isNotNull())
			usingNormal = true;
		if(vm[i]->coloraa.isNotNull())
			usingColor = true;
	}

	// extract triangles
	maxSingleTrianglePixels = 1;
	totalUsedPixels = 0;

	// traverse all images
	for(auto & imgs_i : imgs){
		Util::PixelAccessor * img = imgs_i;
		const uint32_t width = img->getWidth();
		const uint32_t height = img->getHeight();

		// traverse all pixels in image
		for(uint32_t x=0;x<width;++x){
			for(uint32_t y=0;y<height;++y){
				const Util::Color4f p = img->readColor4f(x,y);
				if(p.getA()>0){
					// get PREPROCESSING_NODE_ID and triangle id (index) from image
					uint32_t nodeID = p.getG();
					uint32_t index = 3*p.getR(); // triangles in image are counted as 0, 1, 2, ...; but triangles in indexData consist of 3 values

					if(vm[nodeID]->indexPointer[index] == std::numeric_limits<uint32_t>::max()){
						// new triangle found
						// get vertices of found triangle
						uint32_t a = vm[nodeID]->indexData[index];
						uint32_t b = vm[nodeID]->indexData[index+1];
						uint32_t c = vm[nodeID]->indexData[index+2];

						// extract position
						Geometry::Triangle<Geometry::Vec3> pos(	vm[nodeID]->worldMatrix.transformPosition(vm[nodeID]->posaa->getPosition(a)),
																vm[nodeID]->worldMatrix.transformPosition(vm[nodeID]->posaa->getPosition(b)),
																vm[nodeID]->worldMatrix.transformPosition(vm[nodeID]->posaa->getPosition(c)));

						// extract normal if available
						Geometry::Triangle<Geometry::Vec3> normal(Geometry::Vec3(0.0,0.0,0.0), Geometry::Vec3(0.0,0.0,0.0), Geometry::Vec3(0.0,0.0,0.0));
						if(vm[nodeID]->normalaa.isNotNull()) {
							normal = Geometry::Triangle<Geometry::Vec3>(vm[nodeID]->normalaa->getNormal(a), vm[nodeID]->normalaa->getNormal(b), vm[nodeID]->normalaa->getNormal(c));
						}

						// extract color if available
						Util::Color4f color_a;
						Util::Color4f color_b;
						Util::Color4f color_c;
						if(vm[nodeID]->coloraa.isNotNull()) {
							color_a = vm[nodeID]->coloraa->getColor4f(a);
							color_b = vm[nodeID]->coloraa->getColor4f(b);
							color_c = vm[nodeID]->coloraa->getColor4f(c);
						}

						// store values
						triangles.emplace_back(pos, normal, color_a, color_b, color_c, usingColor);

						// update index pointer of current triangle
						vm[nodeID]->indexPointer[index] = triangles.size()-1;
						++totalUsedPixels;
					} else {
						// triangle already stored: increase pixel count and update maxSingleTrianglePixels
						if(++(triangles[vm[nodeID]->indexPointer[index]].numPixels) > maxSingleTrianglePixels)
							maxSingleTrianglePixels = triangles[vm[nodeID]->indexPointer[index]].numPixels;

						++totalUsedPixels;
					}
				}
			}
		}
	}

	// clear memory
	for(auto & elem : vm){
		delete(elem);
	}
}

static inline void addTriangleToMeshBuilder(ExtractedTriangle & triangle, Util::Reference<Rendering::MeshUtils::MeshBuilder> & mb, bool useNormal, bool useColor){
	mb->addIndex(mb->getNextIndex());
	mb->position(triangle.triangle.getVertexA());
	if(useNormal)
		mb->normal(triangle.normal.getVertexA());
	if(useColor)
		mb->color(triangle.color_a);
	mb->addVertex();

	mb->addIndex(mb->getNextIndex());
	mb->position(triangle.triangle.getVertexB());
	if(useNormal)
		mb->normal(triangle.normal.getVertexB());
	if(useColor)
		mb->color(triangle.color_b);
	mb->addVertex();

	mb->addIndex(mb->getNextIndex());
	mb->position(triangle.triangle.getVertexC());
	if(useNormal)
		mb->normal(triangle.normal.getVertexC());
	if(useColor)
		mb->color(triangle.color_c);
	mb->addVertex();
}

Rendering::Mesh * selectTriangles(	const std::vector<ExtractedTriangle> & triangles,
									bool useNormal,
									bool useColor,
									std::vector<double> parameters,
									float sceneDiameter,
									uint32_t maxSingleTrianglePixels,
									uint32_t /*totalUsedPixels*/,
									bool useTimeSeed){
	if(triangles.size()==0){
		// no triangles -> return empty mesh
		std::cout<<"no triangles found, returning empty mesh\n";
		return new Rendering::Mesh();
	}

	uint32_t maxAbsTriangles = static_cast<uint32_t>(parameters[POS_MAX_ABS_TRIANGLES]);
	uint32_t samplesPerRound = static_cast<uint32_t>(parameters[POS_SAMPLES_PER_ROUND]);
	uint32_t acceptSamples = static_cast<uint32_t>(parameters[POS_ACCEPT_SAMPLES]);
	double reusalRate = parameters[POS_REUSAL_RATE];

	Geometry::Box bb;
	bb.invalidate();

	// calculate bounding box and initialize sourceTriangle-mapping
	std::deque<size_t> sourceTriangleIds(triangles.size());
	{
		size_t i=0;
		for(const auto & triangle : triangles){
			bb.include(triangle.triangle.getVertexA());
			bb.include(triangle.triangle.getVertexB());
			bb.include(triangle.triangle.getVertexC());
			sourceTriangleIds[i] = i;
			++i;
		}
	}

	// randomize triangles
	{
		std::default_random_engine rEngine;
		if(useTimeSeed)
			rEngine = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
		std::shuffle(sourceTriangleIds.begin(),sourceTriangleIds.end(),rEngine );
	}

	// generate the mesh
	Rendering::VertexDescription vd;
	vd.appendPosition3D();
	if(useNormal){
		vd.appendNormalByte();
	}
	if(useColor){
		vd.appendColorRGBAByte();
	}
	Util::Reference<Rendering::MeshUtils::MeshBuilder> mb = new Rendering::MeshUtils::MeshBuilder(vd);

	struct OctreeEntry : public Geometry::Point<Geometry::Vec3> {
		size_t index;
		OctreeEntry(size_t i,const Geometry::Vec3 & p) : Geometry::Point<Geometry::Vec3>(p), index(i) {}
	};

	Geometry::PointOctree<OctreeEntry> octree(bb,bb.getExtentMax()*0.01,8);
	size_t triangleCount = 0;

	// add initial triangle
	{
		size_t triangleId = sourceTriangleIds.front();
		sourceTriangleIds.pop_front();
		ExtractedTriangle triangle = triangles[triangleId];

		addTriangleToMeshBuilder(triangle, mb, useNormal, useColor);
		++triangleCount;

		switch(static_cast<int>(parameters[POS_ALG])){
			case ALG_MID_POINT_DIST:
			case ALG_MID_POINT_DIST_SIZE_WEIGHT:
			case ALG_THESIS_FACTORS:
			case ALG_THESIS_WEIGHTS:
				{
					const Geometry::Triangle<Geometry::Vec3> & t = triangles[triangleId].triangle;
					Geometry::Vec3 center = ( t.getVertexA() + t.getVertexB() + t.getVertexC() ) / 3.0;
					octree.insert(OctreeEntry(triangleId,center));
				}
				break;

			case ALG_SIZE:
			case ALG_RANDOM:
				break;

			default:
				throw std::logic_error("Algorithm not found.");
		}
	}

	struct QS {
		bool operator()(const std::pair<size_t, float> &a, const std::pair<size_t, float> &b)const{
			return a.second<b.second;
		}
	} qualitySorter;

	std::vector<std::pair<size_t, float> > samples; // triangleId

	std::cout << "Overall number:" << triangles.size();
	// selection of triangles
	uint32_t round = 1;
	while(triangleCount<maxAbsTriangles && !sourceTriangleIds.empty()){
		if(sourceTriangleIds.size()<samplesPerRound)
			samplesPerRound = sourceTriangleIds.size();

		// draw samples and calculate their fitness
		for(size_t i=0;i<samplesPerRound;++i){
			const size_t tId = sourceTriangleIds.front();
			sourceTriangleIds.pop_front();

			float fitness=0;
			switch(static_cast<int>(parameters[POS_ALG])){
				case ALG_MID_POINT_DIST:
					{
						// calculate squared distance to closest triangle center point
						std::deque<OctreeEntry> closest;
						const Geometry::Triangle<Geometry::Vec3> & t = triangles[tId].triangle;
						Geometry::Vec3 tCenter = ( t.getVertexA() + t.getVertexB() + t.getVertexC() ) / 3.0;
						octree.getClosestPoints(tCenter, 1, closest);
						fitness = tCenter.distanceSquared(closest.front().getPosition());
					}
					break;

				case ALG_MID_POINT_DIST_SIZE_WEIGHT:
					{
						// calculate squared distance to closest triangle center point
						std::deque<OctreeEntry> closest;
						const Geometry::Triangle<Geometry::Vec3> & t = triangles[tId].triangle;
						Geometry::Vec3 tCenter = ( t.getVertexA() + t.getVertexB() + t.getVertexC() ) / 3.0;
						octree.getClosestPoints(tCenter, 1, closest);
						fitness = parameters[POS_WEIGHT_POS] * tCenter.distanceSquared(closest.front().getPosition())
								+ parameters[POS_WEIGHT_SIZE] * triangles[tId].numPixels;
					}
					break;

				case ALG_THESIS_FACTORS:
					{
						// calculate preference as in thesis with color and angle as factors

						// get closest neighbor
						std::deque<OctreeEntry> neighbors;
						const Geometry::Triangle<Geometry::Vec3> & t = triangles[tId].triangle;
						Geometry::Vec3 tCenter = ( t.getVertexA() + t.getVertexB() + t.getVertexC() ) / 3.0;
						octree.getClosestPoints(tCenter, 1, neighbors);
						OctreeEntry closest(neighbors.front());
						float minDist = tCenter.distance(closest.getPosition());
						float posFactor = minDist/sceneDiameter;

						// get neighborhood
						neighbors.clear();
						Geometry::Sphere_f sphere(closest.getPosition(), 2.0f*minDist);
						octree.collectPointsWithinSphere(sphere, neighbors);

						// calculate cosMinAngle
						float cosMinAngle = -1;
						for(const OctreeEntry & neighbor : neighbors){
							Geometry::Vec3 normalA = t.calcNormal();
							Geometry::Vec3 normalB = triangles[neighbor.index].triangle.calcNormal();
							float cosAngle = normalA.x()*normalB.x() + normalA.y()*normalB.y() + normalA.z()*normalB.z();
							if(cosAngle>cosMinAngle)
								cosMinAngle = cosAngle;
						}

						float angleFactor = (1-cosMinAngle)/2;

						// get size factor
						float sizeFactor = static_cast<float>(triangles[tId].numPixels)/maxSingleTrianglePixels;

						// get color factor
						float colorFactor = 375.5955271f;

						for(const OctreeEntry & neighbor : neighbors){
							float colorDist = sqrt(	pow(triangles[tId].mid_color[0]-triangles[neighbor.index].mid_color[0], 2)+
													pow(triangles[tId].mid_color[1]-triangles[neighbor.index].mid_color[1], 2)+
													pow(triangles[tId].mid_color[2]-triangles[neighbor.index].mid_color[2], 2));
							if(colorDist<colorFactor)
								colorFactor = colorDist;
						}

						colorFactor /= 375.5955271f;

						// use weights as as they where parsed
						const float w_pos = parameters[POS_WEIGHT_POS];
						const float w_ang = parameters[POS_WEIGHT_NORMAL];
						const float w_size = parameters[POS_WEIGHT_SIZE];
						const float w_col = parameters[POS_WEIGHT_COLOR];

						fitness = pow(1+posFactor, w_pos) * pow(1+angleFactor, w_ang) * pow(1+sizeFactor, w_size) * pow(1+colorFactor, w_col);
					}
					break;

				case ALG_THESIS_WEIGHTS:
					{
						// calculate preference as in thesis with color and angle changing weights

						// get closest neighbor
						std::deque<OctreeEntry> neighbors;
						const Geometry::Triangle<Geometry::Vec3> & t = triangles[tId].triangle;
						Geometry::Vec3 tCenter = ( t.getVertexA() + t.getVertexB() + t.getVertexC() ) / 3.0;
						octree.getClosestPoints(tCenter, 1, neighbors);
						OctreeEntry closest(neighbors.front());
						float minDist = tCenter.distance(closest.getPosition());
						float posFactor = minDist/sceneDiameter;

						// get neighborhood
						neighbors.clear();
						Geometry::Sphere_f sphere(closest.getPosition(), 2.0f*minDist);
						octree.collectPointsWithinSphere(sphere, neighbors);

						// calculate cosMinAngle
						float cosMinAngle = -1;
						for(const OctreeEntry & neighbor : neighbors){
							Geometry::Vec3 normalA = t.calcNormal();
							Geometry::Vec3 normalB = triangles[neighbor.index].triangle.calcNormal();
							float cosAngle = normalA.x()*normalB.x() + normalA.y()*normalB.y() + normalA.z()*normalB.z();
							if(cosAngle>cosMinAngle)
								cosMinAngle = cosAngle;
						}

						float angleFactor = (1-cosMinAngle)/2;

						// get size factor
						float sizeFactor = static_cast<float>(triangles[tId].numPixels)/maxSingleTrianglePixels;

						// get color factor
						float colorFactor = 375.5955271f; // maximum color dist

						for(const OctreeEntry & neighbor : neighbors){
							float colorDist = sqrt(	pow(triangles[tId].mid_color[0]-triangles[neighbor.index].mid_color[0], 2)+
													pow(triangles[tId].mid_color[1]-triangles[neighbor.index].mid_color[1], 2)+
													pow(triangles[tId].mid_color[2]-triangles[neighbor.index].mid_color[2], 2));
							if(colorDist<colorFactor)
								colorFactor = colorDist;
						}

						colorFactor /= 375.5955271f; // normalize color dist

						// update weights
						const float w_ang = parameters[POS_WEIGHT_NORMAL];
						const float w_col = parameters[POS_WEIGHT_COLOR];

						float w_pos = parameters[POS_WEIGHT_POS] * pow(1+angleFactor, w_ang) * pow(1+colorFactor, w_col);
						float w_size = parameters[POS_WEIGHT_SIZE];

						// normalize weights
						w_pos = w_pos / (w_pos + w_size);
						w_size = w_size / (w_pos + w_size);


						fitness = pow(1+posFactor, w_pos) * pow(1+sizeFactor, w_size);
					}
					break;

				case ALG_SIZE:
					fitness = triangles[tId].numPixels;

				case ALG_RANDOM:
					break;

				default:
					throw std::logic_error("Algorithm not found.");
			}
			samples.emplace_back(tId, fitness);
		}
		// sort samples according to fitness
		switch(static_cast<int>(parameters[POS_ALG])){
			case ALG_MID_POINT_DIST:
			case ALG_MID_POINT_DIST_SIZE_WEIGHT:
			case ALG_THESIS_FACTORS:
			case ALG_THESIS_WEIGHTS:
			case ALG_SIZE:
				{
					// highest quality at the back
					std::sort(samples.begin(),samples.end(),qualitySorter);
				}
				break;

			case ALG_RANDOM:
				// no sorting -> take random element
				break;

			default:
				throw std::logic_error("Algorithm not found.");
		}

		// add accepted samples to mesh
		for(unsigned int j = 0;j<acceptSamples && !samples.empty(); ++j ){
			auto & sample = samples.back();
			samples.pop_back();
			ExtractedTriangle t = triangles[sample.first];

			addTriangleToMeshBuilder(t, mb, useNormal, useColor);
			++triangleCount;

			switch(static_cast<int>(parameters[POS_ALG])){
				case ALG_MID_POINT_DIST_SIZE_WEIGHT:
				case ALG_MID_POINT_DIST:
				case ALG_THESIS_FACTORS:
				case ALG_THESIS_WEIGHTS:
					{
						Geometry::Vec3 tCenter = ( t.triangle.getVertexA() + t.triangle.getVertexB() + t.triangle.getVertexC() ) / 3.0;
						octree.insert(OctreeEntry(sample.first,tCenter));
					}
					break;

				case ALG_SIZE:
				case ALG_RANDOM:
					break;

				default:
					throw std::logic_error("Algorithm not found.");
			}
		}

		// round end (some cleanup and new calculation of parameters for next round)
		// shuffle samples
		if( (round%10000) == 100){
			std::default_random_engine rEngine;
			if(useTimeSeed)
				rEngine = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
			std::shuffle(sourceTriangleIds.begin(),sourceTriangleIds.end(),rEngine );
		}
		if( (round%500) == 0){
			samplesPerRound = std::max(samplesPerRound*0.5f,20.0f);
			acceptSamples = std::min( static_cast<unsigned int>(samplesPerRound*0.3), acceptSamples+1);
		}

		// add samples for reuse
		for(size_t i=static_cast<size_t>(samples.size()*reusalRate);i>0;--i){
			const size_t tId = samples.back().first;
			samples.pop_back();
			sourceTriangleIds.push_back(tId);
		}
		samples.clear();
		++round;
	}
	std::cout << " \t target number:" << maxAbsTriangles;
	std::cout << " \t created:" << triangleCount;
	std::cout << std::endl;

	auto mesh = mb->buildMesh();
	mesh->setDrawMode(Rendering::Mesh::DRAW_TRIANGLES);
	Rendering::MeshUtils::eliminateDuplicateVertices(mesh);
	return mesh;
}

void approximateNodeFromImage(Node * node, std::vector<Util::PixelAccessor *> imgs, std::vector<double> parameters) {
	bool usingNormal;
	bool usingColor;
	uint32_t maxSingleTrianglePixels;
	uint32_t totalUsedPixels;
	std::vector<ExtractedTriangle> triangles;

	extractTimer.resume();
	extractTrianglesFromImage(imgs, node, parameters, usingNormal, usingColor, maxSingleTrianglePixels, totalUsedPixels, triangles);
	extractTimer.stop();
	selectTimer.resume();
	Rendering::Mesh * mesh = selectTriangles(triangles, usingNormal, usingColor, parameters, node->getBB().getDiameter(), maxSingleTrianglePixels, totalUsedPixels, true);
	selectTimer.stop();

	// store sampling mesh at node
	if(parameters[POS_STORE_AT_PROTO]!=0 && node->isInstance())
		updateSamplingMesh(node->getPrototype(), mesh);
	else
		updateSamplingMesh(node, mesh);
}


void randomizeTriangleOrder(GeometryNode * geoNode){
	std::cout << "Starting randomization of triangles...\n";
	Rendering::Mesh * m = geoNode->getMesh();
	Rendering::MeshIndexData id = m->openIndexData();
	struct _triangle {
		uint32_t a;
		uint32_t b;
		uint32_t c;
		_triangle(uint32_t p_a, uint32_t p_b, uint32_t p_c) : a(p_a), b(p_b), c(p_c) {}
	};

	// get triangles
	std::vector<_triangle> triangles;
	for(uint32_t i=0; i<id.getIndexCount(); i+=3) {
		triangles.emplace_back(id[i], id[i+1], id[i+2]);
	}
	// shuffle triangles
	std::default_random_engine rEngine;
	rEngine = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
	std::shuffle(triangles.begin(), triangles.end(), rEngine);
	// build new index data
	Rendering::MeshIndexData id2;
	id2.allocate(id.getIndexCount());
	uint32_t * data = id2.data();
	for(uint32_t i=0; i<triangles.size(); ++i) {
		data[3*i] = triangles[i].a;
		data[3*i+1] = triangles[i].b;
		data[3*i+2] = triangles[i].c;
	}
	id2.markAsChanged();

	// build new mesh
	auto m2 = new Rendering::Mesh(id2, m->openVertexData());

	// store mesh as attribute
	if(geoNode->isInstance())
		updateSamplingMesh(geoNode->getPrototype(), m2);
	else
		updateSamplingMesh(geoNode, m2);
}

void printAndResetTimers() {
	std::cout<<"extractTimer: "<<extractTimer.getMilliseconds()<<" ms; selectTimer: "<<selectTimer.getMilliseconds()<<" ms;\n";
	extractTimer.reset();
	extractTimer.stop();
	selectTimer.reset();
	selectTimer.stop();
}

void printHistoHistogramOfNode(Node * n){
	GeometryNode * geoNode = dynamic_cast<GeometryNode *>(n);
	if(geoNode != nullptr){
		Rendering::Mesh * m = geoNode->getMesh();
		std::cout << "Histogram of node's mesh\n";
		printIndexHistoHistogram(m);
	}
	if(hasSamplingMesh(n)){
		Rendering::Mesh * m = retrieveSamplingMesh(n);
		std::cout << "Histogram of SM\n";
		printIndexHistoHistogram(m);
	}
}
void printIndexHistoHistogram(Rendering::Mesh * mesh){
	Rendering::MeshIndexData id = mesh->openIndexData();
	uint32_t indexHistogramLength = id.getMaxIndex()+1;
	auto indexHistogram = new uint32_t[indexHistogramLength]();
	for(uint32_t i=0; i<id.getMaxIndex(); ++i){
		indexHistogram[i] = 0;
	}

	uint32_t max = 0;
	for(uint32_t i=0; i<id.getIndexCount(); ++i){
		uint32_t index = id[i];
		indexHistogram[index] += 1;
		if(indexHistogram[index]>max){
			max = indexHistogram[index];
		}
	}

	max++;
	auto indexHistoHistogram = new uint32_t[max]();
	std::fill(indexHistoHistogram, indexHistoHistogram + max, 0);
	for(uint32_t i=0; i<id.getMaxIndex(); ++i){
		indexHistoHistogram[indexHistogram[i]] += 1;
	}

	std::cout << "indexHistoHistogram: ";
	for(uint32_t i=0; i<max; ++i)
		std::cout << indexHistoHistogram[i] << ", ";
	std::cout << "\n";

	delete(indexHistogram);
	delete(indexHistoHistogram);
}


/************************************************************************************************************************************
 *                            Second Implementation using static Variables for single Image Preprocessing
 */

static Node * prep_node;
static std::vector<double> prep_parameters;
static bool prep_usingNormal;
static bool prep_usingColor;
static uint32_t prep_maxSingleTrianglePixels;
static uint32_t prep_totalUsedPixels;
static std::vector<ExtractedTriangle> prep_triangles;
static std::vector<_VisitedMesh *> prep_vm;

void initPreprocessing(Node * node, std::vector<double> parameters){
	prep_node = node;
	prep_parameters = parameters;

	// open meshes
	unsigned int nodeCount = 0;
	forEachNodeBottomUp<Node>(prep_node, std::bind(&countUsedNodes, std::placeholders::_1, std::ref(nodeCount)));
	prep_vm.resize(nodeCount);
	forEachNodeBottomUp<Node>(prep_node, std::bind(&openMesh, std::placeholders::_1, prep_node, prep_parameters, std::ref(prep_vm)));

	// update indicator variables
	prep_usingNormal = false;
	prep_usingColor = false;
	for(unsigned int i=0; i<nodeCount && (!prep_usingNormal || !prep_usingColor); ++i){
		if(prep_vm[i]->normalaa.isNotNull())
			prep_usingNormal = true;
		if(prep_vm[i]->coloraa.isNotNull())
			prep_usingColor = true;
	}

	// extract triangles
	prep_maxSingleTrianglePixels = 1;
	prep_totalUsedPixels = 0;
}

void extractTrianglesFromNewDirection(Util::PixelAccessor * img){
	// reopen meshes
	forEachNodeBottomUp<Node>(prep_node, std::bind(&openMesh, std::placeholders::_1, prep_node, prep_parameters, std::ref(prep_vm)));


	const uint32_t width = img->getWidth();
	const uint32_t height = img->getHeight();

	// traverse all pixels in image
	for(uint32_t x=0;x<width;++x){
		for(uint32_t y=0;y<height;++y){
			const Util::Color4f p = img->readColor4f(x,y);
			if(p.getA()>0){
				// get PREPROCESSING_NODE_ID and triangle id (index) from image
				uint32_t nodeID = p.getG();
				uint32_t index = 3*p.getR(); // triangles in image are counted as 0, 1, 2, ...; but triangles in indexData consist of 3 values

				if(prep_vm[nodeID]->indexPointer[index] == std::numeric_limits<uint32_t>::max()){
					// new triangle found
					// get vertices of found triangle
					uint32_t a = prep_vm[nodeID]->indexData[index];
					uint32_t b = prep_vm[nodeID]->indexData[index+1];
					uint32_t c = prep_vm[nodeID]->indexData[index+2];

					// extract position
					Geometry::Triangle<Geometry::Vec3> pos(	prep_vm[nodeID]->worldMatrix.transformPosition(prep_vm[nodeID]->posaa->getPosition(a)),
															prep_vm[nodeID]->worldMatrix.transformPosition(prep_vm[nodeID]->posaa->getPosition(b)),
															prep_vm[nodeID]->worldMatrix.transformPosition(prep_vm[nodeID]->posaa->getPosition(c)));

					// extract normal if available
					Geometry::Triangle<Geometry::Vec3> normal(Geometry::Vec3(0.0,0.0,0.0), Geometry::Vec3(0.0,0.0,0.0), Geometry::Vec3(0.0,0.0,0.0));
					if(prep_vm[nodeID]->normalaa.isNotNull()) {
						normal = Geometry::Triangle<Geometry::Vec3>(prep_vm[nodeID]->normalaa->getNormal(a), prep_vm[nodeID]->normalaa->getNormal(b), prep_vm[nodeID]->normalaa->getNormal(c));
					}

					// extract color if available
					Util::Color4f color_a;
					Util::Color4f color_b;
					Util::Color4f color_c;
					if(prep_vm[nodeID]->coloraa.isNotNull()) {
						color_a = prep_vm[nodeID]->coloraa->getColor4f(a);
						color_b = prep_vm[nodeID]->coloraa->getColor4f(b);
						color_c = prep_vm[nodeID]->coloraa->getColor4f(c);
					}

					// store values
					prep_triangles.emplace_back(pos, normal, color_a, color_b, color_c, prep_usingColor);

					// update index pointer of current triangle
					prep_vm[nodeID]->indexPointer[index] = prep_triangles.size()-1;
					++prep_totalUsedPixels;
				} else {
					// triangle already stored: increase pixel count and update maxSingleTrianglePixels
					if(++(prep_triangles[prep_vm[nodeID]->indexPointer[index]].numPixels) > prep_maxSingleTrianglePixels)
						prep_maxSingleTrianglePixels = prep_triangles[prep_vm[nodeID]->indexPointer[index]].numPixels;

					++prep_totalUsedPixels;
				}
			}
		}
	}
}

void finishPreprocessing(){
	// clear memory of prep_vm
	for(auto & elem : prep_vm){
		delete(elem);
	}
	prep_vm.clear();

	// sort triangles
	Rendering::Mesh * mesh = selectTriangles(prep_triangles,
					prep_usingNormal,
					prep_usingColor,
					prep_parameters,
					prep_node->getBB().getDiameter(),
					prep_maxSingleTrianglePixels,
					prep_totalUsedPixels,
					true);

	// store sampling mesh at node
	if(prep_parameters[POS_STORE_AT_PROTO]!=0 && prep_node->isInstance())
		updateSamplingMesh(prep_node->getPrototype(), mesh);
	else
		updateSamplingMesh(prep_node, mesh);


	prep_node = nullptr;
	prep_parameters.clear();
	prep_usingNormal = false;
	prep_usingColor = false;
	prep_maxSingleTrianglePixels = 0;
	prep_totalUsedPixels = 0;
	prep_triangles.clear();
}

}
}

#endif /* MINSG_EXT_THESISJONAS */
