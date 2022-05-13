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

#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/debug.h"
#include "common/error.h"
#include "common/events.h"
#include "common/file.h"
#include "common/hashmap.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "graphics/screen.h"
#include "graphics/surface.h"

#include "crux/crux.h"

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
	// 0x1dcd
	uint32 signature = f.readUint16LE();
	while (f.pos() < f.size()) {
		Common::String name = f.readPascalString(false);
		uint32 type = f.readUint32LE();
		uint32 offset = f.readUint32LE();
		uint32 length = f.readUint32LE();

		ResourceId id(type, name);
		ResourceEntry entry;
		entry.offset = offset;
		entry.length = length;
		_resources[id] = entry;
		// debug("Found file: %s, type=%d, offset=%d, size=%d", id.c_str(), type, offset, size);
	}

	debug("Total number of resources: %d", _resources.size());

        // playVideo("VVKSPACE");
        playVideo("INTRO3");
        // playVideo("GNTLOGO");
        // playVideo("STICK");
	// playVideo("MENGINE");
	// loadScript("MENU");
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

void CruxEngine::loadScript(const char *name) {
	debug("Loading script %s", name);

	ResourceId id(4, Common::String(name));
	ResourceEntry entry = _resources.getVal(id);
	debug("Found script at %d %d", entry.offset, entry.length);

	Common::File f;
	f.open("ADVENT.RES");
	f.seek(entry.offset);

	uint script_type = f.readUint32LE();
	debug("Script type %d", script_type);

                        // $this->constants = $this->readConsts(fopen("22.MENU", "rb"));
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

	uint32 number_of_scripts = f.readUint32LE();
	debug("Number of scripts: %d", number_of_scripts);

	for (int i=0; i<number_of_scripts; i++) {
		uint32 number_of_commands = (script_type == 1 ? f.readByte() : f.readUint32LE());
		debug("Commands in script %d: %d", i, number_of_commands);
		f.skip(16*number_of_commands);
	}
}

