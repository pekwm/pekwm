//
// TextureHandler.hh for pekwm
// Copyright (C) 2005-2025 Claes Nästén <pekdon@gmail.com>
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
#include "Util.hh"

#include <map>
#include <string>
#include <vector>

extern "C" {
#include <string.h>
}

class PTexture;

class TextureHandler {
public:
	typedef PTexture*(*parse_fun)(const std::string &texture,
				      const std::vector<std::string> &tok);

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

	TextureHandler();
	~TextureHandler();

	void registerTexture(const char *name, parse_fun fun);

	size_t getLengthMin() { return _length_min; }
	PTexture *getTexture(const std::string &texture);
	PTexture *referenceTexture(PTexture *texture);
	void returnTexture(PTexture *texture);

	void logTextures(const std::string& msg) const;

private:
	PTexture *parse(const std::string &texture);
	static PTexture *parseSolid(const std::string &texture,
				    const std::vector<std::string> &tok);
	static PTexture *parseSolidRaised(const std::string &texture,
					  const std::vector<std::string> &tok);
	static PTexture *parseLinesHorz(const std::string &texture,
					const std::vector<std::string> &tok);
	static PTexture *parseLinesVert(const std::string &texture,
					const std::vector<std::string> &tok);
	static PTexture *parseLines(bool horz,
				    const std::vector<std::string> &tok);
	static PTexture *parseImage(const std::string &texture,
				    const std::vector<std::string> &tok);
	static PTexture *parseImageMapped(const std::string& texture,
					  const std::vector<std::string> &tok);
	static PTexture *parseEmpty(const std::string& texture,
				    const std::vector<std::string> &tok);

	static bool parseSize(PTexture *tex, const std::string &size);

	/** Minimum texture name length. */
	size_t _length_min;

	std::vector<Util::StringTo<parse_fun> > _texture_types;

	entry_vector _textures;
	std::map<std::string, std::map<int,int>*> _color_maps;
};

namespace pekwm
{
	TextureHandler* textureHandler();
}

#endif // _PEKWM_TEXTUREHANDLER_HH_
