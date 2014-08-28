/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "WriterDAE.h"
#include "../SceneDescription.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Nodes/CameraNodeOrtho.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/States/TextureState.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Util/IO/FileUtils.h>
#include <Util/Macros.h>
#include <Util/StringUtils.h>
#include <ctime>
#include <map>
#include <memory>

namespace MinSG {
namespace SceneManagement {
using namespace Util;
using namespace Rendering;

/**
 * Write a <source> tag for the given @a index of the @a mesh to the output stream.
 *
 * @param mesh Data source
 * @param attrNameId Attr type to take from the mesh
 * @param id Identifier for the XML tags
 * @param out Output stream
 * @param indention Indent the lines by this number of tabs
 */
static void outputMeshColorSource(Mesh * mesh, Util::StringIdentifier attrNameId,
								 const std::string & id,
								 std::ostream & out,
								 uint_fast8_t indention) {
	Util::Reference<ColorAttributeAccessor> accessor = ColorAttributeAccessor::create(mesh->openVertexData(), attrNameId);

	const uint32_t count = mesh->getVertexCount();
	const unsigned int numValues = 4;

	std::string tabs(indention, '\t');
	out << tabs << "<source id=\"" << id << "\">\n";
	out << tabs << "\t<float_array count=\"" << count * numValues << "\" id=\"" << id << "-array\">";
	for (uint32_t v = 0; v < count; ++v) {
		if (v != 0) {
			out << ' ';
		}
		out << accessor->getColor4f(v);
	}
	out << "</float_array>\n";
	out << tabs << "\t<technique_common>\n";
	out << tabs << "\t\t<accessor count=\"" << count << "\" source=\"#" << id << "-array\" stride=\"" << numValues << "\">\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"R\" />\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"G\" />\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"B\" />\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"A\" />\n";
	out << tabs << "\t\t</accessor>\n";
	out << tabs << "\t</technique_common>\n";
	out << tabs << "</source>\n";
}

/**
 * Write a <source> tag for the given @a index of the @a mesh to the output stream.
 *
 * @param mesh Data source
 * @param attrNameId Attr type to take from the mesh
 * @param id Identifier for the XML tags
 * @param out Output stream
 * @param indention Indent the lines by this number of tabs
 */
static void outputMeshNormalSource(Mesh * mesh, Util::StringIdentifier attrNameId,
								 const std::string & id,
								 std::ostream & out,
								 uint_fast8_t indention) {
	Util::Reference<NormalAttributeAccessor> accessor = NormalAttributeAccessor::create(mesh->openVertexData(), attrNameId);

	const uint32_t count = mesh->getVertexCount();
	const unsigned int numValues = 3;

	std::string tabs(indention, '\t');
	out << tabs << "<source id=\"" << id << "\">\n";
	out << tabs << "\t<float_array count=\"" << count * numValues << "\" id=\"" << id << "-array\">";
	for (uint32_t v = 0; v < count; ++v) {
		if (v != 0) {
			out << ' ';
		}
		out << accessor->getNormal(v);
	}
	out << "</float_array>\n";
	out << tabs << "\t<technique_common>\n";
	out << tabs << "\t\t<accessor count=\"" << count << "\" source=\"#" << id << "-array\" stride=\"" << numValues << "\">\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"X\" />\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"Y\" />\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"Z\" />\n";
	out << tabs << "\t\t</accessor>\n";
	out << tabs << "\t</technique_common>\n";
	out << tabs << "</source>\n";
}

/**
 * Write a <source> tag for the given @a index of the @a mesh to the output stream.
 *
 * @param mesh Data source
 * @param attrNameId Attr type to take from the mesh
 * @param id Identifier for the XML tags
 * @param out Output stream
 * @param indention Indent the lines by this number of tabs
 */
static void outputMeshPositionSource(Mesh * mesh, Util::StringIdentifier attrNameId,
								 const std::string & id,
								 std::ostream & out,
								 uint_fast8_t indention) {
	Util::Reference<PositionAttributeAccessor> accessor = PositionAttributeAccessor::create(mesh->openVertexData(), attrNameId);

	const uint32_t count = mesh->getVertexCount();
	const unsigned int numValues = 3;

	std::string tabs(indention, '\t');
	out << tabs << "<source id=\"" << id << "\">\n";
	out << tabs << "\t<float_array count=\"" << count * numValues << "\" id=\"" << id << "-array\">";
	for (uint32_t v = 0; v < count; ++v) {
		if (v != 0) {
			out << ' ';
		}
		out << accessor->getPosition(v);
	}
	out << "</float_array>\n";
	out << tabs << "\t<technique_common>\n";
	out << tabs << "\t\t<accessor count=\"" << count << "\" source=\"#" << id << "-array\" stride=\"" << numValues << "\">\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"X\" />\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"Y\" />\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"Z\" />\n";
	out << tabs << "\t\t</accessor>\n";
	out << tabs << "\t</technique_common>\n";
	out << tabs << "</source>\n";
}

/**
 * Write a <source> tag for the given @a index of the @a mesh to the output stream.
 *
 * @param mesh Data source
 * @param attrNameId Attr type to take from the mesh
 * @param id Identifier for the XML tags
 * @param out Output stream
 * @param indention Indent the lines by this number of tabs
 */
static void outputMeshTexCoordSource(Mesh * mesh, Util::StringIdentifier attrNameId,
								 const std::string & id,
								 std::ostream & out,
								 uint_fast8_t indention) {
	Util::Reference<TexCoordAttributeAccessor> accessor = TexCoordAttributeAccessor::create(mesh->openVertexData(), attrNameId);

	const uint32_t count = mesh->getVertexCount();
	const unsigned int numValues = 2;

	std::string tabs(indention, '\t');
	out << tabs << "<source id=\"" << id << "\">\n";
	out << tabs << "\t<float_array count=\"" << count * numValues << "\" id=\"" << id << "-array\">";
	for (uint32_t v = 0; v < count; ++v) {
		if (v != 0) {
			out << ' ';
		}
		out << accessor->getCoordinate(v);
	}
	out << "</float_array>\n";
	out << tabs << "\t<technique_common>\n";
	out << tabs << "\t\t<accessor count=\"" << count << "\" source=\"#" << id << "-array\" stride=\"" << numValues << "\">\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"U\" />\n";
	out << tabs << "\t\t\t<param type=\"float\" name=\"V\" />\n";
	out << tabs << "\t\t</accessor>\n";
	out << tabs << "\t</technique_common>\n";
	out << tabs << "</source>\n";
}

bool WriterDAE::saveFile(const FileName & fileName, Node * scene) {
	time_t rawTime;
	time(&rawTime);
	tm utcTime;
#if defined(_WIN32)
	utcTime = *gmtime(&rawTime);
#else
	gmtime_r(&rawTime, &utcTime);
#endif
	char buffer[256];
	strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utcTime);

