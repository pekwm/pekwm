[< Previous (Themes)](themes.md) - [(Development) Next >](development.md)

***

The pekwm FAQ
=============

This is the official pekwm FAQ. Here you can find answers to the
questions and cries for help frequently appearing in the pekwm IRC
channel and mailing lists.

If you can't find the answer to your question here, read the whole
thing again. Chances are you just missed it the first time around. If
you still can't find it, and you have gone through the documentation,
tried until your fingers bleed from typing, and started a hunger
strike against evil documentation projects that contain no helpful
advices for you, then you may ask your question in IRC or on the
mailing list.

Common questions and answers
============================

### How is this ~/.pekwm/start thing used?

The file _~/.pekwm/start_ is a regular shell script that is executed
when pekwm starts. The file needs to be chmodded as executable for it
to get used:

```
chmod +x ~/.pekwm/start
```

A simple example start file could look like this:

```
#!/bin/sh
gkrellm &
pekwm_bg -D -l ~/backgrounds mybackground.png &
```

Remember the &'s.

### What is the harbour and how is it used?

Harbour is pekwm's way of supporting dockapps, special small
applications that usually display things like system information or
the status of your email inbox. It's essentially the same thing you
might know as a dock or a wharf. The harbour is not a KDE/GNOME
systray for notification icons. If you want notification icons in the
harbour, you need to find a dockapp that does this for you.

If a dockapp doesn't go into the harbour even you have it enabled at
compile time, you should see if the application has an option to start
it "withdrawn".

### Can I have automatically changing menus in pekwm?

Yes. The Dynamic keyword is a way to use automatically generated menus
in pekwm. That is, menus that regenerate every time you view them. As
an example, by default the themes menu is dynamic.

