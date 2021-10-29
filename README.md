GMetronome
==========
A practical and precise tempo measurement tool for composers and musicians. 

![Screenshot](data/screenshots/screenshot.png)

Features
--------
* Tempo range from 30 to 250 beats per minute
* Custom accentuation patterns
* Animated markers to highlight current position in the accentuation pattern
* Divisible beats for simple and compound meters
* Pre defined patterns for widely used meters (e.g. simple triple meter) and 
  time signatures
* Configurable accentuation levels for strong, middle and weak accents
* Accurate trainer function to smoothly raise or lower tempo
* Profiles to (automatically) save and restore metronome settings
* Customizable keyboard shortcuts for nearly all metronome functions

Download, building and installation
-----------------------------------
GMetronome is currently under development. It is written for GNU/Linux and
other UN*X like operating systems and utilizes the C++ language binding for
GTK 3 (https://gtkmm.org).

It depends on the following packages:

* gtk+-3.0
* gtkmm-3.0
* pulseaudio

Since there is no official release yet, clone the projects repository:

```
$ git clone https://gitlab.gnome.org/dqpb/gmetronome.git
```

Now change to the gmetronome directory and generate the makefiles. 
The autogen.sh script requires a working autotools (autoconf, automake) 
installation to succeed.

```
$ cd gmetronome
$ NOCONFIGURE=1 ./autogen.sh
```

Then run the traditional GNU triplet:

```
$ ./configure
$ make

[ become super user if necessary ]
$ make install
```

See [INSTALL](INSTALL) for further details.

How to report bugs
------------------
Bugs should be reported on the projects [issues page](https://gitlab.gnome.org/dqpb/gmetronome/issues/new).
Please read the GNOME [bug reporting guidelines](https://wiki.gnome.org/Community/GettingInTouch/BugReportingGuidelines).