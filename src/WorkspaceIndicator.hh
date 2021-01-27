//
// WorkspaceIndicator.hh for pekwm
// Copyright (C) 2009-2020 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

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
    /** Display winobj handling rendering of workspace status */
    Display _display_wo;
    PDecor::TitleItem _title; //!< Title item added to the decor
};
