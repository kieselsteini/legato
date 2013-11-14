Legato
======

A Lua/Allegro5 based gaming framwork

What is Legato?
===============

Take the famous scripting language Lua and bind it together with the powerful
game framework Allegro. The result is a very flexible and fun to work with
framework called Legato. So in short:

Legato = Lua + Allegro

The idea behind Legato is, to ship only a single executable which contains
a Lua interpreter, Allegro 5 game framework, PhysFS virtual filesystem and
the robust UDP networking library enet. If you write the game on one system,
it will work on every system which is supported by Legato.


What is Legato not?
===================

Legato is a framework which provides various functions and objects to create
multimedia applications and games. It's not a ready to run game engine or
a full featured 3D engine. You have to implement everything yourself.

What's implemented so far?
==========================

Almost every function and module of Allegro 5. There are only a few things,
which are not implemented right now. All function calls for Allegro 5 have
the same name and arguments, so take a look to
http://alleg.sourceforge.net/a5docs/5.0.10/index.html and compare it to the
list below.

Allegro 5
---------
* Configuration files - implemented
* Display             - implemented (some icons stuff is missing)
* Events              - implemented (Joystick stuff is missing)
* File I/O            - not implemented (use PhysFS instead)
* Filesystem          - not implemented (use PhysFS instead)
* Fixed point math    - not implemented (Lua uses numbers which are implemented as doubles -> makes no sense)
* Fullscreen modes    - implemented
* Graphics            - implemented (bitmap locking is missing right now)
* Joystick            - not implemented (I was lazy...should be done!)
* Keyboard            - implemented (Allegro 5 key symbols are missing)
* Memory              - not implemented (Lua uses a dynamic memory management -> makes no sense)
* Monitor             - implemented (some bits missing)
* Mouse               - implemented
* Path                - implemented
* State               - implemented
* System              - implemented (some bits missing)
* Threads             - not implemented (and probably will never be...)
* Time                - implemented (timeout missing)
* Timers              - implemented
* Transformations     - implemented
* UTF-8               - not implemented (Lua has strings...)
* Miscellaneous       - not implemented (nothing of value here)
* Platform-specific   - not implemented
* Direct3D            - not implemented
* OpenGL              - not implemented

Allegro 5 - Addons
------------------
* Audio addon         - implemented (with audio codecs)
* Color addon         - implemented (missing some color to hsl etc.)
* Font addons         - implemented (no way to create bitmap fonts right now)
* Memfile addon       - not implemented
* Native dialogs      - not implemented (this will add even more dependencies on Linux/Unix)
* PhysicsFS addon     - implemendted (Legato forces the use of PhysFS)
* Primitives addon    - implemented (low level drawing functions missing)

PhysicsFS
---------
Nearly all functions are implemented.
Manual is here: http://www.icculus.org/physfs/docs/html/

enet (UDP networking)
---------------------
Not functional at all. Only stubs implemented so far.
Manual is here: http://enet.bespin.org/

legato.core (some core functionality)
-------------------------------------
* get_version()
* get_version_string()
* load_script(filename)
* encode_UTF8_codepoint(codepoint)
* get_UTF8_length(string)
* split_UTF8_string(string)

How to use?
===========

Hmm. Compile legato.c on your computer (you will need Allegro 5+dependencies, PhysFS,
Lua and enet). If you don't know how to do this, ask a nice person to do it. You can
use the executable for every game you going to create.

Next step. Create a directory called "data" in the directory of your executable.

Fire up your favourite editor and create a file in the "data" directory called
"boot.lua".

Write some lines like:

```Lua
local al = legato.al -- improve speed
local display = al.create_display(800, 600)
local event_queue = al.create_event_queue()
al.register_event_source(event_queue, display)

while true do
    local event = al.get_next_event(event_queue)
    if event then
        if event.type == 'display_close' then
            return -- stop running if display get's closed
        end
    end
    al.clear_to_color(al.map_rgb(0, 0, 0))
    al.flip_display()
    al.rest(0.1)
end
```

Sorry there's no real manual yet. I'm going to create some in the future :(


Bugs ?!
=======

There may be many of them. I just started to convert some examples over to
Legato. If you find something please don't hesitate and write me a mail.


Last famous words...
====================

This project is still work in progress. But you can still use a lot of things
right now. Prototyping a small game is very very efficient with that framework.
Please feel free to modify and improve this code, share it with your friend
and do whatever you want (even exploit it commercially if you like).
But it would be nice to hear from you and let me see, what amazing stuff you
going to create with it.

Thanks!
Have fun and happy hacking!

Sebastian Steinhauer <s.steinhauer@yahoo.de>
