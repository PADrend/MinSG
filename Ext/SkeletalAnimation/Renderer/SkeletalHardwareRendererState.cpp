/*
	This file is part of the MinSG library extension SkeletalAnimation.
	Copyright (C) 2011-2012 Lukas Kopecki
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_SKELETAL_ANIMATION

#include "SkeletalHardwareRendererState.h"
#include "../Util/SkeletalAnimationUtils.h"

#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>

#include "../../../Core/FrameContext.h"
#include "../../../Core/Nodes/GeometryNode.h"
#include "../../../Helper/DataDirectory.h"

#include "../Joints/JointNode.h"
#include "../SkeletalNode.h"

#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Mesh/Mesh.h>

#include <Rendering/Helper.h>

using namespace Rendering;
using namespace std;
using namespace Geometry;
using namespace Util;

namespace MinSG {
    
SkeletalHardwareRendererState *SkeletalHardwareRendererState::clone() const
{
	return new SkeletalHardwareRendererState(*this);
}
    
SkeletalHardwareRendererState::SkeletalHardwareRendererState(const SkeletalHardwareRendererState &source) : SkeletalAbstractRendererState(source),
                                                                                                            shaderType(source.getUsingShaderType()), textureUnitSet(false) {
    init(source.getUsingShaderType());
}

SkeletalHardwareRendererState::SkeletalHardwareRendererState() : SkeletalAbstractRendererState(), textureUnitSet(false)
{
	init(UNIFORM);
}
    
SkeletalHardwareRendererState::SkeletalHardwareRendererState(const uint32_t forceShaderType) : SkeletalAbstractRendererState() {
    init(forceShaderType);
}
    
bool SkeletalHardwareRendererState::switchToHandlingType(const uint32_t _shaderType) {
    if(shaderType == _shaderType)
        return true;
    
    if(_shaderType == TEXTURE)
        return textureTransformation();
    
    if(_shaderType == UNIFORM)
        return uniformTransformation();
    
    if(_shaderType == NOTSUPPORTED)
        return noTransformation();
        
    return false;
}

void SkeletalHardwareRendererState::init(uint32_t forceShaderType)
{
    validatedMatrices = false;
    
    if(shader.isNull()) {
        bool shaderCompiled = false;
        if(forceShaderType == TEXTURE)
            shaderCompiled = textureTransformation();
        
        if(forceShaderType == UNIFORM || !shaderCompiled)
            shaderCompiled = uniformTransformation();
        
        if(!shaderCompiled)
            if(!noTransformation())
                shaderType = NOSHADER;
    }
    
    debugJointId = -1;
    setUniform(Uniform("debugJointId", debugJointId));
    
    setBindMatrix(Matrix4x4());
}
    
bool SkeletalHardwareRendererState::noTransformation()
{
    WARN("Skeletal Animation not supported on this Graphic Card.");
    shaderType = NOTSUPPORTED;
    
    shader = Shader::createShader();

    shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/SkeletalAnimation/skeletalAnimationTransformation.vs")));
    shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/SkeletalAnimation/noTransformation.vs")));
    
    attachUniversalShaderFiles(shader);
    
    shader->init();
    
    if(shader->getStatus() != Shader::LINKED)
        return false;
    
    return true;
}

bool SkeletalHardwareRendererState::textureTransformation()
{
    shader = Shader::createShader();
    shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/SkeletalAnimation/skeletalAnimationTransformation.vs")));
    
    std::string transformationCode = Util::FileUtils::getParsedFileContents(Util::FileName(DataDirectory::getPath() + "/shader/universal2/SkeletalAnimation/textureTransformation.vs"));
    
    string shadingVersion = string(getShadingLanguageVersion());
    if(shadingVersion.find("1.20") != std::string::npos || shadingVersion.find("1.10") != std::string::npos)
    {
        if(isExtensionSupported("GL_EXT_gpu_shader4"))
            transformationCode = "#extension GL_EXT_gpu_shader4 : enable\n\n" + transformationCode;
        else
            return false;
    }
    
    shader->attachShaderObject(ShaderObjectInfo::createVertex(transformationCode));
    
    attachUniversalShaderFiles(shader);
    
    shader->init();
    
    if(shader->getStatus() == Shader::LINKED)
        shaderType = TEXTURE;
    else
        return false;
    
    return true;
}

bool SkeletalHardwareRendererState::uniformTransformation()
{
    shaderType = UNIFORM;
    
    shader = Shader::createShader();
    
    shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/SkeletalAnimation/skeletalAnimationTransformation.vs")));
    shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/SkeletalAnimation/uniformTransformation.vs")));

    attachUniversalShaderFiles(shader);
    
    shader->init();
    
    if(shader->getStatus() == Shader::LINKED)
        shaderType = UNIFORM;
    else
        return false;
    
    return true;
}
    
void SkeletalHardwareRendererState::attachUniversalShaderFiles(Util::Reference<Rendering::Shader> _shader) {
    _shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/universal.vs")));
    _shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/texture.sfn")));
    _shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/sgHelpers.sfn")));
    _shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/shading_phong.sfn")));
    _shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/shadow_disabled.sfn")));
    _shader->attachShaderObject(ShaderObjectInfo::loadVertex(Util::FileName(DataDirectory::getPath() + "/shader/universal2/color_standard.sfn")));
    
    _shader->attachShaderObject(ShaderObjectInfo::loadFragment(Util::FileName(DataDirectory::getPath() + "/shader/universal2/universal.fs")));
    _shader->attachShaderObject(ShaderObjectInfo::loadFragment(Util::FileName(DataDirectory::getPath() + "/shader/universal2/texture.sfn")));
    _shader->attachShaderObject(ShaderObjectInfo::loadFragment(Util::FileName(DataDirectory::getPath() + "/shader/universal2/sgHelpers.sfn")));
    _shader->attachShaderObject(ShaderObjectInfo::loadFragment(Util::FileName(DataDirectory::getPath() + "/shader/universal2/shading_phong.sfn")));
    _shader->attachShaderObject(ShaderObjectInfo::loadFragment(Util::FileName(DataDirectory::getPath() + "/shader/universal2/shadow_disabled.sfn")));
    _shader->attachShaderObject(ShaderObjectInfo::loadFragment(Util::FileName(DataDirectory::getPath() + "/shader/universal2/color_standard.sfn")));
    _shader->attachShaderObject(ShaderObjectInfo::loadFragment(Util::FileName(DataDirectory::getPath() + "/shader/universal2/effect_highlight.sfn")));
}
    
void SkeletalHardwareRendererState::validateMatriceOrder(Node *node)
{
    if(shaderType == NOTSUPPORTED)
        return;
    
    SkeletalNode *root = dynamic_cast<SkeletalNode *>(node);
    if(root == nullptr)
    {
        shaderType = NOSHADER;
        return;
    }
    rootJoint = root;
    
    if(rootJoint->getJointMapSize() < 1)
    {
        noTransformation();
        return;    
    }
    
    SkeletalAbstractRendererState::validateMatriceOrder(node);
    
    if(shaderType == TEXTURE)
    {        
        texture = TextureUtils::createTextureDataArray_Vec4(9 + inverseMatContainer.size()*4 + jointMats.size()*4); 
        texture.get()->allocateLocalData();
        
        pa = PixelAccessor::create(texture.get()->getLocalBitmap());
    
        if(!SkeletalAnimationUtils::generateUniformTexture(bindMatrix, inverseMatContainer, jointMats, &pa))
            WARN("Could not generate joint texture.");
        SkeletalAnimationUtils::putMatrixInTexture(5, rootJoint->getWorldTransformationMatrix().inverse(), &pa);
    }else if(shaderType == UNIFORM)
    {
        if(rootJoint->getJointMapSize() > 8)
        {
            noTransformation();
            return;
        }else {            
            setUniform(Uniform("jointInv", inverseMatContainer));
            setUniform(Uniform("invWorldMatrix", rootJoint->getWorldTransformationMatrix().inverse()));
        }
    }
    
    validatedMatrices = true;
}
    
State::stateResult_t SkeletalHardwareRendererState::doEnableState(FrameContext & context,Node *node, const RenderParam &  rp )
{
    Rendering::Shader *currentShader = context.getRenderingContext().getActiveShader();
    if(currentShader != nullptr) {
        setUniform(context, currentShader->getUniform(Util::StringIdentifier("color")));
    }
    
    if(shaderType == NOSHADER)
        return State::STATE_SKIPPED;
    
    if(shaderType == NOTSUPPORTED)
        return ShaderState::doEnableState(context, node, rp);
    
    if(!validatedMatrices) {
        validateMatriceOrder(node);
        return ShaderState::doEnableState(context, node, rp);
    }
    
    jointMats.clear();
    generateJointGLArray(&jointMats);
    
    if(shaderType == TEXTURE)
    {
        SkeletalAnimationUtils::putMatrixInTexture(5, rootJoint->getWorldTransformationMatrix().inverse(), &pa);
        SkeletalAnimationUtils::putMatricesInTexture(9+inverseMatContainer.size()*4, jointMats, &pa);
        
        texture.get()->_uploadGLTexture(context.getRenderingContext());
        if(!textureUnitSet) {
            context.getRenderingContext().setTexture(7, texture.get());
            textureUnitSet = true;
        }
        
    }else if(shaderType == UNIFORM) {
        setUniform(context, Uniform("invWorldMatrix", rootJoint->getWorldTransformationMatrix().inverse()));
        setUniform(context, Uniform("joints", jointMats));
    }
    
    return ShaderState::doEnableState(context, node, rp);
}
    
void SkeletalHardwareRendererState::doDisableState(FrameContext & context,Node *node, const RenderParam & rp)
{    
    ShaderState::doDisableState(context, node, rp);
}

void SkeletalHardwareRendererState::generateJointGLArray(vector<Geometry::Matrix4x4> *container)
{        
    for(const auto item : matriceOrder)
        container->emplace_back(item->getWorldTransformationMatrix());
}
    
}

#endif
