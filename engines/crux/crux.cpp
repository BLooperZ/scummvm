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
#include <stdlib.h>

#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/debug.h"
#include "common/error.h"
#include "common/events.h"
#include "common/file.h"
#include "common/hashmap.h"
#include "common/scummsys.h"
#include "common/stream.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"
#include "graphics/screen.h"
#include "graphics/surface.h"

#include "crux/crux.h"

#include <common/std/set.h>
#include <image/png.h>

#define RESOURCE_TYPE_PALETTE 3
#define RESOURCE_TYPE_SCRIPT 4
#define RESOURCE_TYPE_BACKGROUND 6
#define RESOURCE_TYPE_CURSOR 7
#define RESOURCE_TYPE_VIDEO 0x10

namespace Crux {

CruxEngine *g_ed = nullptr;

CruxEngine::CruxEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst), _gameDescription(gameDesc) {
	_rnd = new Common::RandomSource("crux");

	_game = nullptr;
	// _screenView = nullptr;

	_showHotspots = false;
	_timerTicks = 0;

	_mouseButton = 0;

	g_ed = this;
	debug("crux::new()");
}

CruxEngine::~CruxEngine() {
	delete _rnd;
	// delete _game;
	// delete _screenView;
}

Common::Error CruxEngine::run() {
	// _game = new EdenGame(this);
	// _screenView = new View(640, 480);
	// setDebugger(new Debugger(this));
	debug("crux::run()");

	///// CLTimer
	_timerTicks = 0; // incremented in realtime

	// Initialize graphics using following:
	initGraphics(640, 480);
	_screen.create(640, 480, Graphics::PixelFormat::createFormatCLUT8());

	// _game->run();
	Common::File f;
	f.open("ADVENT.IDX");
	uint32 count = f.readUint32LE();
	int file_index = 0;
	while (file_index < count) {
		Common::String name = f.readPascalString(false);
		uint32 type = f.readUint32LE();
		uint32 offset = f.readUint32LE();
		uint32 length = f.readUint32LE();

		ResourceId id(type, name);
		ResourceEntry entry{};
		entry.offset = offset;
		entry.length = length;
		_resources[id] = entry;
		debug("Found file %d: %s, type=0x%x, offset=%d, size=%d", file_index++, name.c_str(), type, offset, length);
	}
	debug("Total number of resources: %d", _resources.size());

	// dumpResource("BPO3GOUT", ResourceId(RESOURCE_TYPE_CURSOR, "BPO3GOUT"));
	// loadAnimation("CHANGECD");

	/*
	loadBackground("MENU");

	for (auto l = _resources.begin(); l != _resources.end(); ++l) {
		if (l->_key._type == RESOURCE_TYPE_BACKGROUND) {
			loadBackground(l->_key._name.c_str());
		}
	}
	*/

	// playVideo("TEL-220");
	// // playVideo("MENGINE");
	playVideo("GNTLOGO");
	// playVideo("BRDFWR3");
	// playVideo("VVKSPACE");
	// playVideo("INTRO3");
	// playVideo("INTRO8");
	// playVideo("BRVLEFT");
	// playVideo("GNTLOGO");
	// playVideo("STICK");
	// playVideo("MENGINE");
	// loadScript("VVI2");
	// loadScript("MENU");
	// loadScript("OPTIONS");
	// loadScript("INTRO");
	// loadScript("ENTRY");

	/*
		// Simple main event loop
		Common::Event evt;
		while (!shouldQuit()) {
			g_system->updateScreen();
			g_system->getEventManager()->pollEvent(evt);
			g_system->delayMillis(1000 / 15);
		}
	*/

	return Common::kNoError;
}

int CruxEngine::loadResource(const ResourceId &res, uint8_t *&data, uint32_t &length) {
	ResourceEntry entry = _resources.getVal(res);
	length = entry.length;

	Common::File inf;
	inf.open("ADVENT.RES");
	inf.seek(entry.offset);
	data = new uint8_t[length];
	inf.read(data, length);
	inf.close();
	return 0;
}

byte *CruxEngine::loadPalette(const char *name) {
	byte *buffer;
	uint32_t length;

	loadResource(ResourceId(RESOURCE_TYPE_PALETTE, Common::String(name)), buffer, length);
	if (length != 786) {
		// palette is 786 uncompressed. compressed palettes not supported at the moment
		// there's an 18 bytes header for this resource.
		debug("Palette is not of the right length (%d != 786)", length);
		delete[] buffer;
		return nullptr;
	}

	byte *palette = new byte[768];

	// patch palette
	for (int i = 0; i < 768; i++) {
		palette[i] = (buffer[i + 18] << 2);
	}

	// TODO: add support for rle compressed palettes

	delete[] buffer;
	return palette;
}

