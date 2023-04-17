/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "base/plugins.h"

#include "engines/advancedDetector.h"

static const PlainGameDescriptor cruxGames[] = {
	{"and", "Armed & Delirious"},
	{0, 0}
};

namespace Crux {

static const ADGameDescription gameDescriptions[] = {

	{
		"and",
		0,
		AD_ENTRY1s("ADVENT.IDX", "d673ea47ebbed39fec338363f87a12f9", 140819),
		Common::EN_ANY,
		Common::kPlatformWindows,
		0, // ADGF_UNSTABLE,
		GUIO1(GUIO_NOMIDI)
	},

	AD_TABLE_END_MARKER
};

} // End of namespace Crux

class CruxMetaEngineDetection : public AdvancedMetaEngineDetection {
public:
	CruxMetaEngineDetection() : AdvancedMetaEngineDetection(Crux::gameDescriptions, sizeof(ADGameDescription), cruxGames) {
	}

	const char *getName() const override {
		return "crux";
	}

	const char *getEngineName() const override {
		return "Crux";
	}

	const char *getOriginalCopyright() const override {
		return "Makh-Shevet Ltd.";
	}
};

REGISTER_PLUGIN_STATIC(CRUX_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, CruxMetaEngineDetection);

