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

#ifndef ULTIMA8_FILESYS_FILESYSTEM_H
#define ULTIMA8_FILESYS_FILESYSTEM_H

#include "ultima/shared/std/string.h"
#include "ultima/ultima8/filesys/idata_source.h"
#include "ultima/ultima8/filesys/odata_source.h"
#include "common/file.h"

namespace Ultima {
namespace Ultima8 {

class FileSystem {
public:
	FileSystem();
	~FileSystem();

	static FileSystem *get_instance() {
		return _fileSystem;
	}

	//! Open a file as readable. Streamed.
	//! \param vfn the (virtual) filename
	//! \param is_text open in text mode?
	//! \return nullptr on failure
	IDataSource *ReadFile(const Std::string &vfn, bool is_text = false);

	//! Open a file as writable. Streamed.
	//! \param vfn the (virtual) filename
	//! \param is_text open in text mode?
	//! \return a new writestream, or nullptr on failure
	Common::WriteStream *WriteFile(const Std::string &vfn, bool is_text = false);

	//! Create a directory
	//! \param path the directory to create. (Can be virtual)
	//! \return true if successful; otherwise, false.
	bool MkDir(const Std::string &path); // can handle both paths and vpaths

private:
	static bool base_to_uppercase(Std::string &str, int count);

	static bool IsDir(const Std::string &path);

	static FileSystem *_fileSystem;

	/**
	 *	Open a file for reading,
	 *	Output: false if couldn't open.
	 */
	bool rawOpen(Common::SeekableReadStream *&in, const Std::string &fname);

	/**
	 *	Open a file for writing,
	 *	Output: false if couldn't open.
	 */
	bool rawOpen(Common::WriteStream *&out, const Std::string &fname);
};

} // End of namespace Ultima8
} // End of namespace Ultima

#endif
