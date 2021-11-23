GMetronome
==========
A practical and precise tempo measurement tool for composers and musicians. 

![Screenshot](data/screenshots/screenshot.png)

Features
--------
* Tempo range from 30 to 250 beats per minute
* Custom accentuation patterns
* Divisible beats for simple and compound meters
* Three accentuation levels for strong, middle and weak accents
* Pre defined patterns for widely used meters and time signatures
* Accurate trainer function to smoothly raise or lower tempo
* Profiles to (automatically) save and restore metronome settings
* Customizable keyboard shortcuts for nearly all metronome functions
* Support for different audio backends 

Download, building and installation
-----------------------------------
GMetronome is currently under development. It is written for GNU/Linux and
other UN*X like operating systems and utilizes the C++ language binding for
GTK 3 (https://gtkmm.org).

It depends on the following packages:

* gtk+-3.0
* gtkmm-3.0
* pulseaudio (optional)
* alsa (optional)

Since there is no official release yet, clone the project's repository:

```
$ git clone https://gitlab.gnome.org/dqpb/gmetronome.git
```

Change to the gmetronome directory and call autoreconf to generate the makefiles. 
This requires a working autotools (autoconf, automake) installation to succeed.

```
$ cd gmetronome
$ autoreconf --install
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
Bugs should be reported on the project's [issues page](https://gitlab.gnome.org/dqpb/gmetronome/issues/new).
Please read the GNOME [bug reporting guidelines](https://wiki.gnome.org/Community/GettingInTouch/BugReportingGuidelines).
