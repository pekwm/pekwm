//
// TextFormatter.hh for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_TEXT_FORMATTER_HH_
#define _PEKWM_PANEL_TEXT_FORMATTER_HH_

#include <string>
#include <vector>

#include "pekwm_panel.hh"
#include "VarData.hh"
#include "WmState.hh"

class TextFormatter
{
public:
	typedef std::string(*formatFun)(TextFormatter *tf,
			    const std::string& buf);

	TextFormatter(VarData& var_data, WmState& wm_state);
	~TextFormatter(void);

	bool referenceWmState(void) const { return _check_wm_state; }
	std::vector<std::string> getFields(void) { return _fields; }

	std::string preprocess(const std::string& raw_format);
	std::string format(const std::string& pp_format);

private:
	std::string format(const std::string& pp_format, formatFun exp);

	std::string preprocessVar(const std::string& var);
	std::string expandVar(const std::string& var);

	static std::string tfPreprocessVar(TextFormatter *tf,
					   const std::string& var)
	{
		return tf->preprocessVar(var);
	}

	static std::string tfExpandVar(TextFormatter *tf,
				       const std::string& var)
	{
		return tf->expandVar(var);
	}

private:
	VarData& _var_data;
	WmState& _wm_state;

	bool _check_wm_state;
	std::vector<std::string> _fields;
};

#endif // _PEKWM_PANEL_TEXT_FORMATTER_HH_
