//
// Frame.hh for pekwm
// Copyright (C) 2003-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_FRAME_HH_
#define _PEKWM_FRAME_HH_

#include "config.h"

#include "pekwm.hh"
#include "PDecor.hh"
#include "Client.hh"

#include "tk/Action.hh"

class PWinObj;
class Strut;
class AutoProperty;

#include <string>

class Frame : public PDecor
{
public:
	typedef std::vector<Frame*> frame_vec;
	typedef frame_vec::iterator frame_it;
	typedef frame_vec::const_iterator frame_cit;

	Frame(Client *client, AutoProperty *ap);
	virtual ~Frame(void);

	// START - PWinObj interface.
	virtual AtomName getWinType() const;

	virtual void iconify(void);
	virtual void toggleSticky();

	virtual void setWorkspace(unsigned int workspace);
	virtual void setLayer(Layer layer);

	virtual ActionEvent *handleMotionEvent(XMotionEvent *ev);
	virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev);
	virtual ActionEvent *handleLeaveEvent(XCrossingEvent *ev);

	virtual ActionEvent *handleMapRequest(XMapRequestEvent *ev);
	virtual ActionEvent *handleUnmapEvent(XUnmapEvent *ev);
	// END - PWinObj interface.

#ifdef PEKWM_HAVE_SHAPE
	void handleShapeEvent(XShapeEvent *ev);
#endif // PEKWM_HAVE_SHAPE

	// START - PDecor interface.
	virtual bool allowMove(void) const;

	virtual void addChild(PWinObj *child,
			      std::vector<PWinObj*>::iterator *it = 0);
	virtual void removeChild(PWinObj *child, bool do_delete = true);
	virtual void activateChild(PWinObj *child);

	virtual void updatedChildOrder(void);
	virtual void updatedActiveChild(void);

	virtual void getDecorInfo(char *buf, uint size,
				  const Geometry& gm);

	virtual void giveInputFocus(void);
	virtual bool setShaded(StateAction sa);
	virtual void setSkip(uint skip);
	virtual void clearFillStateAfterResize();
	// END - PDecor interface.

	Client *getActiveClient() const;

	void addChildOrdered(Client *child);

	static Frame *findFrameFromID(uint id);

	// START - Iterators
	static uint frame_size(void) { return _frames.size(); }
	static frame_cit frame_begin(void) { return _frames.begin(); }
	static frame_cit frame_end(void) { return _frames.end(); }
	static frame_vec::reverse_iterator frame_rbegin(void) {
		return _frames.rbegin();
	}
	static frame_vec::reverse_iterator frame_rend(void) {
		return _frames.rend();
	}

	Client *getTransFor(void) const {
		return _client?_client->getTransientForClient():0;
	}
	bool hasTrans(void) const {
		return _client && _client->hasTransients();
	}
	// Call getTransBegin() only (!) if hasTrans() == true.
	std::vector<Client*>::const_iterator getTransBegin(void) const {
		return _client->getTransientsBegin();
	}
	// Call getTransEnd() only (!) if hasTrans() == true.
	std::vector<Client*>::const_iterator getTransEnd(void) const {
		return _client->getTransientsEnd();
	}
	// END - Iterator

	inline uint getId(void) const { return _id; }
	void setId(uint id);

	Frame *detachClient(Client *client, int x, int y);

	const ClassHint& getClassHint() const { return _class_hint; }

	void setGeometry(const std::string& geometry, int head=-1,
			 bool honour_strut=false);
	const Geometry &getOldGeometry() const { return _old_gm; }
	void setOldGeometry(const Geometry &gm) { _old_gm = gm; }

	void growDirection(uint direction);
	void moveToHead(const std::string& arg);
	void moveToHead(int head_nr);

	void updateInactiveChildInfo(void);

	// state actions
	void setStateMaximized(StateAction sa, bool horz, bool vert, bool fill);
	void setStateFullscreen(StateAction sa);
	void setStateSticky(StateAction sa);
	void setStateAlwaysOnTop(StateAction sa);
	void setStateAlwaysBelow(StateAction sa);
	void setStateDecorBorder(StateAction sa);
	void setStateDecorTitlebar(StateAction sa);
	void setStateIconified(StateAction sa);
	void setStateTagged(StateAction sa, bool behind);
	void setStateSkip(StateAction sa, uint skip);
	void setStateTitle(StateAction sa, Client *client,
			   const std::string &title);
	void setStateMarked(StateAction sa, Client *client);
	void setStateOpaque(StateAction sa);

	void close(void);

	void readAutoprops(ApplyOn type = APPLY_ON_RELOAD);

	bool fixGeometry(void);

	// client message handling
	void handleConfigureRequest(XConfigureRequestEvent *ev,
				    Client *client);
	ActionEvent *handleClientMessage(XClientMessageEvent *ev,
					 Client *client);
	void handlePropertyChange(XPropertyEvent *ev, Client *client);

	static Frame *getTagFrame(void) { return _tag_frame; }
	static bool getTagBehind(void) { return _tag_behind; }

	static void resetFrameIDs(void);

