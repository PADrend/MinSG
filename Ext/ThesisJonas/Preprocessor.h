/*
	This file is part of the MinSG library extension ThesisJonas.
	Copyright (C) 2013 Jonas Knoll

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_THESISJONAS

#ifndef MINSG_THESISJONAS_PREPROCESSOR_H_
#define MINSG_THESISJONAS_PREPROCESSOR_H_

#include "../../Core/Nodes/GeometryNode.h"
#include <Geometry/Triangle.h>
#include <Geometry/Vec3.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Timer.h>

namespace MinSG {
class CameraNodeOrtho;
class FrameContext;
class GroupNode;
class Node;

namespace ThesisJonas {
static const Util::StringIdentifier PREPROCESSING_NODE_ID("ThesisJonas_NodeId"); // node id for preprocessing to get correct mesh for a triangle
static const Util::StringIdentifier SAMPLING_MESH_ID("ThesisJonas_SamplingMesh");

static Util::Timer extractTimer;
static Util::Timer selectTimer;

/*
 * converts c from RGB to CIE-L*ab color space
 */
void RGBtoCIELab(const Util::Color4f & c_in, float c_out[3]);

struct ExtractedTriangle{
	Geometry::Triangle<Geometry::Vec3> triangle;
	Geometry::Triangle<Geometry::Vec3> normal;
	Util::Color4f color_a;
	Util::Color4f color_b;
	Util::Color4f color_c;
	float cielab_color_a [3];
	float cielab_color_b [3];
	float cielab_color_c [3];
	float mid_color [3];
	uint32_t numPixels;

	ExtractedTriangle()
			: triangle(Geometry::Vec3(), Geometry::Vec3(), Geometry::Vec3()),
			normal(Geometry::Vec3(), Geometry::Vec3(), Geometry::Vec3()),
			numPixels(1) {}
	ExtractedTriangle(	Geometry::Triangle<Geometry::Vec3>  _triangle,
						Geometry::Triangle<Geometry::Vec3>  _normal,
						Util::Color4f  _color_a,
						Util::Color4f  _color_b,
						Util::Color4f  _color_c,
						bool useCIELab)
			: triangle(std::move(_triangle)),normal(std::move(_normal)),color_a(std::move(_color_a)),color_b(std::move(_color_b)),color_c(std::move(_color_c)),numPixels(1){

		if(useCIELab){
			RGBtoCIELab(color_a, cielab_color_a);
			RGBtoCIELab(color_b, cielab_color_b);
			RGBtoCIELab(color_c, cielab_color_c);

			RGBtoCIELab((color_a+color_b+color_c)/3.0f, mid_color);
		}
	}

	bool operator==(const ExtractedTriangle&other)const{
		return triangle==other.triangle
				&& normal==other.normal
				&& color_a==other.color_a
				&& color_b==other.color_b
				&& color_c==other.color_c
				&& numPixels==other.numPixels;
	}
};

// some helper methods for rendering (TODO: move to extra helpers.cpp>)
/*
 * Checks and returns whether the node has a Sampling Mesh or not.
 * If node is an instance of a prototype, the prototype is also checked if checkProto is set.
 */
bool hasSamplingMesh(Node * node, bool checkProto=true);

/*
 * Returns node's Sampling Mesh.
 * When node is an instance of a prototype but has a Sampling Mesh for it own
 * this Sampling Mesh is returned. If the node is an instance and has no
 * Sampling Mesh but the prototype has one, that one is returned.
 */
Rendering::Mesh * retrieveSamplingMesh(Node * node);

/*
 * Given mesh is stored as Sampling Mesh attribute at given node.
 */
void storeSamplingMesh(Node * node, Rendering::Mesh * mesh);

/*
 * Removes Sampling Mesh of given node.
 * If the node has no Sampling Mesh then the Sampling Mesh of the prototype is removed.
 */
void removeSamplingMesh(Node * node, bool checkProto=true);

/*
 * Given mesh is stored as Sampling Mesh attribute at given node.
 * Existing attribute will be removed.
 */
void updateSamplingMesh(Node * node, Rendering::Mesh * mesh);


/*
 * Extracts triangle information from image and returns a vector containing visible triangles.
 *
 * Creates new ExtractedTriangle in vector for every triangle found.
 * Number of times a triangle was found (number of pixels in image) is stored in ExtractedTriangle.numPixels
 *
 * concept taken from MinSG/Ext/BlueSurfels/SurfelGenerator::extractSurfelsFromTextures
 *
 * TODO: store projected size (totalUsedPixels/imgPixels) at node?
 *
 * @param[in] imgs vector of images with index position as red and PREPROCESSING_NODE_ID as green component
 * @param[in] node extracting triangles of image from successors of node
 * @param[in] parameters useOriginal and storeAtProto will be used
 * @param[out] usingNormal indicates if normals were extracted
 * @param[out] usingColor indicates if colors were extracted
 * @param[out] maxSingleTrianglePixels returns the maximum number of Pixels a single triangle has (min 1)
 * @param[out] totalUsedPixels returns the total number of pixels that show triangles
 */
