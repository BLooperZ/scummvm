/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef CRUX_CRUX_H
#define CRUX_CRUX_H

#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug.h"
#include "common/debug-channels.h"
#include "common/error.h"
#include "common/array.h"
#include "common/random.h"
#include "engines/engine.h"
#include "gui/debugger.h"
#include "graphics/surface.h"
#include "graphics/screen.h"

struct ADGameDescription;

namespace Crux {

struct ResourceEntry {
    uint32 offset;
    uint32 length;
};

class ResourceId {
    public:
    	ResourceId(uint32 type, Common::String name): _type(type), _name(name) {
        }

        inline uint hash() const {
            return ((uint)((_type << 16) ^ _name.hash()));
	    }

        bool operator==(const ResourceId &other) const {
		return (_type == other._type) && (_name.equals(other._name));
        }

	bool operator!=(const ResourceId &other) const {
		return !operator==(other);
	}

    private:
        uint32 _type;
        Common::String _name;
};

struct ResourceIdHash : public Common::UnaryFunction<ResourceId, uint> {
	uint operator()(ResourceId val) const { return val.hash(); }
};

class Console;

class CruxEngine : public Engine {
public:
	CruxEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~CruxEngine() override;

	Common::Error run() override;
	bool hasFeature(EngineFeature f) const override;

	// Detection related functions
	const ADGameDescription *_gameDescription;
	const char *getGameId() const;
	Common::Platform getPlatform() const;
	bool isDemo() const;

	// We need random numbers
	Common::RandomSource *_rnd;

	Graphics::Surface _screen;
	void *_game;

	// View *_screenView;
	volatile int32 _timerTicks;

	bool _showHotspots;

	void pollEvents();

	void hideMouse();
	void showMouse();
	void getMousePosition(int16 *x, int16 *y);
	void setMousePosition(int16 x, int16 y);
	bool isMouseButtonDown();

    void playVideo(const char *name);
	void loadScript(const char *name);
    void decodePicture1(byte *buffer, uint32 decodePicture, Graphics::Surface surface);
    void decodePicture4(byte *buffer, uint32 decodePicture, Graphics::Surface surface);

private:
	int _mouseButton;
    Common::HashMap<ResourceId, ResourceEntry, ResourceIdHash> _resources;

	Common::Array<Common::String> readArrayOfStrings(Common::File &f);
	Common::String serializeStringArray(Common::Array<Common::String> &arr);
};

extern CruxEngine *g_ed;

} // End of namespace Crux

#endif
