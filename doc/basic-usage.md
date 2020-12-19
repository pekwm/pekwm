II. Basic Usage
===============

Pekwm is a fast, functional, and flexible window manager. Here's some notes on how to operate it on it's default settings.

**Table of Contents**

4\. [Getting Started](#usage-gettingstarted)

5\. [Window Grouping](#usage-grouping)

6\. [Workspaces](#usage-workspaces)

* * *

Chapter 4. Getting Started
==========================

Now that you have pekwm installed, you should take a few moments to test how the basics work.

The documentation generally tries to explain the terms it uses, but useful terms to know beforehand include "Mod1" which usually means your Alt key, and "Mod4" which refers to the "windows key" found on recent keyboards.

It's also good to know that a "frame" basically means the same as a window, but this window can contain one or more real windows. The same concept is also referred as a "window group". In relation to this, window inside such a frame can be referred as a "grouped window" or a "client window", or simply just as a "client".

* * *

#### 4.1. First Run

The first time you run pekwm, the following files should be copied into your ~/.pekwm directory: config, menu, keys, autoproperties, start, vars. You will learn more about these files in the [Configuration](#config) section.

All this happens behind-the-scenes, so by default, you'll be placed into a nice working environment.

* * *

#### 4.2. About Menus and Iconification

When you iconify (This is the traditional name in unix. Windows calls this minimizing.) a window in pekwm, it doesn't really go anywhere like you might expect. You can de-iconify using one of three menus: The Icon menu, the Goto menu, or the GotoClient menu. When you click on an item in one of these menus, it takes you to that window deiconifying when necessary.

Icon menu shows you a list of all currently iconified windows. Use Mod4+Shift+I to bring it up.

Goto menu shows you a list of windows currently active. This menu will only show the currently active window of possible window groups. Use Mod4+L, or middle click of the mouse on the root window or screen edges to bring this menu up.

GotoClient menu shows you a list of every window currently open. Window groups are separated from each other with a menu separator which is defined by the currently used theme, usually a line of some sort. You need to be using window grouping to really see any difference between the GotoClient and Goto menus. Use Mod4+C, or middle click of the mouse on the root window or screen edges while holding down Mod4 to bring up the GotoClient menu.

An item in the goto and gotoclient menu and icon menu (and attach menus) has the following syntax:

<number> \[symbols\] Window title

The number represents what workspace the window is on. Symbols is a list of symbols that represent window states. They are:

**Symbols in the pekwm menus**

*   \* (sticky)
    
*   . (iconified)
    
*   ^ (shaded)
    
*   \+ (above normal window layer)
    
*   \- (below normal window layer)
    
*   A (active in group)
    

> If you are using window grouping, the whole group will iconify instead of one window. Please ungroup before minimizing if you wish to iconify a single client window from a group frame.

* * *

#### 4.3. Using the mouse

Pekwm has excellent mouse support. Here you'll learn how to do some usual window management actions using the default configuration.

Moving windows is rather easy and I think you already got the hang of using the left mouse button on the titlebar and dragging. But did you notice that when you press Mod1 while dragging on the client window (not the titlebar) it works just as well.

Resizing is also easy and most are familiar with it. Hang on to a border of the window with the left mouse button and drag. Release the button and you're done. But what you likely didn't know is that if you press Mod1 and then drag on the client window with the right mouse button it also makes windows resize. Try it, it's great.

Minimizing (iconifying) with the mouse is possible thru the window menu. Right click on a windows titlebar and select Iconify. Many themes also implement a iconifying button on the titlebar. Also see [About Menus and Iconification](#usage-gettingstarted-icons).

Shading is done by double clicking the titlebar with the middle mouse button. Unshade doing it again.

Maximizing is quite easy. Hold Mod1 and double click with the left button. Many themes also have a maximize button in them. The default theme has one on the right corner of the titlebar. It's also possible to use the window menu (right click on titlebar).

Filling. Sounds odd? Its not. It just means you can make a window grow as large as it can until it hits the borders of the windows surrounding it. Easy as pie, double click on the titlebar with the left mouse button. Excellent feature you are likely to grow to like.

Raising windows. Easy. Left click on the windows titlebar or hold Mod1 and left click anywhere on the window.

Lowering windows. Hold Mod4 and left click anywhere on the window.

Closing. Most themes implement a close button. Default theme has one on the left end of the titlebar. You can also close a client by holding Mod4 and right clicking on its title. Note that the client doesn't have to be the active client of the frame for this to work. Also see the window menu.

Grouping. Middle click and drag on a titlebar and release over the frame you want the window into. Holding Mod1 and middle clicking works on the whole client window. This process can also be automated, more on that later.

Activating clients. Now that you have multiple clients grouped into one frame, you can switch between them simply by left clicking on the clients title. Doing so also raises the frame. If you don't want the frame to raise, middle click on the clients title. Also try turning the mouse wheel on a frames titlebar when it has more than one client.

Menus. As mentioned, press the right mouse button on a windows titlebar and you get the window menu. You can do lots of things from there that are not possible by mouse shortcuts. To bring up the root menu (the one you use to launch programs) click the right button on the background or on the screen edges. To get the Goto menu, click the middle button on the background or screen edges.

Most theme buttons work with a left click. Some also have specials when you use other mouse buttons on them. Like the default themes maximize button. Try it. The default themes close button also has a special when you right click on it. With it it is possible to kill the client if it's so stuck you can't close it normally.

That ends our short introduction to using the mouse in pekwm. Hope you found the defaults pleasant to use. Remember that if you didn't like something, you can change it. See [Mouse Bindings](#config-keys_mouse-mouse) for how.

* * *

#### 4.4. Using the keyboard

Pekwm allows excellent keyboard control of your window management. Lets try it out a bit. If you don't have the windows key on your keyboard, please see ~/.pekwm/keys for the keychains you can use to do the same and a lot more.

Moving and Resizing windows. To be able to move and resize windows you have to activate the special MoveResize state. This happens by pressing Mod4+Enter. The window should after this be movable by using the arrow keys. To resize, press Mod4 and use the arrows. Using the Shift-key with these actions makes them be careful. To accept the new size and position, press Enter. To fail back to the old position and size press Escape.

Minimizing. Press Mod4+I. Mod4+Shift+I pops up the icon menu you can use to bring it back.

Shading. This is to hide most of the window, leaving only the titlebar visible. Press Mod4+S to toggle the shaded state.

Maximizing. Mod4+M toggles the maximized state.

Filling (making a window grow as big as it can in the space it has around it). Press Mod4+G to make windows grow to fit.

Fullscreen. Press Mod4+F to toggle the fullscreen state.

Moving between frames. Press Mod1+Tab and Mod1+Shift+Tab to move between frames. Or use Mod1+Ctrl+Tab and Mod1+Ctrl+Shift+Tab to move between most recently used frames. You can also use directional focusing. Press Mod4 and one of the arrow keys. The focus should change to the frame that is in the direction you pointed to. Try it out.

Moving inside frames. Press Mod4+Tab and Mod4+Shift+Tab to move between the clients in a frame.

Closing. Press Mod4+Q to close windows.

Grouping. The easiest way to group is to use marking. You select clients you want to group to another frame by toggling them marked with Mod4+Z. You can have as many marked clients as you wish. Then go to the frame you want those now marked clients to be attached and press Mod4+A. That's it.

Menus. There are some simple menu bindings. Mod4+R shows your main menu (the Root menu). Mod4+L shows a list of your active windows (the Goto menu). Mod4+C shows a list of all your open windows (the Goto menu). Mod4+W brings up the Window menu. And Mod4+Shift+I the Icon menu.

Those were the basics. There's a ton more. See the rest of the documentation for rest of the simple bindings and ~/.pekwm/keys for a list of the keychains. And again, if you hated something, go ahead and edit it.

* * *

Chapter 5. Window Grouping
==========================

The main feature of pekwm is window grouping, similar to that of ion, or the tabs in pwm or fluxbox.

* * *

#### 5.1. What is window grouping?

Window grouping is a very simple concept, but it could be hard to understand at first. It's a simple way of making multiple applications share the exact same space.

The simplest way to explain this is with an analogy. Imagine you have 20 sheets of paper. To save space, you stack them on top of each other. then, you have little tabs sticking out of one edge so you can quickly flip to any sheet of paper.

You have likely stumbled upon a WWW-browser that calls this tabbing. In pekwm, Window grouping is visually done by dividing up the physical space of the titlebar. We don't call them tabs for historical reasons, but refer to them as "clients". Windows that can contain any number of clients are more than often referred as "frames".

Also note that a frame can contain any type of clients. If you want to group one of your WWW-browser windows with your text editor for future reference, you're free to do so.

* * *

#### 5.2. How window grouping works

The first thing to know is how to group one window to another. Middle-Click (On a normal X setup, the 2nd mouse button is the middle button) the titlebar of the first window and begin to drag the window. You should now see a rectangle with the window's title in it. Drag that rectangle to above the target window, and release your mouse button.

> Any time this document mentions a key or mouse button, there's a strong likelihood that you can change which key or mouse button is used for that function. Please see the [Keyboard and Mouse config](#config-keys_mouse) section.

Now that you have windows in a group, you need to learn to choose between windows in that group. The first way is by clicking the middle mouse button on the window's part of the titlebar. That window should now be the currently-active window of the group. You can also use a keybinding for this. The default keybindings are Mod4+Tab and Mod4+Shift+Tab to go forward and back between active window in the frame.

To de-group, simply middle click and drag the window off the frame, and release anywhere. If you release over another window group, you'll move the window to the new group. Default keybinding for detaching clients from a group is first Ctrl+Mod1+T then D.

You can also set windows up to automatically be grouped to one another. See the [Autoproperties](#config-autoprops) section for more details.

* * *

#### 5.3. Advanced Grouping Topics

Another thing you can do with window grouping is Tagging. This is done by setting the toggleable attribute "tagged" on a frame with the action "Set Tagged". A tag is like a miniature autogroup. It says "All new windows launched should be automatically grouped to this Frame" and all other autogrouping defined in the autoproperties will be ignored while it is set. "UnSet Tagged" removes the tag. Default keybinding to toggle tagging on a frame is Ctrl+Mod1+T then T. Unsetting tagging works even if the window you have set tagged isn't active. It's default keybinding is Ctrl+Mod1+T then C.

You can toggle all autogrouping on and off with the toggleable attribute GlobalGrouping. To disable you need to use the action "Unset GlobalGrouping" and to enable autogrouping use "Set GlobalGrouping". The default keybinding that toggless between set and unset is Ctrl+Mod1+T then G.

You can set a marked state on clients with "set marked" and then attach those marked clients to another frame by focusing the frame you want the marked clients attached to and then using the AttachMarked action. By default marking can be reached with two simple keybindings. Mod4+Z toggles a clients Marked state and Mod4+A attaches clients with marked state set into the current frame. Marked clients will have "\[M\]" appended to their titlebars.

Pekwm also includes some menus that have to do with grouping. AttachClientInFrame (Ctrl+Mod1+M, A) sends the current client to the selected frame. AttachFrameInFrame (Ctrl+Mod1+M, F) sends the contents of the current frame to the selected frame. AttachClient (Ctrl+Mod1+M, Shift+A) brings the selected client into the current frame. AttachFrame (Ctrl+Mod1+M, Shift+F) brings the contents of the selected frame into the current frame.

* * *

Chapter 6. Workspaces
=====================

Workspaces in pekwm are a very common feature, found in almost every UNIX window manager in existence. They're also called desktops in some window managers. In Pekwm-Speak, "workspace", "desktop", and "desk" are interchangeable. Use whichever one you feel like using.

> By default, pekwm enables four workspaces. You can change this by editing your ~/.pekwm/config file. See [The main config file](#config-configfile) section for more details.

* * *

#### 6.1. Workspace Navigation

You can send windows to another workspace by right-clicking the titlebar, going to 'send to' and picking the desktop you'd like. Another option is using the SendToWorkspace keybindings (by default, Mod4 and one of F1, F2, F3, or F4). Using the mouse to drag a window over the right or left screen edge makes it move to the next or previous workspace. Also try placing the mouse pointer on a client window and rotating the mouse wheel while holding Mod1 down to send a window to the next or previous workspace and follow it there yourself.

Switch desktops by using the GoToWorkspace keybindings (by default Mod4 and one of 1,2,3, or 4), or the "GotoWorkspace Next" and "GotoWorkspace Prev" actions (by default Ctrl+Mod1+Right and Ctrl+Mod1+Left). Holding Mod1 key while moving the mouse pointer over the right or left screen edge will make you move to the next or previous workspace. Also pressing the left mouse button on the right or left screen edge will make you move to the next or previous workspace. Using the mouse wheel on the background or the screen edges also changes your workspace.