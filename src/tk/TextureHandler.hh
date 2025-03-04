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
#include "Container.hh"
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
	typedef PTexture*(*parse_fun)(float scale, const std::string &texture,
				      const std::vector<std::string> &tok);
	typedef Util::StringTo<parse_fun> string_to_parse_fun;

	class Entry {
	public:
		Entry(float scale, const std::string &name, PTexture *texture)
			: _scale(scale),
			  _name(name),
			  _texture(texture),
			  _ref(0)
		{
		}
		~Entry()
		{
			delete _texture;
		}

		float getScale() const { return _scale; }
		const std::string& getName() const { return _name; }
		PTexture *getTexture() { return _texture; }

		inline uint getRef(void) const { return _ref; }
		inline void incRef(void) { ++_ref; }
		inline void decRef(void) { if (_ref > 0) { --_ref; } }

		inline bool operator==(const std::string &name) {
			return pekwm::ascii_ncase_equal(_name, name);
		}

	private:
		float _scale;
		std::string _name;
		PTexture *_texture;

		uint _ref;
	};

	typedef std::vector<TextureHandler::Entry*> entry_vector;

	TextureHandler(float scale);
	~TextureHandler();

	float getScale() const { return _scale; }
	void setScale(float scale) { _scale = scale; }

	void registerTexture(const char *name, parse_fun fun);

	size_t getLengthMin() { return _length_min; }
	PTexture *getTexture(const std::string &texture);
	PTexture *referenceTexture(PTexture *texture);
	void returnTexture(PTexture **texture);

	void logTextures(const std::string& msg) const;

private:
	const string_to_parse_fun *getTextureTypes() const
	{
		return Container::type_data<string_to_parse_fun,
		       const string_to_parse_fun*>(_texture_types);
	}

	PTexture *parse(const std::string &texture);
	static PTexture *parseSolid(float scale, const std::string &str,
				    const std::vector<std::string> &tok);
	static PTexture *parseSolidRaised(float scale, const std::string &str,
					  const std::vector<std::string> &tok);
	static PTexture *parseLinesHorz(float scale, const std::string &str,
					const std::vector<std::string> &tok);
	static PTexture *parseLinesVert(float scale, const std::string &str,
					const std::vector<std::string> &tok);
	static PTexture *parseLines(float scale, bool horz,
				    const std::vector<std::string> &tok);
	static PTexture *parseImage(float scale, const std::string &str,
				    const std::vector<std::string> &tok);
	static PTexture *parseImageMapped(float scale, const std::string& str,
					  const std::vector<std::string> &tok);
	static PTexture *parseEmpty(float scale, const std::string& str,
				    const std::vector<std::string> &tok);

	static bool parseSize(PTexture *tex, float scale,
			      const std::string &size);
	static uint parsePixels(float scale, const std::string &str);

	/** Size scaling for texture values. */
	float _scale;
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
