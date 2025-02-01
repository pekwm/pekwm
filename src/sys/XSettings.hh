//
// XSettings.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_XSETTINGS_HH_
#define _PEKWM_XSETTINGS_HH_

#include "CfgParserSource.hh"
#include "Types.hh"
#include "X11.hh"

#include <string>
extern "C" {
#ifdef PEKWM_HAVE_STDINT_H
#include <stdint.h>
#endif // PEKWM_HAVE_STDINT_H
}

enum XSettingType {
	XSETTING_TYPE_INTEGER = 0,
	XSETTING_TYPE_STRING = 1,
	XSETTING_TYPE_COLOR = 2
};

class XSetting {
public:
	virtual ~XSetting() { }

	enum XSettingType getType() const { return _type; }
	uint32_t getLastChanged() const { return _last_changed; }
	void setLastChanged(unsigned long last_changed)
	{
		_last_changed = last_changed;
	}

	virtual void write(std::ostream &os, const std::string& name);
	virtual std::string toString() const = 0;

	static bool validateName(const std::string &name, std::string &error);

protected:
	XSetting(enum XSettingType type)
		: _type(type),
		  _last_changed(0)
	{
	}

	void writePad(std::ostream &os, size_t len, size_t multiple);

private:
	enum XSettingType _type;
	uint32_t _last_changed;
};

/**
 * String, 32bit length.
 */
class XSettingString : public XSetting {
public:
	XSettingString(const std::string &value)
		: XSetting(XSETTING_TYPE_STRING),
		  _value(value)
	{
	}
	virtual ~XSettingString() { }

	virtual void write(std::ostream &os, const std::string& name);
	virtual std::string toString() const;

	const std::string &getValue() const { return _value; }

private:
	std::string _value;
};

/**
 * Integer 32bit signed.
 */
class XSettingInteger : public XSetting {
public:
	XSettingInteger(int32_t value)
		: XSetting(XSETTING_TYPE_INTEGER),
		  _value(value)
	{
	}
	virtual ~XSettingInteger() { }

	virtual void write(std::ostream &os, const std::string& name);
	virtual std::string toString() const;

	int32_t getValue() const { return _value; }

private:
	int32_t _value;
};

/**
 * Color with alpha.
 *
 * 2 r (uint16)
 * 2 g (uint16)
 * 2 b (uint16)
 * 2 a (uint16)
 */
class XSettingColor : public XSetting {
public:
	XSettingColor(uint16_t r, uint16_t g, uint16_t b, uint16_t a)
		: XSetting(XSETTING_TYPE_COLOR),
		  _r(r),
		  _g(g),
		  _b(b),
		  _a(a)
	{
	}
	virtual ~XSettingColor() { }

	virtual void write(std::ostream &os, const std::string& name);
	virtual std::string toString() const;

	uint16_t getR() const { return _r; }
	uint16_t getG() const { return _g; }
	uint16_t getB() const { return _b; }
	uint16_t getA() const { return _a; }

private:
	uint16_t _r;
	uint16_t _g;
	uint16_t _b;
	uint16_t _a;
};

XSetting *mkXSetting(const std::string& str);

class XSettings {
public:
	typedef std::map<std::string, XSetting*> map;

	XSettings();
	~XSettings();

	bool setOwner();
	bool isOwner() const { return _owner; }
	void updateServer();

	bool load(const std::string &source,
		  CfgParserSource::Type type = CfgParserSource::SOURCE_FILE);
	bool save(const std::string &path);
	void save(std::ostream &os);

	const XSetting *get(const std::string &name) const;

	void setString(const std::string &name, const std::string &value);
	void setInt32(const std::string &name, int32_t value);
	void setColor(const std::string &name, uint16_t r, uint16_t g,
		      uint16_t b, uint16_t a);

protected:
	void writeProp(std::ostream &os, unsigned long serial);

private:
	void set(const std::string &name, XSetting *setting);
	void sendOwnerMessage(Time timestamp);

	unsigned long getTime(Time &cur_time) const;

	Window _window;
	Atom _session_atom;
	bool _owner;
	map _settings;
};

#endif // _PEKWM_XSETTINGS_HH_