void CruxEngine::loadAnimation(const char *name) {
	uint8_t *data;
	uint32_t length;

	loadResource(ResourceId(RESOURCE_TYPE_CURSOR, Common::String(name)), data, length);

	if (data[0] != 0x10 || data[1] != 0x01 || data[2] != 0x00 || data[7] != 0x08) {
		debug("Not a valid animation format");
		delete[] data;
		return;
	}

	int width = data[3] | (data[4] << 8);
	int height = data[5] | (data[6] << 8);
	int frames = data[8] | (data[9] << 8);
	debug("Animation of width=%d height=%d frames=%d", width, height, frames);

	/*
	auto *ptr = &data[0xc];
	for (auto i = 0; i < frames; i++) {
		uint16 x = ptr[0] | (ptr[1] << 8);
		uint16 y = ptr[2] | (ptr[3] << 8);
		uint16 frame_size = ptr[4] | (ptr[5] << 8);
		uint16 w = ptr[6] | (ptr[7] << 8);
		ptr += 8;
		debug("x=%d, y=%d, frame_size=%d, w=%d", x, y, frame_size, w);
	}

	debug("Bytes since beginning of buffer %ld", (long)ptr - (long)data);
	*/

	const uint8_t *palette = loadPalette("CHANGECD");

	auto frame_offset = 0xc + 8 * frames;

	auto background = loadBackground("OPTIONS");
	/*
	auto *background = new Graphics::Surface();
	background->create(640, 480, Graphics::PixelFormat::createFormatCLUT8());
	*/

	Graphics::Surface surface;
	surface.create(640, 480, Graphics::PixelFormat::createFormatCLUT8());

	for (auto i = 0; i < frames; i++) {
		char dummy[768];
		Common::sprintf_s(dummy, sizeof(dummy), "%s-%03d.png", name, i);

		const uint16 x0 = data[0x0c + i * 8 + 0] | (data[0x0c + i * 8 + 1] << 8);
		const uint16 y0 = data[0x0c + i * 8 + 2] | (data[0x0c + i * 8 + 3] << 8);
		const uint16 frame_size = data[0x0c + i * 8 + 4] | (data[0x0c + i * 8 + 5] << 8);
		debug("Current frame size: %d", frame_size);

		const Common::FSNode file(dummy);
		surface.copyFrom(*background);
		decodePicture1(data + frame_offset, frame_size, x0, y0, surface);
		Common::WriteStream *out = file.createWriteStream();
		Image::writePNG(*out, surface, palette);

		frame_offset += frame_size;
	}

	delete[] data;
	delete background;
}

Graphics::Surface *CruxEngine::loadBackground(const char *name) {
	uint8_t *data;
	uint32 length;

	uint8_t *palette = loadPalette(name);
	if (palette == nullptr) {
		return nullptr;
	}

	loadResource(ResourceId(RESOURCE_TYPE_BACKGROUND, Common::String(name)), data, length);

	if (data[0] != 0x10 || data[1] != 0x01 || data[8] != 0x01) {
		// note: other bytes are not checked in the original header as well
		debug("Loading background resource failed");
		delete[] palette;
		delete[] data;
		return nullptr;
	}

	uint16_t width = data[3] | (data[4] << 8);
	uint16_t height = data[5] | (data[6] << 8);
	uint16_t x = data[12] | (data[13] << 8);
	uint16_t y = data[14] | (data[15] << 8);
	uint32_t size = data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24);

	debug("w %d h %d x %d y %d size %d length %d", width, height, x, y, size, length);

	char dummy[768];
	Common::sprintf_s(dummy, sizeof(dummy), "%s.png", name);

	Common::FSNode file(dummy);
	auto *surface = new Graphics::Surface();
	surface->create(640, 480, Graphics::PixelFormat::createFormatCLUT8());

	uint8_t *ptr = &data[20];

	decodePicture1(ptr, size, 0, 0, *surface);

	Common::WriteStream *out = file.createWriteStream();
	Image::writePNG(*out, *surface, palette);

	delete[] data;
	delete[] palette;

	return surface;
}

