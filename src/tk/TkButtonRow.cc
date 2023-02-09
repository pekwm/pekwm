#include "TkButtonRow.hh"

TkButtonRow::TkButtonRow(Theme::DialogData* data, PWinObj& parent,
			 stop_fun stop, std::vector<std::string> options)
	: TkWidget(data, parent)
{
	int i = 0;
	std::vector<std::string>::iterator it = options.begin();
	for (; it != options.end(); ++it) {
		_buttons.push_back(new TkButton(_data, parent, stop, i++, *it));
	}
}

TkButtonRow::~TkButtonRow(void)
{
	std::vector<TkButton*>::iterator it = _buttons.begin();
	for (; it != _buttons.end(); ++it) {
		delete *it;
	}
}
