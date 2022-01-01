[< Previous (FAQ)](faq.md)

***

Development
===========

Interested in pekwm? Want to keep up with the development? Good, we
can tell you how.

**Table of Contents**

1. [IRC](#irc)
1. [Issue Tracker](#issue-tracker)
1. [The developers](#the-developer)

IRC
---

Our IRC channel #pekwm is located on [Libera.Chat](https://libera.chat).
Join if you want to ask questions, share thoughts and ideas or idle as much as
you like.

The following is a "do not" list. If some idiotic behaviour like,
public away messages, or nick changing as a way of telling what you
are doing, are not mentioned it doesn't mean you can do them.

As it seems to be a problem for some, do not ask questions in IRC
unless you are willing to listen to the answer. If listening is your
problem, you need other help than what we at IRC can provide.

If you have a question, ask it. It doesn't help to go around shouting
"can anyone help me". Help you with what? This will help the right
people to take initial contact with you, saving your time and adding
less clutter to the channel.

For same reasons, don't ask if we are alive or other meaningless
questions leading to the real question. Don't ask if someone specific
is around either. This is not the pager channel for you and your
friends.

Do not ask from one person. The fact is, even how much you think he or
she must be the only one who knows the answer, you are wrong. There's
at least a handful of people lurking on the channel who could help
you.

Read the documentation and FAQ before you ask anything. Doing this is
common courtesy towards the people who answer your questions. Not
reading at least the FAQ will be considered rude.

When asking a question, the answer more than often depends on the
version of pekwm you use. Showing us the information the command
**pekwm --info** gives will help us help you.

Do not expect people to answer your questions during the two minutes
you stay in the channel. We can't sit there all the time waiting for
your questions. If you can't wait longer, think about how stressful
your life is, take a break, go on a vacation, take it easy for a
while.

For similar reasons, don't repeat yourself. We saw it the first
time. Repeating easily gets you ignored completely so it eats its own
purpose. It's also considered rude, much like cutting in line.

If people tell you to read the documentation, they have a good reason
to do so. If they include an URL that helps you find the info quicker,
offer them your firstborn as a payback.

Don't offend anyone. They will offend you back two times.

Nicks with excessive capital letters will be hunted down and shot on
sight. You might as well use a proper nick to start with to avoid
this.

You can check the [developers](#devel-who) section to see the list of
developers mapped to their IRC nicks.

As with all IRC channels, it's best to not do anything too drastic
before keeping an eye for a while on how the channel works. We won't
mind even if you just idle there, there are long standing traditions
on that.

Welcome aboard!

Issue Tracker
-------------

pekwm bugs, feature requests and what-not are filed on Github issue
tracker available at https://github.com/pekdon/pekwm/issues

When reporting a bug or other misbehavior please provide as much
information as possible, including but not limited to:

1. How is the issue reproduced?
2. What did you expect to happen, what did actually happen?
3. Version of pekwm and other applications used.
4. Output of pekwm --info

### Gathering pekwm log files

To provide more details it is possible to run pekwm with the log level
set to trace and log to file. If the issue appears during start of
pekwm, start pekwm like this:

```
pekwm --log-file issue.log --log-level trace
```

If the issue is reproducible while pekwm is running, it is recommended
to set the log level and log file while reproducing the issue to avoid
including potential misleading information.


Run the following commands using the CmdDialog or pekwm_ctrl before
reproducing the issue.

```
Debug level trace
Debug enable logfile
Debug logfile issue.log
```

And directly after the issue has been reproduced:

```
Debug disable logfile
```

Include the issue.log in the error report if it includes any
information.


### Gathering information about a pekwm crash

If pekwm crash please provide a stack trace from the core dump, if no
core (or pekwm.core). To ensure a core file is generated enable core
dumps before starting pekwm:

```
ulimit -c unlimited
exec pekwm
```

If a core file has been created, generate a backtrace by running:

```
$ gdb /path/to/pekwm core
(gdb) bt
#0 ...
(gdb)
```

The output between the two _(gdb)_ lines should be included in the
report.

The developers
--------------

Below is a list of pekwm developers and what they do. The email
address is more of an informational thing than anything else; the
developers all subscribe to the + [pekwm mailing
list](http://pekwm.org/projects/pekwm/mailing_lists/15), so you should
email that instead.

### Developers

Claes Nästén (aka pekdon) <pekdon@gmail.com>

Main developer- Writes code. Has a serious screenshot fetish.

### Past devekopers

Andreas Schlick (aka ioerror)

Maintainer.

Jyri Jokinen (aka shared) <shared@adresh.com>

Writes documentation. Acts ill-tempered. You have been warned.

***

[< Previous (FAQ)](faq.md)
