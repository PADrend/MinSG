/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "AbstractSurfelStrategy.h"

#include <Util/Macros.h>

#include "../../../SceneManagement/Exporter/ExporterTools.h"
#include "../../../SceneManagement/Importer/ImporterTools.h"
#include "../../../SceneManagement/SceneDescription.h"

#include <unordered_map>

namespace MinSG {
namespace BlueSurfels {
using namespace MinSG;
using namespace MinSG::SceneManagement;

typedef std::unordered_map<Util::StringIdentifier, SurfelStrategyExporter_t> ExporterMap_t;
typedef std::unordered_map<Util::StringIdentifier, SurfelStrategyImporter_t> ImporterMap_t;

const std::string TYPE_STRATEGY("strategy");
const Util::StringIdentifier ATTR_STRATEGY_TYPE("type");
static const Util::StringIdentifier ATTR_ENABLED("enabled");

static ExporterMap_t& getStrategyExporter() {
	static ExporterMap_t strategyExporter;
	return strategyExporter;
}

static ImporterMap_t& getStrategyImporter() {
	static ImporterMap_t strategyImporter;
	return strategyImporter;
}

bool registerExporter(const Util::StringIdentifier& type, const SurfelStrategyExporter_t& exporter) {
	getStrategyExporter().emplace(type, exporter);
	return true;
}

bool registerImporter(const Util::StringIdentifier& type, const SurfelStrategyImporter_t& importer) {
	getStrategyImporter().emplace(type, importer);
	return true;
}

std::unique_ptr<Util::GenericAttributeMap> exportStrategy(AbstractSurfelStrategy* strategy) {	
	const auto handler = getStrategyExporter().find(strategy->getTypeId());
	if(handler == getStrategyExporter().end()) {
		WARN(std::string("Unsupported Surfel Strategy ") + strategy->getTypeName());
		return nullptr;
	}
	std::unique_ptr<Util::GenericAttributeMap> description(new Util::GenericAttributeMap);
	description->setString(Consts::TYPE, TYPE_STRATEGY);
	description->setString(ATTR_STRATEGY_TYPE, strategy->getTypeName());	
	description->setValue(ATTR_ENABLED, Util::GenericAttribute::createBool(strategy->isEnabled()));
	handler->second(*description, strategy);
	return description;
}

AbstractSurfelStrategy* importStrategy(const Util::GenericAttributeMap* desc) {
	const auto& type = desc->getString(ATTR_STRATEGY_TYPE);
	const auto handler = getStrategyImporter().find(type);
	if(handler == getStrategyImporter().end()) {
		WARN(std::string("Unsupported Surfel Strategy ") + type);
		return nullptr;
	}
	auto strategy = handler->second(desc);
	if(strategy) {
		strategy->setEnabled(desc->getBool(ATTR_ENABLED, true));
	}
	return strategy;
}
	
} /* BlueSurfels */
} /* MinSG */
#endif // MINSG_EXT_BLUE_SURFELS