	auto outPtr = FileUtils::openForWriting(fileName);
	if(!outPtr) {
		WARN("");
		return false;
	}
	std::ostream & out=*outPtr;
	out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
	out << "<COLLADA version=\"1.4.0\" "
		<< "xmlns=\"http://www.collada.org/2005/11/COLLADASchema\">\n";
	out << "\t<asset>\n";
	out << "\t\t<contributor>\n";
	out << "\t\t\t<author>MinSG build " << __DATE__ << " " << __TIME__
		<< "</author>\n";
	out << "\t\t\t<authoring_tool>MinSG build " << __DATE__ << " "
		<< __TIME__ << "</authoring_tool>\n";
	out << "\t\t\t<comments></comments>\n";
	out << "\t\t\t<copyright></copyright>\n";
	out << "\t\t\t<source_data>file://</source_data>\n";
	out << "\t\t</contributor>\n";
	out << "\t\t<created>" << buffer << "</created>\n";
	out << "\t\t<modified>" << buffer << "</modified>\n";
	out << "\t\t<unit meter=\"0.01\" name=\"centimeter\"/>\n";
	out << "\t\t<up_axis>Y_UP</up_axis>\n";
	out << "\t</asset>\n";

	struct Visitor : public NodeVisitor {
		Visitor(std::ostringstream & output) :
			out(output), indention(3) {

		}
		NodeVisitor::status enter(Node * node) override {
			out << std::string(indention, '\t');
			out << "<node>\n";
			++indention;
			Geometry::SRT srt = node->getRelTransformationSRT();
			const Geometry::Vec3f & translate = srt.getTranslation();
			if (!translate.isZero()) {
				out << std::string(indention, '\t');
				out << "<translate sid=\"translate\">"
					<< translate.x() << " " << translate.y()
					<< " " << translate.z() << "</translate>\n";
			}
			const Geometry::Matrix3x3f & rotate = srt.getRotation();
			Geometry::Vec3f axis;
			float angle;
			rotate.getRotation_deg(axis, angle);
			if (angle != 0.0f) {
				out << std::string(indention, '\t');
				out << "<rotate sid=\"rotate\">" << axis.x() << " "
					<< axis.y() << " " << axis.z() << " "
					<< angle << "</rotate>\n";
			}
			const float scale = srt.getScale();
			if (scale != 1.0f) {
				out << std::string(indention, '\t');
				out << "<scale sid=\"scale\">" << scale << " "
					<< scale << " " << scale << "</scale>\n";
			}
			return CONTINUE_TRAVERSAL;
		}