void CruxEngine::dumpResource(const char *filename, const ResourceId &res) {
	uint8_t *data;
	uint32 length;

	loadResource(res, data, length);
	Common::DumpFile f;
	f.open(filename);
	f.write(data, length);
	f.close();

	delete[] data;
}

void CruxEngine::loadScript(const char *name) {
	debug("Loading script %s", name);

	ResourceId id(RESOURCE_TYPE_SCRIPT, Common::String(name));
	ResourceEntry entry = _resources.getVal(id);
	debug("Found script at %d %d", entry.offset, entry.length);

	Common::File f;
	f.open("ADVENT.RES");
	f.seek(entry.offset);

	uint script_type = f.readUint32LE();
	debug("Script type %d", script_type);

	auto strings = readArrayOfStrings(f);
	auto palettes = readArrayOfStrings(f);
	auto exits = readArrayOfStrings(f);
	auto animations = readArrayOfStrings(f);
	auto smc = readArrayOfStrings(f);
	auto themes = readArrayOfStrings(f);
	auto sounds = readArrayOfStrings(f);

	debug("Loaded script:");
	debug("  Strings: %s", serializeStringArray(strings).c_str());
	debug("  Palettes: %s", serializeStringArray(palettes).c_str());
	debug("  Exits: %s", serializeStringArray(exits).c_str());
	debug("  Animations: %s", serializeStringArray(animations).c_str());
	debug("  SMC: %s", serializeStringArray(smc).c_str());
	debug("  Themes: %s", serializeStringArray(themes).c_str());
	debug("  Sounds: %s", serializeStringArray(sounds).c_str());

	// readCursors();
	auto number_of_cursors = f.readUint32LE();
	f.skip(number_of_cursors * 176);

	// readAreas();
	auto number_of_areas = f.readUint32LE();
	f.skip(number_of_areas * 20);

	// unknown
	f.skip(0xf * 4);

	const uint32 number_of_scripts = f.readUint32LE();
	debug("Number of scripts: %d", number_of_scripts);

	Std::set<uint32> missing_opcodes;

	for (int i = 0; i < number_of_scripts; i++) {
		const uint32 number_of_commands = (script_type == 1 ? f.readByte() : f.readUint32LE());
		debug("Commands in script %d: %d", i, number_of_commands);

		for (int j = 0; j < number_of_commands; j++) {
			const auto opcode = f.readUint32LE();
			const auto arg1 = f.readUint32LE();
			const auto arg2 = f.readUint32LE();
			const auto arg3 = f.readUint32LE();

			switch (opcode) {

			case 0x03:
				debug("\t0x%04x: exit_value = exit_table_values[%d] /* %s */", j, arg1, exits[arg1].c_str());
				break;

			case 0x04:
				debug("\t0x%04x: vars[0x%x] = 0x%08x", j, arg1, arg2);
				break;

			case 0x05:
				debug("\t0x%04x: vars[0x%x]++", j, arg1);
				break;

			case 0x06:
				debug("\t0x%04x: vars[0x%x]--", j, arg1);
				break;

			case 0x07:
				debug("\t0x%04x: disable_cursor_by_field_0x14(%d)", j, arg1);
				break;

			case 0x08:
				debug("\t0x%04x: enable_cursor_by_field_0x14(%d)", j, arg1);
				break;

			case 0x09:
				// note: original code implements the skip instructions if <=
				debug("\t0x%04x: if vars[0x%x] > 0x%x {", j, arg1, arg2);
				break;

			case 0x0a:
				// note: original code implements the skip instructions if !=
				debug("\t0x%04x: if vars[0x%x] == 0x%x {", j, arg1, arg2);
				break;

			case 0x0b:
				// note: original code implements the skip instructions if >=
				debug("\t0x%04x: if vars[0x%x] < 0x%x {", j, arg1, arg2);
				break;

			case 0x0e:
				debug("\t0x%04x: if vars[0x%x] != 0x%x {", j, arg1, arg2);
				break;

			case 0x0c:
				if (!strcmp(name, "ENTRY")) {
					debug("\t0x%04x: /* 0xc in ENTRY script. code ends", j);
				} else {
					debug("\t0x%04x: (something with inventory)", j);
				}
				break;

			case 0x0f:
				debug("\t0x%04x: }", j);
				break;

			case 0x10:
				debug("\t0x%04x: } else {", j);
				break;

			case 0x13:
				debug("\t0x%04x: ani_rem_onscreen(0x%x)", j, arg1);
				break;

			case 0x14:
				debug("\t0x%04x: thm_play(0x%x)", j, arg1);
				break;

			case 0x15:
				debug("\t0x%04x: sfx_play(0x%x)", j, arg1);
				break;

			case 0x16:
			case 0x17:
				debug("\t0x%04x: nop", j);
				break;

			case 0x19:
				debug("\t0x%04x: ani_add_by_num(0x%x) /* %s */", j, arg1, animations[arg1].c_str());
				break;

			case 0x49:
				debug("\t0x%04x: wait_frames_no_async()", j);
				break;

			case 0x65:
				debug("\t0x%04x: call_script %d", j, arg1);
				break;

			case 0x70:
				debug("\t0x%04x: exit() /* ?? */", j);
				break;

			case 0x71:
				debug("\t0x%04x: intro_play(0x%x, 0x%x, 0x%x) /* %s */", j, arg1, arg2, arg3, smc[arg1].c_str());
				break;

			case 0x77:
			case 0x78:
				debug("\t0x%04x: scm_add(0x%x) /* \"%s\" */", j, arg1, smc[arg1].c_str());
				break;

			case 0xcd:
				debug("\t0x%04x: nwspeak(0x%x) /* %s */", j, arg1, "");
				break;

			case 0xff:
			case 0x100:
				debug("\t0x%04x: nop", j);
				break;

			case 0x12f:
				debug("\t0x%04x: refpal()", j);
				break;

			case 0x13c:
				debug("\t0x%04x: ani_set_frame(0x%x, %d) /* %s */", j, arg1, arg2, animations[arg1].c_str());
				break;

			case 0x16c:
				debug("\t0x%04x: thm_event(0x%x)", j, arg1);
				break;

			case 0x170:
				debug("\t0x%04x: fx_setvol(0x%x)", j, arg1);
				break;

			case 0x171:
				debug("\t0x%04x: si_snd_setvol(0x%x)", j, arg1);
				break;

			case 0x172:
				debug("\t0x%04x: si_spk_setvol(0x%x)", j, arg1);
				break;

			case 0x17a:
				debug("\t0x%04x: spk_stop()", j);
				break;

			case 0x191:
				debug("\t0x%04x: ani_suspend(0x%x)", j, arg1);
				break;

			case 0x195:
				debug("\t0x%04x: ani_clear_suspended(0x%x)", j, arg1);
				break;

			case 0x196:
				// note: this is flipped (arg2 first)
				debug("\t0x%04x: async_add_timer(0x%x, 0x%0x)", j, arg2, arg1);
				break;

			case 0x901:
				debug("\t0x%04x: gv_addbutton(%d, 0)", j, arg1);
				break;

			case 0x902:
				debug("\t0x%04x: gv_update_buttons()", j);
				break;

			case 0x903:
				// note: arg1 ignored
				debug("\t0x%04x: gv_addbutton(-1, %d)", j, arg2);
				break;

			case 0x905:
				debug("\t0x%04x: sav_select_load()", j);
				break;

			case 0x84c:
				debug("\t0x%04x: vars[0x%x] = si_get_vol()", j, arg1);
				break;

			case 0x850:
				debug("\t0x%04x: vars[0x%x] = txt_get_speed()", j, arg1);
				break;

			case 0x852:
				debug("\t0x%04x: txt_set_on(0x%x)", j, arg1);
				break;

			case 0x855:
				debug("\t0x%04x: vars[0x%x] = thunk_FUN_0047d7e0()", j, arg1);
				break;

			case 0x856:
				debug("\t0x%04x: vars[0x%x] = txt_get_on()", j, arg1);
				break;

			case 0x857:
				debug("\t0x%04x: vars[0x%x] = (_DAT_0062b284 == 0)", j, arg1);
				break;

			case 0x858:
				debug("\t0x%04x: vars[0x%x] = pal_get_brightness()", j, arg1);
				break;

			case 0x1004:
				debug("\t0x%04x: initialize_script()", j);
				break;

			case 0x13ba:
				debug("\t0x%04x: ani_add_by_num(num=0x%x, prio=0x%x) /* %s */", j, arg1, arg2, animations[arg1].c_str());
				break;

			case 0x1838:
				debug("\t0x%04x: gran_diary_init()", j);
				break;

			default:
				debug("\t0x%04x: 0x%08x 0x%08x 0x%08x 0x%08x", j, opcode, arg1, arg2, arg3);
				if (missing_opcodes.find(opcode) == missing_opcodes.end()) {
					missing_opcodes.insert(opcode);
				}
				break;
			}
		}
	}

	for (auto op : missing_opcodes) {
		debug("missing opcode: 0x%x", op);
	}
}

