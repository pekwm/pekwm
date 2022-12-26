//
// TextFormatter.cc for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "TextFormatter.hh"

/** empty string, used as default return value. */
static std::string _empty_string;

TextFormatter::TextFormatter(VarData& var_data, WmState& wm_state)
	: _var_data(var_data),
	  _wm_state(wm_state),
	  _check_wm_state(false)
{
}

TextFormatter::~TextFormatter(void)
{
}

/**
 * pre process format string for use with the format command, expands
 * environment variables and other static data.
 */
std::string
TextFormatter::preprocess(const std::string& raw_format)
{
	return format(raw_format, TextFormatter::tfPreprocessVar);
}

/**
 * format previously pre-processed format string expanding external
 * command data and wm state.
 */
std::string
TextFormatter::format(const std::string& pp_format)
{
	return format(pp_format, TextFormatter::tfExpandVar);
}

std::string
TextFormatter::format(const std::string& pp_format, formatFun exp)
{
	std::string formatted;

	bool in_escape = false, in_var = false;
	std::string buf;
	size_t size = pp_format.size();
	for (size_t i = 0; i < size; i++) {
		char chr = pp_format[i];
		if (in_escape) {
			buf += chr;
			in_escape = false;
		} else if (chr == '\\') {
			in_escape = true;
		} else if (in_var && isspace(chr)) {
			formatted += exp(this, buf);
			buf = chr;
			in_var = false;
		} else if (! in_var && chr == '%') {
			if (! buf.empty()) {
				formatted += buf;
				buf = _empty_string;
			}
			in_var = true;
		} else {
			buf += chr;
		}
	}
	if (! buf.empty()) {
		if (in_var) {
			formatted += exp(this, buf);
		} else {
			formatted += buf;
		}
	}

	return formatted;
}

std::string
TextFormatter::preprocessVar(const std::string& buf)
{
	if (buf.empty()) {
		return "%";
	}

	switch (buf[0]) {
	case '_': {
		return Util::getEnv(buf.substr(1));
	}
	case ':':
		_check_wm_state = true;
		return "%" + buf;
	default:
		_fields.push_back(buf);
		return "%" + buf;
	}
}

std::string
TextFormatter::expandVar(const std::string& buf)
{
	if (buf.empty()) {
		return _empty_string;
	}

	if (buf[0] == ':') {
		// window manager state variable
		if (buf == ":CLIENT_NAME:") {
			Window win = _wm_state.getActiveWindow();
			ClientInfo *client_info =
				_wm_state.findClientInfo(win);
			return client_info
				? client_info->getName()
				: _empty_string;
		} else if (buf == ":WORKSPACE_NUMBER:") {
			return std::to_string(
					_wm_state.getActiveWorkspace() + 1);
		} else if (buf == ":WORKSPACE_NAME:") {
			return _wm_state.getWorkspaceName(
					_wm_state.getActiveWorkspace());
		}
		return _empty_string;
	}

	// external command data
	return _var_data.get(buf);
}