		NodeVisitor::status leave(Node * node) override {
			std::string id;
			std::string type;
			id = getOrGenerateId(
					 dynamic_cast<AbstractCameraNode *> (node),
					 cameraMapping);
			if (!id.empty()) {
				type = "camera";
			} else {
				id = getOrGenerateId(
						 dynamic_cast<LightNode *> (node),
						 lightMapping);
				if (!id.empty()) {
					type = "light";
				} else {
					GeometryNode * geoNode =
						dynamic_cast<GeometryNode *> (node);
					if (geoNode != nullptr) {
						id = getOrGenerateId(geoNode->getMesh(),
											 meshMapping);
						if (!id.empty()) {
							type = "geometry";
						}
					}
				}
			}
			if (!type.empty()) {
				std::string textureId;
				if (type == "geometry") {
					const Node::stateList_t * states = node->getStateListPtr();
					if (states != nullptr) {
						for (const auto & stateEntry : *states) {
							textureId = getOrGenerateId(dynamic_cast<TextureState *>(stateEntry.first.get()), textureMapping);
							if (!textureId.empty()) {
								break;
							}
						}
					}
				}
				if (textureId.empty()) {
					out << std::string(indention, '\t');
					out << "<instance_" << type << " url=\"#" << id
						<< "\"/>\n";
				} else {
					out << std::string(indention, '\t');
					out << "<instance_" << type << " url=\"#" << id
						<< "\">\n";
					out << std::string(indention + 1, '\t');
					out << "<bind_material>\n";
					out << std::string(indention + 2, '\t');
					out << "<technique_common>\n";
					out << std::string(indention + 3, '\t');
					out << "<instance_material symbol=\""
						<< textureId << "\" target=\"#"
						<< textureId << "\">\n";
					out << std::string(indention + 4, '\t');
					out << "<bind_vertex_input "
						<< "input_semantic=\"TEXCOORD\" "
						<< "input_set=\"1\" "
						<< "semantic=\"CHANNEL1\"/>\n";
					out << std::string(indention + 3, '\t');
					out << "</instance_material>\n";
					out << std::string(indention + 2, '\t');
					out << "</technique_common>\n";
					out << std::string(indention + 1, '\t');
					out << "</bind_material>\n";
					out << std::string(indention, '\t');
					out << "</instance_" << type << ">\n";
				}
			}
			--indention;
			out << std::string(indention, '\t');
			out << "</node>\n";
			return CONTINUE_TRAVERSAL;
		}

		std::map<AbstractCameraNode *, std::string> cameraMapping;
		std::map<LightNode *, std::string> lightMapping;
		std::map<Mesh *, std::string> meshMapping;
		std::map<TextureState *, std::string> textureMapping;

		//! Stream used to write the output to.
		std::ostringstream & out;

		//! Number of tab characters for formatting purposes.
		uint_fast8_t indention;
	};

	std::ostringstream sceneOutput;
	Visitor vis(sceneOutput);
	scene->traverse(vis);

