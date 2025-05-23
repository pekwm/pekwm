//
// PDecor.hh for pekwm
// Copyright (C) 2004-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PDECOR_HH_
#define _PEKWM_PDECOR_HH_

#include "config.h"

#include "Config.hh"
#include "tk/PWinObj.hh"
#include "ThemeGm.hh"

class ActionEvent;
class PFont;

/**
 * Create window attributes.
 */
class CreateWindowParams {
public:
	CreateWindowParams(bool override_redirect)
	{
		attr.override_redirect = override_redirect;
	}

	bool override_redirect;
	int depth;
	long mask;
	XSetWindowAttributes attr;
	Visual *visual;
};

//! @brief PWinObj container class with fancy decor.
class PDecor : public PWinObj,
	       public ThemeGm,
	       public ThemeState
{
public:
	//! @brief Decor title button class.
	class Button : public PWinObj {
	public:
		Button(PWinObj *parent, Theme::PDecorButtonData *data,
		       uint width, uint height);
		~Button(void);

		const ActionEvent *findAction(XButtonEvent *ev);
		ButtonState getState(void) const { return _state; }
		void setState(ButtonState state);

		/** Returns wheter the button is positioned relative the left
		    title edge. */
		bool isLeft(void) const { return _left; }

	private:
		Theme::PDecorButtonData *_data;
		ButtonState _state;
		bool _left;
	};

	//! @brief Decor title item class.
	class TitleItem {
	public:
		//! Info bitmask enum.
		enum Info {
			INFO_MARKED = (1 << 1),
			INFO_ID = (1 << 2)
		};

		TitleItem(void);

		inline const std::string &getVisible(void) const {
			return _visible;
		}
		inline const std::string &getReal(void) const { return _real; }
		inline const std::string &getCustom(void) const {
			return _custom;
		}
		inline const std::string &getUser(void) const { return _user; }

		inline uint getCount(void) const { return _count; }
		inline uint getId(void) const { return _id; }
		inline bool isUserSet(void) const { return _user.size() > 0; }
		inline bool isCustom(void) const { return _custom.size() > 0; }
		inline uint getWidth(void) const { return _width; }

		void infoAdd(enum Info info);
		void infoRemove(enum Info info);
		bool infoIs(enum Info info);

		void setReal(const std::string &real) {
			_real = real;
			if (! isUserSet() && ! isCustom()) {
				updateVisible();
			}
		}
		void setCustom(const std::string &custom) {
			_custom = custom;
			if (_custom.size() > 0 && ! isUserSet()) {
				updateVisible();
			}
		}
		void setUser(const std::string &user) {
			_user = user;
			updateVisible();
		}
		void setCount(uint count) { _count = count; }
		void setId(uint id) { _id = id; }
		inline void setWidth(uint width) { _width = width; }

		void updateVisible(void);

	private:
		std::string _visible; //!< Visible version of title

		std::string _real; //!< Title from client
		/** Custom (title rule) set version of title */
		std::string _custom;
		std::string _user; //!< User set version of title

		uint _count; //!< Number of title
		uint _id; //!< ID of title
		uint _info; //!< Info bitmask for extra title info

		uint _width;
	};

	PDecor(const Window child_window, bool override_redirect, bool init,
	       const std::string &decor_name = DEFAULT_DECOR_NAME);
	virtual ~PDecor(void);

	// START - PWinObj interface.
	virtual void mapWindow(void);
	virtual void mapWindowRaised(void);
	virtual void unmapWindow(void);

	virtual void move(int x, int y);
	virtual void resize(uint width, uint height);
	virtual void moveResize(int x, int y, uint width, uint height);
	virtual void raise(void);
	virtual void lower(void);

	void moveResize(const Geometry &geometry, int gm_mask);

	virtual void setFocused(bool focused);
	virtual void setWorkspace(uint workspace);

	virtual void giveInputFocus();
	virtual void warpPointer();

	virtual const ActionEvent *handleButtonPress(XButtonEvent *ev);
	virtual const ActionEvent *handleButtonRelease(XButtonEvent *ev);
	virtual const ActionEvent *handleMotionEvent(XMotionEvent *ev);
	virtual const ActionEvent *handleEnterEvent(XCrossingEvent *ev);
	virtual const ActionEvent *handleLeaveEvent(XCrossingEvent *ev);

	virtual bool operator == (const Window &window);
	virtual bool operator != (const Window &window);
	// END - PWinObj interface.

	// START - PDecor interface.
	virtual bool allowMove(void) const { return true; }

	virtual void addChild(PWinObj *child,
			      std::vector<PWinObj*>::iterator *it = 0);
	virtual void removeChild(PWinObj *child, bool do_delete = true);
	virtual void activateChild(PWinObj *child);

	virtual void updatedChildOrder(void) { }
	virtual void updatedActiveChild(void) { }

	virtual void getDecorInfo(char *buf, uint size, const Geometry& gm);

	virtual bool setShaded(StateAction sa);
	virtual void setSkip(uint skip) { _skip = skip; }

	virtual std::string getDecorName(void);
	// END - PDecor interface.

	uint getSkip() const { return _skip; }
	bool isSkip(uint skip) const { return _skip & skip; }

	static std::vector<PDecor*>::const_iterator pdecor_begin(void) {
		return _pdecors.begin();
	}
	static std::vector<PDecor*>::const_iterator pdecor_end(void) {
		return _pdecors.end();
	}

	bool isAnyFill() const { return _fill_state; }
	void clearFill() { _fill_state = 0; }
	virtual void clearFillStateAfterResize() { }

	bool isMaximizedVert() const
	{
		return _fill_state & FILL_STATE_MAXIMIZED_VERT;
	}
	bool isMaximizedHorz() const
	{
		return _fill_state & FILL_STATE_MAXIMIZED_HORZ;
	}
	void setMaximized(bool horz, bool vert)
	{
		_fill_state =
			(horz ? FILL_STATE_MAXIMIZED_HORZ : 0)
			| (vert ? FILL_STATE_MAXIMIZED_VERT : 0);
	}
	void setFullscreen(bool fullscreen)
	{
		_fill_state = fullscreen ? FILL_STATE_FULLSCREEN : 0;
	}
	bool isFullscreen() const
	{
		return _fill_state & FILL_STATE_FULLSCREEN;
	}
	OrientationType getEdgeFilled() const
	{
		if (_fill_state & FILL_STATE_EDGE) {
			return _edge_filled;
		}
		return NO_EDGE;
	}
	void setEdgeFilled(OrientationType edge)
	{
		_fill_state = (edge != NO_EDGE ? FILL_STATE_EDGE : 0);
		_edge_filled = edge;
	}

	void addDecor(PDecor *decor);

	/**
	 * Updates _decor_name to represent decor state
	 * \return true if decor was changed
	 */
	bool updateDecor(void);
	void setDecorOverride(StateAction sa, const std::string &name);
	void loadDecor(void);

	//! @brief Returns title Window.
	inline Window getTitleWindow(void) const {
		return _title_wo.getWindow();
	}
	PDecor::Button *findButton(Window win);

	uint getRealHeight(void) const;

	uint getChildWidth(void) const;
	uint getChildHeight(void) const;

	// child list actions

	//! @brief Returns number of children in PDecor.
	inline uint size(void) const { return _children.size(); }
	//! @brief Returns iterator to the first child in PDecor.
	std::vector<PWinObj*>::const_iterator begin(void) {
		return _children.begin();
	}
	//! @brief Returns iterator to the last+1 child in PDecor.
	std::vector<PWinObj*>::const_iterator end(void) {
		return _children.end();
	}

	//! @brief Returns pointer to active PWinObj.
	inline PWinObj *getActiveChild() const { return _child; }
	PWinObj *getChildFromPos(int x);

	void activateChildNum(uint num);
	void activateChildRel(int off); // +/- relative from active
	void moveChildRel(int off); // +/- relative from active

	// title

	//! @brief Adds TitleItem to title list.
	inline void titleAdd(PDecor::TitleItem *ct) { _titles.push_back(ct); }
	//! @brief Removes all TitleItems from title list.
	inline void titleClear(void) { _titles.clear(); }
	//! @brief Sets active TitleItem.
	void titleSetActive(uint num) {
		_title_active = (num > _titles.size()) ? 0 : num;
	}

	// move and resize relative to the child instead of decor
	void moveChild(int x, int y);
	void resizeChild(uint width, uint height);

	// decor state

	/** Returns whether we have a border or not. */
	virtual bool hasBorder(void) const { return _border; }
	/** @brief Returns whether we have a title bar or not. */
	virtual bool hasTitlebar(void) const { return _titlebar; }
	/** @brief Returns whether we are shaded or not. */
	virtual bool isShaded(void) const { return _shaded; }
	bool setBorder(StateAction sa);
	bool setTitlebar(StateAction sa);

	bool demandAttention(void) const { return _attention; }
	void incrAttention(void) { ++_attention; }
	void decrAttention(void) {
		if (_attention && ! --_attention) {
			updateDecor();
		}
	}

	//! @brief Returns border position Window win is at.
	inline BorderPosition getBorderPosition(Window win) const {
		for (uint i = 0; i < BORDER_NO_POS; ++i) {
			if (_border_win[i] == win) {
				return BorderPosition(i);
			}
		}
		return BORDER_NO_POS;

	}

	void deiconify(void);

	static void drawOutline(const Geometry &gm, uint shaded);
	static void checkSnap(PWinObj *skip_wo, Geometry &gm);

protected:
	// START - PDecor interface.
	virtual void renderTitle(void);
	virtual void renderButtons(void);
	virtual void renderBorder(void);
	virtual void setBorderShape(void); // shapes border corners

	// called after loadDecor, render child
	virtual void loadTheme(void) { }
	// called after decor change (theme change, decor type change,
	// border state etc)
	virtual void decorUpdated(void) { }
	// END - PDecor interface.

#ifdef PEKWM_HAVE_SHAPE
	void applyBorderShape(int kind=ShapeBounding);
	void applyBorderShapeNormal(int kind, bool client_shape);
	void applyBorderShapeShaded(int kind);
	void applyBorderShapeBorder(int kind, Window shape);
#else
	void applyBorderShape(int kind=0) {}
#endif // PEKWM_HAVE_SHAPE

	void resizeTitle(void);

	static void checkWOSnap(const PWinObj *skip_wo, Geometry &gm);
	static void checkEdgeSnap(Geometry &gm);

	void alignChild(PWinObj *child);

	FocusedState getFocusedState(bool selected) const {
		if (selected) {
			return _focused
				? FOCUSED_STATE_FOCUSED_SELECTED
				: FOCUSED_STATE_UNFOCUSED_SELECTED;
		}
		return _focused
			? FOCUSED_STATE_FOCUSED
			: FOCUSED_STATE_UNFOCUSED;
	}

private:
	void init(Window child_window, bool override_redirect);

	void createParentWindow(CreateWindowParams &params,
				Window child_window);
	void createTitle(CreateWindowParams &params);
	void createBorder(CreateWindowParams &params);

	void setDataFromDecorName(const std::string &decor_name);
	void unloadDecor(void);

	const ActionEvent *handleButtonPressDecor(XButtonEvent *ev);
	const ActionEvent *handleButtonReleaseDecor(XButtonEvent *ev);
	const ActionEvent *handleButtonPressButton(XButtonEvent *ev,
						   PDecor::Button *button);
	const ActionEvent *handleButtonReleaseButton(XButtonEvent *ev,
						     PDecor::Button *button);
	const ActionEvent *handleButtonPressBorder(XButtonEvent *ev);
	const ActionEvent *handleButtonReleaseBorder(XButtonEvent *ev);

	void placeButtons(void);
	void placeBorder(void);
	void shapeBorder(void);
	void restackBorder(void); // shaded, borderless, no border visible

	void getBorderSize(BorderPosition pos, uint &width, uint &height);

	uint calcTitleWidth(void);
	uint calcTitleWidthDynamic(void);
	void calcTabsWidth(void);
	void calcTabsGetAvailAndTabWidth(uint &width_avail,
					 uint &tab_width,
					 int &off);
	void calcTabsWidthSymetric(void);
	void calcTabsWidthAsymetric(void);
	void calcTabsWidthAsymetricGrow(uint width_avail, uint width_total,
					uint tab_width);
	void calcTabsWidthAsymetricShrink(uint width_avail, uint tab_width);

protected:
	std::string _decor_name; //!< Name of the active decoration
	/** Original decor name if it is temp. overridden */
	std::string _decor_name_saved;

	PWinObj *_child; //!< Pointer to active child in PDecor.
	std::vector<PWinObj*> _children; //!< List of children in PDecor.

	PDecor::Button *_button; /**< Active title button in PDecor. */
	/** Active border window, for button release handling. */
	Window _button_press_win;

	// used for treshold calculation
	int _pointer_x; //!< Last click x root position.
	int _pointer_y; //!< Last click y root position.
	int _click_x; //!< Last click x window position.
	int _click_y; //!< Last click y window position.

	/** Boolean to set wheter ::move is overloaded. */
	bool _decor_cfg_child_move_overloaded;

	/** Boolean to configure wheter to call XReplayPointer on clicks. */
	bool _decor_cfg_bpr_replay_pointer;
	/** What list to search for child actions. */
	MouseActionListName _decor_cfg_bpr_al_child;
	/** What list to search for title actions. */
	MouseActionListName _decor_cfg_bpr_al_title;

	/** Default decor name in normal state. */
	static const std::string DEFAULT_DECOR_NAME;
	/** Default decor name in borderless state. */
	static const std::string DEFAULT_DECOR_NAME_BORDERLESS;
	/** Default decor name in titlebarless state. */
	static const std::string DEFAULT_DECOR_NAME_TITLEBARLESS;
	/** Default decor name for demands attention state. */
	static const std::string DEFAULT_DECOR_NAME_ATTENTION;

	Window _border_win[BORDER_NO_POS]; /** Array of border windows. */

private:
	Theme::PDecorData *_data;

	enum fill_state {
		FILL_STATE_MAXIMIZED_VERT = 1 << 0,
		FILL_STATE_MAXIMIZED_HORZ = 1 << 1,
		FILL_STATE_FULLSCREEN = 1 << 2,
		FILL_STATE_EDGE = 1 << 3
	};
	// state
	int _fill_state;
	OrientationType _edge_filled;
	uint _skip;

	// decor state
	bool _border;
	bool _titlebar;
	bool _shaded;
	uint _attention; // Number of children that demand attention
	bool _need_shape;
	uint _real_height;

	PWinObj _title_wo;
	std::vector<PDecor::Button*> _buttons;

	// variable decor data
	uint _title_active;
	std::vector<PDecor::TitleItem*> _titles;
	uint _titles_left, _titles_right; // area where to put titles

	static std::vector<PDecor*> _pdecors; /**< List of all PDecors */
};

#endif // _PEKWM_PDECOR_HH_
