//
// TextureHandler.hh for pekwm
// Copyright (C) 2005-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_TEXTUREHANDLER_HH_
#define _PEKWM_TEXTUREHANDLER_HH_

#include "config.h"

#include "Compat.hh"
#include "PTexture.hh"
#include "String.hh"

#include <map>
#include <string>
#include <vector>

extern "C" {
#include <string.h>
}

class PTexture;

class TextureHandler {
public:
	class Entry {
	public:
		Entry(const std::string &name, PTexture *texture)
			: _name(name),
			  _texture(texture),
			  _ref(0)
		{
		}
		~Entry(void)
		{
			delete _texture;
		}

		PTexture *getTexture(void) { return _texture; }
		const std::string& getName(void) const { return _name; }

		inline uint getRef(void) const { return _ref; }
		inline void incRef(void) { ++_ref; }
		inline void decRef(void) { if (_ref > 0) { --_ref; } }

		inline bool operator==(const std::string &name) {
			return pekwm::ascii_ncase_equal(_name, name);
		}

	private:
		std::string _name;
		PTexture *_texture;

		uint _ref;
	};

	typedef std::vector<TextureHandler::Entry*> entry_vector;

	TextureHandler(void);
	~TextureHandler(void);

	size_t getLengthMin(void) { return _length_min; }
	PTexture *getTexture(const std::string &texture);
	PTexture *referenceTexture(PTexture *texture);
	void returnTexture(PTexture *texture);

	void logTextures(const std::string& msg) const;

private:
	PTexture *parse(const std::string &texture);
	PTexture *parseSolid(std::vector<std::string> &tok);
	PTexture *parseSolidRaised(const std::vector<std::string> &tok);
	PTexture *parseLines(bool horz, std::vector<std::string> &tok);
	PTexture *parseImage(const std::string& texture);
	PTexture *parseImageMapped(const std::string& texture);

	bool parseSize(PTexture *tex, const std::string &size);

private:
	/** Minimum texture name length. */
	const size_t _length_min;

	entry_vector _textures;
	std::map<std::string, std::map<int,int>*> _color_maps;
};

namespace pekwm
{
	TextureHandler* textureHandler();
}

#endif // _PEKWM_TEXTUREHANDLER_HH_