protected:
	// used for testing
	Frame(void);

	// BEGIN - PDecor interface
	virtual void decorUpdated(void);

	virtual std::string getDecorName(void);
	// END - PDecor interface

	static void applyGeometry(Geometry &gm, const Geometry &ap_gm,
				  int mask);
	static void applyGeometry(Geometry &gm, const Geometry &ap_gm,
				  int mask, const Geometry &screen_gm);

private:
	Frame(const Frame&);
	Frame& operator=(const Frame&);

	void grabButtons(void);

	void handleClientStateMessage(XClientMessageEvent *ev, Client *client);
	static bool getStateActionFromMessage(XClientMessageEvent *ev,
					      StateAction &sa);
	void handleStateAtom(StateAction sa, Atom atom, Client *client);
	void handleCurrentClientStateAtom(StateAction sa, Atom atom,
					  Client *client);
	bool isRequestGeometryFullscreen(XConfigureRequestEvent *ev);

	void getMaxBounds(int &max_x,int &max_r, int &max_y, int &max_b);
	void calcSizeInCells(uint &width, uint &height, const Geometry& gm);
	void setGravityPosition(int gravity, int &x, int &y,
				int diff_w, int diff_h);
	void downSize(Geometry &gm, bool keep_x, bool keep_y);

	void handleTitleChange(Client *client, bool read_name);

	void getState(Client *cl);
	void applyState(Client *cl);
	void setFrameExtents(Client *cl);

	void setupAPGeometry(Client *client, AutoProperty *ap);

	void workspacesInsert();
	void workspacesRemove();

	static uint findFrameID(void);
	static void returnFrameID(uint id);

private:
	uint _id; // unique id of the frame

	Client *_client; // to skip all the casts from PWinObj
	ClassHint _class_hint;

	// frame information used when maximizing / going fullscreen
	Geometry _old_gm;
	uint _non_fullscreen_decor_state;
	Layer _non_fullscreen_layer;

	static frame_vec _frames; //!< Vector of all Frames.
	static std::vector<uint> _frameid_list; //!< Vector of free Frame IDs.

	static ActionEvent _ae_move;
	static ActionEvent _ae_resize;
	static ActionEvent _ae_move_resize;

	// Tagging, static as only one Frame can be tagged
	static Frame *_tag_frame; //!< Pointer to tagged frame.
	static bool _tag_behind; //!< Tagging actions will set behind.

	// EWMH
	static const int NET_WM_STATE_REMOVE = 0; // remove/unset property
	static const int NET_WM_STATE_ADD = 1; // add/set property
	static const int NET_WM_STATE_TOGGLE = 2; // toggle property
};

#endif // _PEKWM_FRAME_HH_
