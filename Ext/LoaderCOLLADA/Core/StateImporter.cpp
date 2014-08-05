/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "StateImporter.h"
#include "../Utils/DescriptionUtils.h"
#include "../Utils/LoaderCOLLADAConsts.h"

#include <Util/Macros.h>
COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wignored-qualifiers)
COMPILER_WARN_OFF_GCC(-Wsign-promo)
COMPILER_WARN_OFF_GCC(-Wzero-as-null-pointer-constant)
COMPILER_WARN_OFF_GCC(-Wshadow)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
COMPILER_WARN_OFF_CLANG(-W#warnings)
#include <COLLADAFWColorOrTexture.h>
#include <COLLADAFWEffect.h>
#include <COLLADAFWFloatOrParam.h>
#include <COLLADAFWImage.h>
#include <COLLADAFWMaterial.h>
#include <COLLADABUURI.h>
COMPILER_WARN_POP

#include "../../../SceneManagement/SceneDescription.h"

namespace MinSG {
namespace LoaderCOLLADA {

bool materialCoreImporter(const COLLADAFW::Material * material, referenceRegistry_t & referenceRegistry) {
	auto materialDesc = new SceneManagement::DescriptionMap;

	materialDesc->setValue(Consts::DAE_REFERENCE, new daeReferenceData(material->getInstantiatedEffect()));

	referenceRegistry[material->getUniqueId()] = materialDesc;
	return true;
}

bool imageCoreImporter(const COLLADAFW::Image * image, referenceRegistry_t & referenceRegistry) {
	auto textureDesc = new SceneManagement::DescriptionMap;

	textureDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_STATE);
	textureDesc->setString(SceneManagement::Consts::ATTR_STATE_TYPE, SceneManagement::Consts::STATE_TYPE_TEXTURE);

	auto dataDesc = new SceneManagement::DescriptionMap;
	dataDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_DATA);
	dataDesc->setString(SceneManagement::Consts::ATTR_DATA_TYPE, "image");
	dataDesc->setString(SceneManagement::Consts::ATTR_TEXTURE_FILENAME, image->getImageURI().originalStr());

	addToMinSGChildren(textureDesc, dataDesc);
	referenceRegistry[image->getUniqueId()] = textureDesc;

	return true;
}

bool effectCoreImporter(const COLLADAFW::Effect * effect, referenceRegistry_t & referenceRegistry, bool invertTransparency) {
	auto desc = new SceneManagement::DescriptionMap;

	desc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_STATE);
	desc->setString(SceneManagement::Consts::ATTR_STATE_TYPE, SceneManagement::Consts::STATE_TYPE_MATERIAL);

	auto & commonEffects = effect->getCommonEffects();
	for(size_t i = 0; i < commonEffects.getCount(); ++i) {
		COLLADAFW::EffectCommon * commonEffect = commonEffects[i];

		if(commonEffect->getAmbient().isColor()) {
			const auto & color = commonEffect->getAmbient().getColor();

			std::ostringstream valueStream;
			valueStream << color.getRed() << " " << color.getGreen() << " " << color.getBlue() << " " << color.getAlpha();

			desc->setString(SceneManagement::Consts::ATTR_MATERIAL_AMBIENT, valueStream.str());
		}
		if(commonEffect->getDiffuse().isColor()) {
			const auto & color = commonEffect->getDiffuse().getColor();

			std::ostringstream valueStream;
			valueStream << color.getRed() << " " << color.getGreen() << " " << color.getBlue() << " " << color.getAlpha();

			desc->setString(SceneManagement::Consts::ATTR_MATERIAL_DIFFUSE, valueStream.str());
		}
		if(commonEffect->getSpecular().isColor()) {
			const auto & color = commonEffect->getSpecular().getColor();

			std::ostringstream valueStream;
			valueStream << color.getRed() << " " << color.getGreen() << " " << color.getBlue() << " " << color.getAlpha();

			desc->setString(SceneManagement::Consts::ATTR_MATERIAL_SPECULAR, valueStream.str());
		}
		if(commonEffect->getShininess().getType() == COLLADAFW::FloatOrParam::FLOAT) {
			const auto shininess = std::max(0.0f, std::min(commonEffect->getShininess().getFloatValue(), 128.0f));

			std::ostringstream valueStream;
			valueStream << shininess;

			desc->setString(SceneManagement::Consts::ATTR_MATERIAL_SHININESS, valueStream.str());
		}
		if(commonEffect->getOpacity().getColor().getAlpha() < 1.0) {
			auto transDesc = new SceneManagement::DescriptionMap;
			transDesc->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_STATE);
			transDesc->setString(SceneManagement::Consts::ATTR_STATE_TYPE, SceneManagement::Consts::STATE_TYPE_BLENDING);
			float alpha = commonEffect->getOpacity().getColor().getAlpha();
			if(invertTransparency) {
				alpha = 1.0f - alpha;
			}
			transDesc->setString(SceneManagement::Consts::ATTR_BLEND_CONST_ALPHA, Util::StringUtils::toString(alpha));

			addToParentAsChildren(desc, transDesc, Consts::EFFECT_LIST);
			addDAEFlag(referenceRegistry, Consts::DAE_FLAG_USE_TRANSPARENCY_RENDERER, new Util::BoolAttribute(true));
		}

		const auto & samplers = commonEffect->getSamplerPointerArray();
		for(size_t j = 0; j < samplers.getCount(); ++j) {
			auto imageDesc = new SceneManagement::DescriptionMap;
			imageDesc->setString(SceneManagement::Consts::ATTR_STATE_TYPE, SceneManagement::Consts::STATE_TYPE_TEXTURE);
			imageDesc->setValue(Consts::DAE_REFERENCE, new daeReferenceData(samplers[j]->getSourceImage()));
			imageDesc->setString(SceneManagement::Consts::ATTR_TEXTURE_UNIT, Util::StringUtils::toString(j));
			addToParentAsChildren(desc, imageDesc, Consts::EFFECT_LIST);
		}
	}

	referenceRegistry[effect->getUniqueId()] = desc;

	return true;
}

}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