Common::String CruxEngine::serializeStringArray(Common::Array<Common::String> &arr) {
	Common::String result;

	result += "[";
	for (int i = 0; i < arr.size(); i++) {
		if (i > 0)
			result += ", ";

		result += "\"";
		result += arr[i];
		result += "\"";
	}

	result += "]";
	return result;
}

Common::Array<Common::String> CruxEngine::readArrayOfStrings(Common::File &f) {
	auto count = f.readUint32LE();
	Common::Array<Common::String> result;

	for (auto i = 0; i < count; i++) {
		Common::String s = f.readPascalString(false);
		result.push_back(s);
	}

	return result;
}

void CruxEngine::playVideo(const char *name) {
	debug("Playing video %s", name);

	ResourceId id(RESOURCE_TYPE_VIDEO, Common::String(name));
	ResourceEntry entry = _resources.getVal(id);
	debug("Found video at %d %d", entry.offset, entry.length);

	Common::File f;
	f.open("ADVENT.RES");
	f.seek(entry.offset);

	const uint16 a0 = f.readUint16LE();
	const uint16 a1 = f.readUint16LE();
	const uint16 frame_count = f.readUint16LE();
	const uint16 a3 = f.readUint16LE();
	debug("header: %04x %04x %04x %04x", a0, a1, frame_count, a3);

	f.skip(8);

	Graphics::Surface framebuffer;
	framebuffer.create(640, 480, Graphics::PixelFormat::createFormatCLUT8());

	int frame_index = 0;

	while (frame_index < frame_count) {

		uint16 count = f.readUint16LE();
		debug("Playing frame: %d, total chunks: %d", frame_index, count);
		uint16 chunk_index = 0;
		while (count-- > 0) {
			const auto chunk_size = f.readUint32LE();
			const auto chunk_type = f.readUint16LE();
			const auto unknown = f.readUint16LE();
			debug("Chunk index: %d, chunk type 0x%x, chunk size %d, unknown %d", chunk_index, chunk_type, chunk_size, unknown);
			chunk_index++;

			if (chunk_size == 0)
				continue;

			assert(chunk_size < 10000000);
			byte *buffer = new byte[chunk_size];
			f.read(buffer, chunk_size);

			if (chunk_type == 0x0082) {
				// audio
			}

			if (chunk_type == 0x0002) {
				decodePalette(buffer, chunk_size);
			}

			if (chunk_type == 0x0010) {
				decodePicture(buffer, chunk_size, framebuffer);
			}

			delete[] buffer;
		}

		g_system->copyRectToScreen(framebuffer.getPixels(), 640, 0, 0, 640, 480);
		g_system->updateScreen();
		g_system->delayMillis(1000 / 10);

		Common::Event evt;
		g_system->getEventManager()->pollEvent(evt);

		if (evt.kbd.keycode == Common::KEYCODE_ESCAPE) {
			g_system->quit();
		}

		if (shouldQuit()) {
			break;
		}

		frame_index++;
	}
}

