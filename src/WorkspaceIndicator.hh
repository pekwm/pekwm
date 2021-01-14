//
// WorkspaceIndicator.hh for pekwm
// Copyright (C) 2009-2020 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _WORKSPACE_INDICATOR_HH_
#define _WORKSPACE_INDICATOR_HH_

#include "config.h"

#include "pekwm.hh"

#include "PWinObj.hh"
#include "PDecor.hh"

/**
 * Workspace indicator rendering a simple window with workspace layout
 * showing the number and name of the active workspace.
 */
class WorkspaceIndicator : public PDecor
{
public:
    /**
     * Display class rendering workspace layout in WorkspaceIndicator.
     */
    class Display : public PWinObj {
    public:
        Display(PWinObj *parent);
        virtual ~Display(void);

        virtual bool getSizeRequest(Geometry &request);
        void render(void);

    private:
        void renderWorkspaces(int x, int y, uint width, uint height);

        uint getPaddingHorizontal(void);
        uint getPaddingVertical(void);

        Pixmap _pixmap; //!< Pixmap holding rendered workspace view
    };

    WorkspaceIndicator();
    virtual ~WorkspaceIndicator(void);

    void render(void);
    void updateHideTimer(uint timeout);

private:
    Display _display_wo; //!< Display winobj handling rendering of workspace status
    PDecor::TitleItem _title; //!< Title item added to the decor
};

#endif // _WORKSPACE_INDICATOR_HH_