void extractTrianglesFromImage(	std::vector<Util::PixelAccessor *> imgs,
								Node * node,
								std::vector<double> parameters,
								bool & usingNormal,
								bool & usingColor,
								uint32_t & maxSingleTrianglePixels,
								uint32_t & totalUsedPixels,
								std::vector<ExtractedTriangle> & triangles);

// positions of parameters in vector
static const unsigned int POS_ALG = 0;
static const unsigned int POS_MAX_ABS_TRIANGLES = POS_ALG + 1;
static const unsigned int POS_ACCEPT_SAMPLES = POS_MAX_ABS_TRIANGLES + 1;
static const unsigned int POS_SAMPLES_PER_ROUND = POS_ACCEPT_SAMPLES + 1;
static const unsigned int POS_REUSAL_RATE = POS_SAMPLES_PER_ROUND + 1;
static const unsigned int POS_WEIGHT_POS = POS_REUSAL_RATE + 1;
static const unsigned int POS_WEIGHT_NORMAL = POS_WEIGHT_POS + 1;
static const unsigned int POS_WEIGHT_COLOR = POS_WEIGHT_NORMAL + 1;
static const unsigned int POS_WEIGHT_SIZE = POS_WEIGHT_COLOR + 1;
static const unsigned int POS_USE_ORIGINAL = POS_WEIGHT_SIZE + 1;
static const unsigned int POS_STORE_AT_PROTO = POS_USE_ORIGINAL + 1;

// algorithm numbers
static const unsigned int ALG_MID_POINT_DIST = 0;
static const unsigned int ALG_MID_POINT_DIST_SIZE_WEIGHT = ALG_MID_POINT_DIST + 1;
static const unsigned int ALG_THESIS_FACTORS = ALG_MID_POINT_DIST_SIZE_WEIGHT + 1;
static const unsigned int ALG_THESIS_WEIGHTS = ALG_THESIS_FACTORS + 1;
static const unsigned int ALG_SIZE = ALG_THESIS_WEIGHTS + 1;
static const unsigned int ALG_RANDOM = ALG_SIZE + 1;

/**
 * Selects triangles of given vector in a poisson-disk distribution and returns a mesh containing those triangles.
 *
 * concept taken from MinSG/Ext/BlueSurfels/SurfelGenerator::buildBlueSurfels
 *
 *
 * @param useNormal indicates if mesh with normals is created
 * @param useColor indicates if mesh with color is created
 * @param parameters vector of parameters with values in position POS_*
 * @param sceneDiameter diameter of the scene, can be used for normalization of position
 * @param maxSingleTrianglePixels number of pixels a single triangle had in image, can be used for normalization of size
 * @param totoalUsedPixels number of non-background pixels in image, can be used for normalization of size
 */

/*
 *
 *
 */
Rendering::Mesh * selectTriangles(
									const std::vector<ExtractedTriangle> & triangles,
									bool useNormal,
									bool useColor,
									std::vector<double> parameters,
									float sceneDiameter,
									uint32_t maxSingleTrianglePixels,
									uint32_t totalUsedPixels,
									bool useTimeSeed);

/**
 * Calculates approximation of node from given image and stores mesh as node attribute.
 *
 * @param parameters vector of parameters
 * 				(maxAbsTriangles[uint32],
 * 				 acceptSamples[uint32],
 * 				 samplesPerRound[uint32],
 * 				 reusalRate[double],
 * 				 weightPos[double],
 * 				 weightSize[double],
 * 				 weightNormal[double],
 * 				 weightColor[double])
 */
void approximateNodeFromImage(Node * node, std::vector<Util::PixelAccessor *> imgs, std::vector<double> parameters);

/**
 * Randomizes the order of triangles of the given geometry node and stores it as sampling mesh attribute
 */
void randomizeTriangleOrder(GeometryNode * geoNode);

/**
 * Prints measured times and resets timers of extractTimer and selectTimer.
 */
void printAndResetTimers();

void printHistoHistogramOfNode(Node * n);
void printIndexHistoHistogram(Rendering::Mesh * mesh);

/************************************************************************************************************************************
 *                            Second Implementation using static Variables for single Image Preprocessing
 */

void initPreprocessing(Node * node, std::vector<double> parameters);
void extractTrianglesFromNewDirection(Util::PixelAccessor * img);
void finishPreprocessing();

}
}

#endif /* MINSG_THESISJONAS_PREPROCESSOR_H_ */

#endif /* MINSG_EXT_THESISJONAS */