static byte *put_single_col(byte *buffer, byte *tto, int block_width, int image_width) {
	// int a = 0;
	byte *b = tto;
	long direction = 1;
	byte *d = tto + block_width;

	byte color = *buffer++;
	while (*buffer != 0xff) {
		if (*buffer <= 0xee) {
			long f = abs(d - b);
			long g = *buffer;
			while (f <= g) {
				g = g - f;
				b = d + image_width - direction;
				d = d + image_width - direction * (block_width + 1);
				direction = -direction;
				f = block_width;
			}

			b += g * direction;
		} else {
			// *buffer > 0xee
			long h = *buffer - 0xee;
			long i = abs(d - b);
			while (i <= h) {
				while (b != d) {
					*b = color;
					b += direction;
				}

				d += image_width - direction * (block_width + 1);
				b += image_width - direction;
				direction = -direction;
				h = h - i;
				i = block_width;
			}

			byte *pbVar1 = b + h * direction;
			while (b != pbVar1) {
				*b = color;
				b += direction;
			}
		}

		buffer++;
	}

	// skip 0xff
	buffer++;
	return buffer;
}

static int DAT_00647848 = 0;

void CruxEngine::decodePalette(byte *buffer, uint32 length) {
	byte palette[768];

	const auto start = buffer[0];
	const auto end = buffer[1];
	const auto total = (end - start + 1) * 3;
	byte *out = palette;
	byte *in = buffer + 2;
	for (auto i = 0; i < total; i++) {
		*out++ = *in++ << 2;
	}

	g_system->getPaletteManager()->setPalette(palette, start, end - start + 1);
}

