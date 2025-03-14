//
// PanelWidget.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Cond.hh"
#include "Debug.hh"
#include "PanelWidget.hh"
#include "TextFormatter.hh"

static RequiredSizeChanged _required_size_changed;

PanelWidget::PanelWidget(const PanelWidgetData &data,
			 const PWinObj* parent,
			 const SizeReq& size_req,
			 const std::string& if_)
	: _os(data.os),
	  _observer(data.observer),
	  _theme(data.theme),
	  _var_data(data.var_data),
	  _wm_state(data.wm_state),
	  _parent(parent),
	  _dirty(true),
	  _if_tfo(_var_data, _wm_state, this, if_),
	  _x(0),
	  _rx(0),
	  _width(0),
	  _size_req(size_req),
	  _cond_true(Cond::eval(_if_tfo.format()))
{
	if (! _if_tfo.isFixed()) {
		pekwm::observerMapping()->addObserver(this, _observer, 100);
	}
}

PanelWidget::~PanelWidget()
{
	if (! _if_tfo.isFixed()) {
		pekwm::observerMapping()->removeObserver(this, _observer);
	}
}

void
PanelWidget::notify(Observable *, Observation *observation)
{
	if (_if_tfo.match(observation)) {
		bool cond_true = Cond::eval(_if_tfo.format());
		if (_cond_true != cond_true) {
			_dirty = true;
			_cond_true = cond_true;
			sendRequiredSizeChanged();
		}
	}
}

/**
 * Assign the given action for button clicks on the given button.
 */
void
PanelWidget::setButtonAction(int button, const PanelAction &action)
{
	_button_actions[button] = action;
}

void
PanelWidget::move(int x)
{
	_x = x;
	_rx = x + _width;
}

void
PanelWidget::click(int button, int, int)
{
	std::map<int, PanelAction>::iterator it(_button_actions.find(button));
	if (it != _button_actions.end()) {
		runAction(it->second);
	}
}

int
PanelWidget::renderText(Render &rend, PSurface *surface, PFont *font,
			int x, const std::string& text, uint max_width)
{
	int y = (_theme.getHeight() - font->getHeight()) / 2;
	return font->draw(surface, x, y, text, 0, max_width);
}

void
PanelWidget::sendRequiredSizeChanged()
{
	_dirty = true;
	pekwm::observerMapping()->notifyObservers(this,
						  &_required_size_changed);
}

void
PanelWidget::runAction(const PanelAction &action)
{
	TextFormatter tf(_var_data, _wm_state);
	std::string command = tf.format(action.getPpParam());
	if (action.getType() == PANEL_ACTION_EXEC) {
		runActionExec(action.getParam(), command);
	} else {
		runActionPekwm(action.getParam(), command);
	}
}

void
PanelWidget::runActionExec(const std::string &param,
			   const std::string &command)
{
	P_TRACE("PanelWidget action exec " << param << " command " << command);
	if (command.size() > 0) {
		std::vector<std::string> args =
			StringUtil::shell_split(command);
		_os->processExec(args);
	}
}

void
PanelWidget::runActionPekwm(const std::string &param,
			    const std::string &command)
{
	P_TRACE("PanelWidget action pekwm " << param << " command " << command);
	std::vector<std::string> args;
	args.push_back(BINDIR "/pekwm_ctrl");
	args.push_back(command);
	_os->processExec(args);
}