	out << "\t<library_cameras>\n";
	for (std::map<AbstractCameraNode *, std::string>::const_iterator
			it = vis.cameraMapping.begin(); it
			!= vis.cameraMapping.end(); ++it) {
		out << "\t\t<camera id=\"" << it->second << "\" name=\""
			<< it->second << "\">\n";
		out << "\t\t\t<optics>\n";
		out << "\t\t\t\t<technique_common>\n";
		CameraNode * perspective =
			dynamic_cast<CameraNode *> (it->first);
		if (perspective != nullptr) {
			out << "\t\t\t\t\t<perspective>\n";
			float fovLeft, fovRight, fovBottom, fovTop;
			perspective->getAngles(fovLeft, fovRight, fovBottom, fovTop);
			out << "\t\t\t\t\t\t<xfov>" << Util::StringUtils::toString(
					std::abs(fovLeft) + std::abs(fovRight))
				<< "</xfov>\n";
			out << "\t\t\t\t\t\t<yfov>" << Util::StringUtils::toString(
					std::abs(fovTop) + std::abs(fovBottom))
				<< "</yfov>\n";
			out << "\t\t\t\t\t\t<znear>"
				<< Util::StringUtils::toString(
					perspective->getNearPlane())
				<< "</znear>\n";
			out << "\t\t\t\t\t\t<zfar>" << Util::StringUtils::toString(
					perspective->getFarPlane()) << "</zfar>\n";
			out << "\t\t\t\t\t</perspective>\n";
		} else {
			CameraNodeOrtho * ortho =
				dynamic_cast<CameraNodeOrtho *> (it->first);
			if (ortho != nullptr) {
				out << "\t\t\t\t\t<orthographic>\n";
				out << "\t\t\t\t\t\t<xmag>"
					<< Util::StringUtils::toString(
						ortho->getWidth()) << "</xmag>\n";
				out << "\t\t\t\t\t\t<ymag>"
					<< Util::StringUtils::toString(
						ortho->getHeight()) << "</ymag>\n";
				out << "\t\t\t\t\t\t<znear>"
					<< Util::StringUtils::toString(
						ortho->getNearPlane())
					<< "</znear>\n";
				out << "\t\t\t\t\t\t<zfar>"
					<< Util::StringUtils::toString(
						ortho->getFarPlane())
					<< "</zfar>\n";
				out << "\t\t\t\t\t</orthographic>\n";
			} else {
				WARN("Unknown camera node.");
			}
		}
		out << "\t\t\t\t</technique_common>\n";
		out << "\t\t\t</optics>\n";
		out << "\t\t</camera>\n";
	}
	out << "\t</library_cameras>\n";

	out << "\t<library_lights>\n";
	for (std::map<LightNode *, std::string>::const_iterator it =
				vis.lightMapping.begin(); it != vis.lightMapping.end(); ++it) {
		out << "\t\t<light id=\"" << it->second << "\" name=\""
			<< it->second << "\">\n";
		out << "\t\t\t<technique_common>\n";
		const Util::Color4f & color = it->first->getDiffuseLightColor();
		std::string colorOut("\t\t\t\t\t<color>"
							 + Util::StringUtils::toString(color.getR()) + ' '
							 + Util::StringUtils::toString(color.getG()) + ' '
							 + Util::StringUtils::toString(color.getB()) + "</color>\n");
		LightNode * light = it->first;
		if (light->getType() == LightParameters::DIRECTIONAL) {
			out << "\t\t\t\t<directional>\n";
			out << colorOut;
			out << "\t\t\t\t</directional>\n";
		} else {
			const float constant = light->getConstantAttenuation();
			const float linear = light->getLinearAttenuation();
			const float quadratic =	light->getQuadraticAttenuation();
			std::string attenuationOut;
			attenuationOut += "\t\t\t\t\t<constant_attenuation>"
							  + Util::StringUtils::toString(constant)
							  + "</constant_attenuation>\n";
			attenuationOut += "\t\t\t\t\t<linear_attenuation>"
							  + Util::StringUtils::toString(linear)
							  + "</linear_attenuation>\n";
			attenuationOut += "\t\t\t\t\t<quadratic_attenuation>"
							  + Util::StringUtils::toString(quadratic)
							  + "</quadratic_attenuation>\n";
			if (light->getType() == LightParameters::POINT) {
				out << "\t\t\t\t<point>\n";
				out << colorOut;
				out << attenuationOut;
				out << "\t\t\t\t</point>\n";
			} else {
				if (light->getType() == LightParameters::SPOT) {
					out << "\t\t\t\t<spot>\n";
					out << colorOut;
					out << attenuationOut;
					out << "\t\t\t\t\t<falloff_angle>"
						<< Util::StringUtils::toString(
							light->getCutoff())
						<< "</falloff_angle>\n";
					out << "\t\t\t\t\t<falloff_exponent>"
						<< Util::StringUtils::toString(
							light->getExponent())
						<< "</falloff_exponent>\n";
					out << "\t\t\t\t</spot>\n";
				} else {
					WARN("Unknown light node.");
				}
			}
		}
		out << "\t\t\t</technique_common>\n";
		out << "\t\t</light>\n";
	}
	out << "\t</library_lights>\n";