void CruxEngine::decodePicture(byte *buffer, uint32 length, Graphics::Surface surface) {
	auto type = buffer[0];
	switch (type) {

	case 0x01:
	case 0x03:
		decodePicture1(buffer, length, 0, 0, surface);
		break;

	case 0x04:
		// decodePicture4(buffer, length, surface);
		break;

	default:
		debug("decodePicture: unknown type %d", type);
		break;
	}
}

static byte *put_block_brun16(byte *buffer, byte *to, int image_width, int block_width) {

	const byte *copy_of_buffer = buffer + 1;

	int b = *buffer++;
	if (b != 0xff) {
		int local_44 = MIN(b, 16);
		buffer += local_44;
		DAT_00647848 = b;
	}

	long direction = 1;
	long local_3c = 0;
	long local_24 = 0; // color
	byte *local_2c = to + block_width;
	while (true) {
		int local_20 = (*buffer >> (1 - local_3c) * 4) & 0x0f;
		int local_28 = (buffer[local_3c] >> (local_3c * 4)) & 0x0f;
		buffer++;
		if (local_20 == 0 && local_28 == 0) {
			break;
		}

		if (local_28 != 0) {
			local_24 = copy_of_buffer[(*buffer >> ((1 - local_3c) * 4)) & 0x0f];
			buffer += local_3c;
			local_3c ^= 1;
		}

		long iVar1 = abs(local_2c - to);
		if (local_20 != 0 && iVar1 <= local_20) {
			while (to != local_2c) {
				*to = copy_of_buffer[(*buffer >> ((1 - local_3c) * 4)) & 0xf];
				buffer += local_3c;
				local_3c ^= 1;
				to += direction;
			}

			local_2c += image_width - (block_width + 1) * direction;
			to += image_width - direction;
			direction = -direction;
			local_20 -= iVar1;
		}

		byte *puVar2 = to + local_20 * direction;
		while (to != puVar2) {
			*to = copy_of_buffer[(*buffer >> ((1 - local_3c) * 4)) & 0xf];
			buffer += local_3c;
			local_3c ^= 1;
			to += direction;
		}

		if (local_28 != 0) {
			iVar1 = abs(local_2c - to);
			if (local_28 < iVar1) {
				while (local_28-- > 0) {
					*to = local_24;
					to += direction;
				}
			} else {
				while (to != local_2c) {
					*to = local_24;
					to += direction;
				}

				local_2c += image_width - (block_width + 1) * direction;
				to += image_width - direction;
				direction = -direction;
				local_28 -= iVar1;
				while (local_28-- > 0) {
					*to = local_24;
					to += direction;
				}
			}
		}
	}

	if (local_3c != 0) {
		buffer++;
	}

	return buffer;
}

static byte *put_block_skip64(byte *buffer, byte *to, int image_width, int block_width) {

	byte *copy_of_buffer = buffer;
	byte *tto = to;

	long direction = 1;
	int c = *buffer++;
	if (c != 0xff) {
		copy_of_buffer = buffer;
		DAT_00647848 = c;
		buffer += MIN(0x40, c);
	}

	c = DAT_00647848;
	byte *local_34 = to + block_width;
	while (*buffer != 0) {
		if ((*buffer & 0xc0) == 0) {
			long local_2c = abs(local_34 - to);
			long local_28 = *buffer;
			while (local_2c <= local_28) {
				local_28 = local_28 - local_2c;
				to = local_34 + image_width - direction;
				local_34 = local_34 + image_width - ((block_width + 1) * direction);
				direction = -direction;
				local_2c = block_width;
			}

			to = to + local_28 * direction;
		} else {
			long cVar1 = copy_of_buffer[*buffer & 0x3f];
			long local_24 = (*buffer & 0xc0) >> 6; // must be [1,2,3]
			long local_44 = abs(local_34 - to);
			while (local_44 <= local_24) {
				while (to != local_34) {
					*to = cVar1;
					to += direction;
				}

				local_34 = local_34 + image_width - (block_width + 1) * direction;

				to += image_width - direction;
				direction = -direction;
				local_24 -= local_44;
				local_44 = abs(local_34 - to);
			}

			byte *pcVar2 = to + local_24 * direction;
			while (to != pcVar2) {
				*to = cVar1;
				to += direction;
			}
		}

		buffer++;
	}

	// skip 0
	buffer++;

	for (int i = 0x40; i < c; i++) {
		buffer = put_single_col(buffer, tto, block_width, image_width);
	}

	return buffer;
}

