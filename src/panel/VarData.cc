//
// VarData.cc for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "VarData.hh"

/** empty string, used as default return value. */
static std::string _empty_string;

FieldObservation::FieldObservation(const std::string& field)
	: _field(field)
{
}

FieldObservation::~FieldObservation(void)
{
}

const std::string&
VarData::get(const std::string& field) const
{
	std::map<std::string, std::string>::const_iterator it =
	  _vars.find(field);
	return it == _vars.end() ? _empty_string : it->second;
}

void
VarData::set(const std::string& field, const std::string& value)
{
	// update the value in the map before notifying in case the
	// value is read by the obvserver
	_vars[field] = value;

	FieldObservation field_obs(field);
	pekwm::observerMapping()->notifyObservers(this, &field_obs);
}