	out << "\t<library_effects>\n";
	for (std::map<TextureState *, std::string>::const_iterator it =
				vis.textureMapping.begin(); it != vis.textureMapping.end(); ++it) {
		out << "\t\t<effect id=\"" << it->second << "-fx\" name=\""
			<< it->second << "-fx\">\n";
		out << "\t\t\t<profile_COMMON>\n";

		out << "\t\t\t\t<newparam sid=\"" << it->second
			<< "-surface\">\n";
		out << "\t\t\t\t\t<surface type=\"2D\">\n";
		out << "\t\t\t\t\t\t<init_from>" << it->second
			<< "-img</init_from>\n";
		out << "\t\t\t\t\t</surface>\n";
		out << "\t\t\t\t</newparam>\n";

		out << "\t\t\t\t<newparam sid=\"" << it->second
			<< "-sampler\">\n";
		out << "\t\t\t\t\t<sampler2D>\n";
		out << "\t\t\t\t\t\t<source>" << it->second
			<< "-surface</source>\n";
		out << "\t\t\t\t\t</sampler2D>\n";
		out << "\t\t\t\t</newparam>\n";

		out << "\t\t\t\t<technique sid=\"default\">\n";
		out << "\t\t\t\t\t<phong>\n";
		out << "\t\t\t\t\t\t<diffuse>\n";
		out << "\t\t\t\t\t\t\t<texture texcoord=\"CHANNEL1\" "
			<< "texture=\"" << it->second << "-sampler\"/>\n";
		out << "\t\t\t\t\t\t</diffuse>\n";
		out << "\t\t\t\t\t</phong>\n";
		out << "\t\t\t\t</technique>\n";

		out << "\t\t\t</profile_COMMON>\n";
		out << "\t\t</effect>\n";
	}
	out << "\t</library_effects>\n";

	out << "\t<library_images>\n";
	for (std::map<TextureState *, std::string>::const_iterator it =
				vis.textureMapping.begin(); it != vis.textureMapping.end(); ++it) {
		out << "\t\t<image id=\"" << it->second << "-img\" name=\""
			<< it->second << "-img\">\n";
		if(it->first->getTexture())
		{
			out << "\t\t\t<init_from>";
			it->first->getTexture()->getFileName();
			out << "</init_from>\n";
		}
		else{
			WARN("found texturestate without texture");
		}
		out << "\t\t</image>\n";
	}
	out << "\t</library_images>\n";

	out << "\t<library_materials>\n";
	for (std::map<TextureState *, std::string>::const_iterator it =
				vis.textureMapping.begin(); it != vis.textureMapping.end(); ++it) {
		out << "\t\t<material id=\"" << it->second << "\" name=\""
			<< it->second << "\">\n";
		out << "\t\t\t<instance_effect url=\"#" << it->second
			<< "-fx\" />\n";
		out << "\t\t</material>\n";
	}
	out << "\t</library_materials>\n";