static byte *dput_block_skip16(byte *buffer, byte *to, int image_width, int block_width) {

	byte *copy_of_buffer = buffer;
	byte *tto = to;

	long direction = 1;
	int uVar2 = *buffer++;
	if (uVar2 != 0xff) {
		copy_of_buffer = buffer;
		DAT_00647848 = uVar2;
		buffer += MIN(uVar2, 0x10);
	}

	uVar2 = DAT_00647848;
	byte *c = to + block_width;
	while (*buffer != 0) {
		if ((*buffer & 0xf0) == 0) {
			int a = abs(c - to);
			int b = *buffer;
			while (a <= b) {
				b = b - a;
				to = c + image_width - direction;
				c = c + (image_width - (block_width + 1) * direction);
				direction = -direction;
				a = block_width;
			}

			to = to + b * direction;
		} else {
			long high_nibble = copy_of_buffer[*buffer & 0x0f];
			long low_nibble = (*buffer & 0xf0) >> 4;
			int f = abs(c - to);
			if (f <= low_nibble) {
				while (to != c) {
					*to = high_nibble;
					to += direction;
				}

				c = c + (image_width - (block_width + 1) * direction);
				to = to + image_width - direction;
				direction = -direction;
				low_nibble = low_nibble - f;
			}

			byte *h = to + low_nibble * direction;
			while (to != h) {
				*to = high_nibble;
				to += direction;
			}
		}

		buffer++;
	}

	// skip 0
	buffer++;

	for (int i = 0x10; i < uVar2; i++) {
		buffer = put_single_col(buffer, tto, block_width, image_width);
	}

	return buffer;
}

static byte *dput_block_skip8(byte *buffer, byte *to, int image_width, int block_width) {

	byte *copy_of_buffer = buffer;
	byte *tto = to;

	long direction = 1;
	int uVar2 = *buffer++;
	if (uVar2 != 0xff) {
		int uVar3 = MIN(uVar2, 0x8);
		copy_of_buffer = buffer;
		DAT_00647848 = uVar2;
		buffer += uVar3;
	}

	uVar2 = DAT_00647848;
	byte *c = to + block_width;
	while (*buffer != 0) {
		if ((*buffer & 0xf8) == 0) {
			int a = abs(c - to);
			int b = *buffer;
			while (a <= b) {
				b = b - a;
				to = c + image_width - direction;
				c = c + (image_width - (block_width + 1) * direction);
				direction = -direction;
				a = block_width;
			}

			to = to + b * direction;
		} else {
			long high_nibble = copy_of_buffer[*buffer & 0x07];
			long low_nibble = (*buffer & 0xf8) >> 3;
			int f = abs(c - to);
			if (f <= low_nibble) {
				while (to != c) {
					*to = high_nibble;
					to += direction;
				}

				c = c + (image_width - (block_width + 1) * direction);
				to = to + image_width - direction;
				direction = -direction;
				low_nibble = low_nibble - f;
			}

			byte *h = to + low_nibble * direction;
			while (to != h) {
				*to = high_nibble;
				to += direction;
			}
		}

		buffer++;
	}

	// skip 0
	buffer++;

	for (int i = 8; i < uVar2; i++) {
		buffer = put_single_col(buffer, tto, block_width, image_width);
	}

	return buffer;
}

static byte *put_block_copy(byte *buffer, byte *to, int image_width, int block_width, int block_height) {

	auto blocky = block_height;

	while (blocky > 0) {

		byte *puVar2 = to + block_width;
		while (to < puVar2) {
			*to++ = *buffer++;
		}

		to += image_width - 1;
		puVar2 = to - block_width;
		while (puVar2 < to) {
			*to-- = *buffer++;
		}

		to += image_width + 1;

		blocky -= 2;
	}

	return buffer;
}