Common::String CruxEngine::serializeStringArray(Common::Array<Common::String> &arr) {
	Common::String result;

	result += "[";
	for (int i=0; i<arr.size(); i++) {
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

	for (auto i=0; i<count; i++) {
		Common::String s = f.readPascalString(false);
		result.push_back(s);
	}

	return result;
}

void CruxEngine::playVideo(const char *name) {
	debug("Playing video %s", name);

	ResourceId id(16, Common::String(name));
	ResourceEntry entry = _resources.getVal(id);
	debug("Found video at %d %d", entry.offset, entry.length);

	Common::File f;
	f.open("ADVENT.RES");
	f.seek(entry.offset);

        int a0 = f.readUint16LE();
        uint16 a1 = f.readUint16LE();
        uint16 frame_count = f.readUint16LE();
        uint16 a3 = f.readUint16LE();
        debug("header: %04x %04x %04x %04x", a0, a1, frame_count, a3);

        f.skip(8);

	Graphics::Surface framebuffer;
	framebuffer.create(640, 480, Graphics::PixelFormat::createFormatCLUT8());

        int frame_index = 0;

	while (frame_index < frame_count) {

		uint16 count = f.readUint16LE();
		while (count-- > 0) {
                        debug("Chunks left %d", count);

			uint32 chunk_size = f.readUint32LE();
			uint16 chunk_type = f.readUint16LE();
			uint16 unknown = f.readUint16LE();
			debug("Chunk type 0x%x, chunk size %d, unknown %d", chunk_type, chunk_size, unknown);

			if (chunk_size == 0)
				continue;

			if (chunk_size > 10000000) {
				debug("OUCH chunk size too big");
				return;
			}

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

		if (shouldQuit()) {
			break;
                }

                frame_index++;
	}
}

byte *put_single_col(byte *buffer, byte *tto, int block_width, int image_width) {
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

                        b = b + g * direction;
                } else {
                        // *buffer > 0xee
                        long h = *buffer - 0xee;
                        long i = abs(d - b);
                        while (i <= h) {
                                while (b != d) {
                                        *b = color;
                                        b += direction;
                                }

                                d = d + image_width - direction * (block_width + 1);
                                b = b + image_width - direction;
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

        int start = buffer[0];
        int end = buffer[1];
        auto total = (end - start + 1) * 3;
        byte *out = palette;
        byte *in = buffer + 2;
        for (auto i = 0; i < total; i++) {
                *out++ = *in++ << 2;
        }

        g_system->getPaletteManager()->setPalette(palette, start, end - start + 1);
}

void CruxEngine::decodePicture(byte *buffer, uint32 length, Graphics::Surface surface) {
        uint8 type = buffer[0];
        if (type == 0x04) {
                decodePicture4(buffer, length, surface);
        } else {
                decodePicture1(buffer, length, surface);
        }
}

void CruxEngine::decodePicture4(byte *buffer, uint32 length, Graphics::Surface surface) {

	uint image_width = (buffer[1]) | (buffer[2] << 8);
	uint image_height = (buffer[3]) | (buffer[4] << 8);
	uint block_width = (buffer[5]) | (buffer[6] << 8);
	uint block_height = (buffer[7]) | (buffer[8] << 8);
        // uint blocks_wide = image_width / block_width;
        // uint blocks_high = image_height / block_height;

	buffer += 9;
        uint8 *copy_of_buffer = NULL;

	for (int y=0; y<image_height; y += block_height) {
		for (int x=0; x<image_width; x += block_width) {
                        debug("%d,%d", y, x);
			uint8 type = *buffer++;
                        if (type > 0)
                        debug("Block with type=%x", type);

                        byte *to = (byte *)surface.getBasePtr(x, y);
                        byte *tto = to;

   			switch (type) {
                                case 0:
                                // nop, just ignore this byte
                                break;

                                case 1:
                                // put_block_copy();
                                for (int a=0; a<32; a++) {
                                        for (int b=0; b<32; b++) {
                                                to = (byte *)surface.getBasePtr(x+b, y+a);
                                                *to = 0x7f;
                                        }
                                }
                                // return;
                                break;

                                case 2:
                                // put_block_brun16();
                                for (int a=0; a<32; a++) {
                                        for (int b=0; b<32; b++) {
                                                to = (byte *)surface.getBasePtr(x+b, y+a);
                                                *to = 0xff;
                                        }
                                }
                                // return;
                                break;

                                case 3:
                                // return;
                                // put_block_skip64();
                                {
                                        long direction = 1;
                                        int b = *buffer++;
                                        if (b != 0xff) {
                                                int local_50 = MIN(0x40, b);
                                                copy_of_buffer = buffer;
                                                DAT_00647848 = b;
                                                buffer += local_50;
                                        }

                                        b = DAT_00647848;
                                        byte *local_34 = to + block_width;
                                        while (*buffer != 0) {
                                                debug("local %p to %p", local_34, to);
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

                                                                local_34 = local_34 + image_width - ((block_width + 1) * direction);

                                                                to = to + image_width - direction;
                                                                direction = -direction;
                                                                local_24 = local_24 - local_44;
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
                                        for (int i=0x40; i<b; i++) {
                                                buffer = put_single_col(buffer, tto, block_width, image_width);
                                        }
                                }
                                break;

   				case 4:
                                {
                                        // dput_block_skip16();
                                        long direction = 1;
                                        int uVar2 = *buffer++;
                                        if (uVar2 != 0xff) {
                                                int uVar3 = MIN(uVar2, 0x10);
                                                copy_of_buffer = buffer;
                                                DAT_00647848 = uVar2;
                                                buffer += uVar3;
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
                                                // bank = (byte *)put_single_col(bank,tto,block_width,pitch);
                                                buffer = put_single_col(buffer, tto, block_width, image_width);
                                        }
                                }
                                break;

                                case 8:
                                // dput_block_skip8();
                                {
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
                                                // bank = (byte *)put_single_col(bank,tto,block_width,pitch);
                                                buffer = put_single_col(buffer, tto, block_width, image_width);
                                        }
                                }
                                break;

                                default:
                                debug("Don't know how to handle type 0x%02x", type);
                                return;
			}

			// char *gg = 0; *gg = 1;
			// return;
		}
	}
}

void CruxEngine::decodePicture1(byte *buffer, uint32 length, Graphics::Surface surface) {

	debug("What's this: %02x %02x %02x %02x %02x %02x %02x %02x %02x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8]);
	byte image_type = buffer[0];
	uint image_width = (buffer[1]) | (buffer[2] << 8);
	uint image_height = (buffer[3]) | (buffer[4] << 8);
	uint y0 = (buffer[5]) | (buffer[6] << 8);
	uint height = (buffer[7]) | (buffer[8] << 8);

	debug("image_type %d y0 %d height %d, image: %d x %d", image_type, y0, height, image_width, image_height);

	int offset = 9;
	while (offset < length) {
		for (int y = 0; y < height; y++) {
			byte *dst = (byte *)surface.getBasePtr(0, y + y0);

			byte type = buffer[offset++];
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
						times = 256 - times;
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
						times = 256 - times;
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
			}
			break;

			case 0x04:
			// dummy
			break;

			default:
				debug("Don't know how to handle type %d", type);
				return;
				break;
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
}

bool CruxEngine::hasFeature(EngineFeature f) const {
	return (f == kSupportsReturnToLauncher);
}

} // End of namespace Crux
