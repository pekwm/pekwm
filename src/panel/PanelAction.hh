#ifndef _PANEL_ACTION_HH_
#define _PANEL_ACTION_HH_

#include <string>

enum PanelActionType {
	PANEL_ACTION_EXEC,
	PANEL_ACTION_PEKWM,
	PANEL_ACTION_NO
};

class PanelAction {
public:
	PanelAction()
		: _type(PANEL_ACTION_NO)
	{
	}
	PanelAction(PanelActionType type, const std::string &param,
		    const std::string &pp_param)
		: _type(type),
		  _param(param),
		  _pp_param(pp_param)
	{
	}
	PanelAction(const PanelAction &action)
		: _type(action.getType()),
		  _param(action.getParam()),
		  _pp_param(action.getPpParam())
	{
	}

	PanelActionType getType() const { return _type; }
	const std::string &getParam() const { return _param; }
	const std::string &getPpParam() const { return _pp_param; }

private:
	PanelActionType _type;

	std::string _param;
	/** Preprocessed version of param string using TextFormatter. */
	std::string _pp_param;
};

#endif // _PANEL_ACTION_HH_