	out << "\t<library_geometries>\n";
	for (std::map<Mesh *, std::string>::const_iterator it =
				vis.meshMapping.begin(); it != vis.meshMapping.end(); ++it) {
		out << "\t\t<geometry id=\"" << it->second << "\" name=\""
			<< it->second << "\">\n";
		Mesh * mesh = it->first;
		if (mesh != nullptr) {
			const VertexDescription & desc =
				mesh->getVertexDescription();
			out << "\t\t\t<mesh>\n";

			outputMeshPositionSource(mesh, VertexAttributeIds::POSITION, it->second + "-Position", out, 4);
			if(desc.hasAttribute(VertexAttributeIds::NORMAL)) {
				outputMeshNormalSource(mesh, VertexAttributeIds::NORMAL, it->second + "-Normal", out, 4);
			}
			if(desc.hasAttribute(VertexAttributeIds::TEXCOORD0)) {
				outputMeshTexCoordSource(mesh, VertexAttributeIds::TEXCOORD0, it->second + "-Texcoord", out, 4);
			}
			if(desc.hasAttribute(VertexAttributeIds::COLOR)) {
				outputMeshColorSource(mesh, VertexAttributeIds::COLOR, it->second + "-Color", out, 4);
			}

			out << "\t\t\t\t<vertices id=\"" << it->second
				<< "-Vertex\">\n";
			out << "\t\t\t\t\t<input semantic=\"POSITION\" source=\"#"
				<< it->second << "-Position\" />\n";
			out << "\t\t\t\t</vertices>\n";

			const uint32_t indexCount = mesh->getIndexCount();
			if(mesh->getDrawMode() == Rendering::Mesh::DRAW_TRIANGLES) {
				out << "\t\t\t\t<triangles count=\"" << (indexCount / 3) << "\">\n";
				out << "\t\t\t\t\t<input offset=\"0\" "
					<< "semantic=\"VERTEX\" source=\"#"
					<< it->second << "-Vertex\" />\n";
				if (!desc.getAttribute(VertexAttributeIds::NORMAL).empty()) {
					out << "\t\t\t\t\t<input offset=\"0\" "
						<< "semantic=\"NORMAL\" source=\"#"
						<< it->second << "-Normal\" />\n";
				}
				if (!desc.getAttribute(VertexAttributeIds::TEXCOORD0).empty()) {
					out << "\t\t\t\t\t<input offset=\"0\" "
						<< "semantic=\"TEXCOORD\" source=\"#"
						<< it->second << "-Texcoord\" />\n";
				}
				if (!desc.getAttribute(VertexAttributeIds::COLOR).empty()) {
					out << "\t\t\t\t\t<input offset=\"0\" "
						<< "semantic=\"COLOR\" source=\"#"
						<< it->second << "-Color\" />\n";
				}
				out << "\t\t\t\t\t<p>";
				{
					MeshIndexData & id = mesh->openIndexData();
					for (uint32_t i = 0; i < indexCount; ++i) {
						if (i > 0) {
							out << ' ';
						}
						out << id[i];
					}
				}
				out << "</p>\n";
				out << "\t\t\t\t</triangles>\n";
			} else {
				WARN("Unsupported triangle mode.");
			}

			out << "\t\t\t</mesh>\n";
		} else {
			WARN("GeometryNode does not have a mesh.");
		}
		out << "\t\t</geometry>\n";
	}
	out << "\t</library_geometries>\n";

	out << "\t<library_visual_scenes>\n";
	out << "\t\t<visual_scene id=\"Scene\" name=\"Scene\">\n";
	out << sceneOutput.str();
	out << "\t\t</visual_scene>\n";
	out << "\t</library_visual_scenes>\n";

	out << "\t<scene>\n";
	out << "\t\t<instance_visual_scene url=\"#Scene\"/>\n";
	out << "\t</scene>\n";
	out << "</COLLADA>";

	return true;
}

template<typename _T>
std::string WriterDAE::getOrGenerateId(_T * object, std::map<_T *,
									   std::string> & mapping) {
	if (object == nullptr) {
		return "";
	}
	typename std::map<_T *, std::string>::const_iterator it =
		mapping.find(object);
	if (it == mapping.end()) {
		// Generate a new id.
		std::string id(object->getTypeName());
		id += '_';
		id += Util::StringUtils::toString(mapping.size());
		mapping.insert(std::make_pair(object, id));
		return id;
	} else {
		// Return stored id.
		return it->second;
	}
}

}
}
