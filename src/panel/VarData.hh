//
// VarData.hh for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_VAR_DATA_HH_
#define _PEKWM_PANEL_VAR_DATA_HH_

#include <map>
#include <string>

#include "Observable.hh"

class FieldObservation : public Observation
{
public:
	FieldObservation(const std::string& field);
	virtual ~FieldObservation(void);

	const std::string& getField(void) const { return _field; }

private:
	std::string _field;
};

/**
 * Variable data storage, used by WmState and ExternalCommandData to
 * store and notify data.
 */
class VarData : public Observable
{
public:
	const std::string& get(const std::string& field) const;
	void set(const std::string& field, const std::string& value);

private:
	std::map<std::string, std::string> _vars;
};

#endif // _PEKWM_PANEL_VAR_DATA_HH_