See [Dynamic Menus](#config-menu-dynamic) for more information.

### How do I install themes?

The idea is to unpack/uncompress the theme file you downloaded into
some directory. In this case, we will unpack it to _~/.pekwm/themes_,
which is the standard location for user installed themes.

In simple, first make sure the themes directory exist, and if not,
make it by issuing the command:

```
mkdir ~/.pekwm/themes
```

Then copy the theme package, lets call it _theme.tar.gz_, into
_~/.pekwm/themes_. Then uncompress the theme pack with the appropriate
tool. Unpack the theme with:

```
gzip -dc theme.tar.gz | tar xvf -
```

You will then end up with a new subdirectory - this is the theme.

Since we uncompressed the theme in a standard location, after this you
can select the new theme from the themes menu. If you installed in a
non-standard location, you'll have to manually edit
_~/.pekwm/config_.

In the top of this file there is a section named _"Files {}"_. In this
section, there is a line that says something like:

```
Theme = "/usr/local/share/pekwm/themes/minimal"
```

Edit this line to point to the directory you installed the
theme. Restart pekwm and you're set.

### I upgraded pekwm and now ......... won't work!

pekwm has not yet achieved a freeze on it's configuration file
syntax. And as pekwm is an actively developed application, there
probably have been some changes on some part of the configuration.

If you encounter a situation that when you upgrade your pekwm, and
some thing just stops to work, you should either:

_Move your old configuration out of the way_ - Move your pekwm
configuration files out of _~/.pekwm_:

```
mv ~/.pekwm ~/old.pekwm**
```

This will result in new fresh configuration files being copied in. If
this helps, your configuration files weren't compatible with the new
version of pekwm.

_Check NEWS.md_ - If something configurable wise has been changed, it
has been documented in the NEWS.md for the release.This is a helpful
resource when you want to convert your old configuration files to a
newer configuration format.

_Look under the source trees data/ directory for reference_ - If you
can't find info about a new feature or for some reason you don't
understand the brief explanation on the wiki, there is a data/
directory in the source package of pekwm that has example
configuration files (these act as the default configuration on a new
install). Chances are you'll find help from there.

_Read the documentation._ - You can find links to up to date
documentation for your pekwm version at the pekwm homepage.

_Make sure the right executable is being executed._ - Locate all
instances of pekwm:

```
find / -name 'pekwm'
```

If you see many pekwm executables laying around, maybe one in
_/usr/bin_ and one in _/usr/local/bin_, you might be starting a wrong
version pekwm. This might happen when you for example, install a
premade pekwm package for your distribution and later install pekwm
from source yourself. The safe way is to remove all these pekwm
instances and either re-apply the package or do **make install** again
in the source. You can also, of course, go thru every pekwm binary
with the --version parameter to find the right executable to
keep. Note to give the full path to the executable when querying for
the version (**/usr/local/bin/pekwm --version**).

### Can I turn off this sloppy focus crap?

Yes. You can. You need to make all enter and leave events not affect
the focus of frames, borders, clients. See the comments above the
`$CLIENT_CLICK = "Focus"` line in _~/.pekwm/mouse_.

See [Mouse Bindings](actions.md#keys-and-mouse) for more info on the
mouse configuration file.

### What is Mod1? How about Mod4?

In the _~/.pekwm/keys_ and _~/.pekwm/mouse_ there are all these odd
Mod1 and Mod4 things used as modifier keys. It's simple - Mod1 is more
widely known as the Alt key, and Mod4 as the "windows key" found on
recent keyboards. Use **xev** to find out what names keys carry.

### Why do my terminals start the wrong size when grouped?

This is a very complicated issue in fact, and has to do with the way
terminals handle their resize actions. One way to bring at least some
help to this situation is to put **resize > /dev/null** in your
.bashrc or equal.

### Where can I find the current size/position of a window?

Use the command **xwininfo | grep geometry**.

### How do I bring up the window menu when the window has no decorations?

You press keys. The default keybinding for window menu is at the
moment first Ctrl+Mod1+M, then W (or Mod4+W for short). You can
specify your own keybinding for window menu at the _~/.pekwm/keys_
configuration file. See [Key Bindings](actions.md#keys-and-mouse) for
information on how to edit keybindings.

### The start file doesn't work!

```
chmod +x ~/.pekwm/start
```

Yes, this is a duplicate of the first FAQ entry. Just making sure we
never have to see this question in IRC anymore.

### How do I set a background/root/desktop image?

pekwm now comes with a background setting tool, _pekwm_bg_ which is
integrated with the themes and will set the background on theme load
if the theme has a backgorund specified.

pekwm_bg can be run independent of pekwm to set the background, see
the [pekwm_bg manual page](pekwm_bg.1.md) for more details.

### A theme I tested doesn't work!

pekwm is an ongoing process. This means the theme file format has gone
thru considerable amounts of changes. Some of these changes are
backwards compatible, some are not. You have hit a theme with too old
or too new theme syntax for your copy of pekwm. Nothing can be done
unless someone who knows the differences between theme formats owns
you a favour and agrees to edit it out for you.

pekwm shouldn't refuse to start again after a faulting theme test, but
you will usually see everything scrambled up. In this case you can
either try to select a new working theme from the menu or change the
theme used manually. This is done in _~/.pekwm/config_. Under the
Files-section, there is an entry named Theme, that points to your
current theme. It might look something like this:

```
Files {
	Theme = "/home/shared/.pekwm/themes/blopsus9-blaah"
}
```

Now, all you need to do is make the path point to any theme you know
is working. The default theme is usually a safe bet. After edited and
saved, (re)start pekwm.

### What desktop pagers work with pekwm?

For general use any NETWM compliant pager should
do. [IPager](http://www.useperl.ru/ipager/index.en.html),
[screenpager](http://zelea.com/project/screenpager/introduction.html),
rox-pager, fbpanel's pager, obpager, gai-pager, gnome's pager, kde's
pager, perlpanel's pager, netwmpager, and so on. Do report your
success stories with pagers not already mentioned here.

### How do I make submenus open on mouse over rather than when clicked?

You need to edit _~/.pekwm/config_. Open it in an editor and search
for the Menu section towards the end of the file. It should look
somewhat like this:

```
Menu {
	# Defines how menus act on mouse input.
	# Possible values are: "ButtonPress ButtonRelease DoubleClick Motion"
	# To make submenus open on mouse over, comment the default Enter,
	# uncomment the alternative, and reload pekwm.
 
	Select = "Motion"
	Enter = "ButtonPress"
	# Enter = "Motion"
	Exec = "ButtonRelease"
}
```

To make submenus open on mouse over, remove or comment out the line

```
    Enter = "ButtonPress"
```

and remove the # character from this line:

```
    # Enter = "Motion"
```

so that it looks like this:

```
    Enter = "Motion"
```

and you should be fine. Reload pekwm configuration from the menu or
press ctrl+mod1+delete to do it with a keybinding. Test your
semiautomatic pekwm menus!

The default requires you to click because of dynamic menus. While
reasonably fast, they can sometimes take a second or two to be
generated depending on the script behind it. Browsing the menu tree
can at such times become more annoying, specially on a slower machine,
than having to do that extra click. There you have it, our reasoning
and the solution for if you don't like it.

### My keyboard doesn't have the window keys, the default key bindings suck!

Probably the easiest way to use the default keybindings on a keyboard
that has no window keys is to assign some other key the Mod4
status. Since Caps Lock is one of the most unused and annoying keys
around, we'll make it the Mod4 modifier by adding the following lines
to _~/.pekwm/start_:

```
# Make Caps Lock act as Mod4
xmodmap -e "remove lock = Caps\_Lock"
xmodmap -e "add mod4 = Caps\_Lock"
```

Remember to:

```
chmod +x ~/.pekwm/start
```

Then restart pekwm. Your caps lock key should now act as Mod4. Oh joy.

### Where's my Unicode support?

Edit your favorite theme to use your preferred Unicode font, and enjoy
all kinds of characters on the titlebars and menus.

### Where did my titlebar buttons go?

Buttons require a name to be set when template based syntax is enabled
in themes or the last button will be the only visible one.

Using this syntax will create only one button:

```
Require {
 Templates = "True"
}

Buttons {
 Left {
   @button
 }
 Left {
   @button
 }
}
```

Given names, both buttons will be created:

```
Require {
 Templates = "True"
}

Buttons {
 Left = "button1" {
   @button
 }
 Left = "button2" {
   @button
 }
}
```

### How can I disable the workspace indicator popup?

In the main configuration file under the Screen section there should
be a ShowWorkspaceIndicator parameter, set this to 0 to disable the
WorkspaceIndicator.

### Why is nvidia dual-head misbehaving?

The nvidia proprietary driver provides dual head information both in
Xinerama and RANDR form. The RANDR information given by the driver is
in the form of one large screen, the Xinerama is divided up into
multiple heads. To work around this the HonourRandr option has been
introduced in the Screen section of the main configuration file. Set
it to False making pekwm only listen on the Xinerama information.

```
Screen {
    HonourRandr = "False"
```

### Mouse scrolling does not work with GTK3 applications?

You need to set the environment variable `GDK_CORE_DEVICE_EVENTS` to
1.

***

[< Previous (Themes)](themes.md) - [(Development) Next >](development.md)