void CruxEngine::decodePicture4(byte *buffer, uint32 length, Graphics::Surface surface) {

	uint image_type = buffer[0];
	uint image_width = (buffer[1]) | (buffer[2] << 8);
	uint image_height = (buffer[3]) | (buffer[4] << 8);
	uint block_width = (buffer[5]) | (buffer[6] << 8);
	uint block_height = (buffer[7]) | (buffer[8] << 8);
	// uint blocks_wide = image_width / block_width;
	// uint blocks_high = image_height / block_height;
	debug("[decodePicture4] image_type %d image_width %d image_height %d, block_width %d, block_height %d", image_type, image_width, image_height, block_width, block_height);

	buffer += 9;

	for (auto y = 0; y < image_height; y += block_height) {
		for (auto x = 0; x < image_width; x += block_width) {
			uint8 type = *buffer++;
			if (type > 0) {
				debug("Block with type=0x%x", type);
			}

			byte *to = static_cast<byte *>(surface.getBasePtr(x, y));

			switch (type) {
			case 0:
				// nop, just ignore this byte
				break;

			case 1:
				buffer = put_block_copy(buffer, to, image_width, block_width, block_height);
				break;

			case 2:
				buffer = put_block_brun16(buffer, to, image_width, block_width);
				break;

			case 3:
				buffer = put_block_skip64(buffer, to, image_width, block_width);
				break;

			case 4:
				buffer = dput_block_skip16(buffer, to, image_width, block_width);
				break;

			case 8:
				buffer = dput_block_skip8(buffer, to, image_width, block_width);
				break;

			default:
				debug("Don't know how to handle type 0x%02x", type);
				return;
			}
		}
	}
}

int CruxEngine::decodePicture1(byte *buffer, uint32 length, uint x0, uint blt_y0, Graphics::Surface surface) {

	const byte image_type = buffer[0];
	const uint image_width = (buffer[1]) | (buffer[2] << 8);
	const uint image_height = (buffer[3]) | (buffer[4] << 8);
	uint y0 = (buffer[5]) | (buffer[6] << 8);
	uint height = (buffer[7]) | (buffer[8] << 8);

	debug("[decodePicture1] image_type %d image_width: %d, image_height %d y0 %d unknown %d", image_type, image_width, image_height, y0, height);

	int offset = 9;
	while (offset < length) {
		for (int y = 0; y < height; y++) {
			auto *dst = static_cast<byte *>(surface.getBasePtr(x0, y + y0 + blt_y0));

			const byte type = buffer[offset++];
			// debug("y=%d/%d type=%d", y, height, type);
			switch (type) {

			case 0x0:
				for (int x = 0; x < image_width; x++) {
					*dst++ = buffer[offset++];
				}
				break;

			case 0x1:
				while (true) {
					byte times = buffer[offset++];
					if (times == 0) {
						// end of line
						break;
					} else if (times < 0x80) {
						byte color = buffer[offset++];
						while (times-- > 0) {
							*dst++ = color;
						}
					} else {
						times = 256 - times;
						while (times-- > 0) {
							*dst++ = buffer[offset++];
						}
					}
				}
				break;

			case 0x02:
				while (true) {
					int times = buffer[offset++];
					if (times == 0) {
						break;
					}

					if (times >= 0x80) {
						times = 0x100 - times;
						while (times-- > 0) {
							*dst++ = buffer[offset++];
						}
					} else {
						dst += times;
					}
				}
				break;

			case 0x03: {
				byte skip = buffer[offset++];
				dst += skip;

				while (true) {
					byte times = buffer[offset++];
					if (times < 0x80 && times > 0) {
						byte c = buffer[offset++];
						while (times-- > 0) {
							*dst++ = c;
						}
					} else if (times >= 0x80) {
						times = 0x100 - times;
						while (times-- > 0) {
							*dst++ = buffer[offset++];
						}
					}

					skip = buffer[offset++];
					if (skip == 0xff) {
						break;
					}

					dst += skip;
				}
			} break;

			case 0x04:
				// dummy
				break;

			default:
				debug("Don't know how to handle type %d in decodePicture1", type);
				return -1;
			}
		}

		y0 += height;

		int skip_y = (buffer[offset + 0]) | (buffer[offset + 1] << 8);
		int new_height = (buffer[offset + 2]) | (buffer[offset + 3] << 8);
		offset += 4;
		debug("skipping %d", skip_y);
		y0 += skip_y;
		if (new_height == 0)
			break;

		height = new_height;
	}

	debug("");
	return offset;
}

bool CruxEngine::hasFeature(EngineFeature f) const {
	return (f == kSupportsReturnToLauncher);
}

} // End of namespace Crux
