bin_PROGRAMS = gmetronome

gmetronome_SOURCES = \
	About.cpp \
	AccentButton.cpp \
	AccentButtonGrid.cpp \
	ActionBinding.cpp \
	Action.cpp \
	Application.cpp \
	Audio.cpp \
	AudioBackend.cpp \
	AudioBackendDummy.cpp \
	AudioBuffer.cpp \
	Auxiliary.cpp \
	Error.cpp \
	Filter.cpp \
	Generator.cpp \
	main.cpp \
	MainWindow.cpp \
	Message.cpp \
	Meter.cpp \
	Pendulum.cpp \
	Physics.cpp \
	Profile.cpp \
	ProfileIOBase.cpp \
	ProfileIOLocalXml.cpp \
	ProfileListStore.cpp \
	ProfileManager.cpp \
	Settings.cpp \
	SettingsDialog.cpp \
	Shortcut.cpp \
	SoundThemeEditor.cpp \
	Synthesizer.cpp \
	TapAnalyser.cpp \
	TempoDisplay.cpp \
	Ticker.cpp \
	Wavetable.cpp \
	WavetableLibrary.cpp

if HAVE_ALSA
gmetronome_SOURCES += Alsa.cpp
endif
if HAVE_OSS
gmetronome_SOURCES += Oss.cpp
endif
if HAVE_PULSEAUDIO
gmetronome_SOURCES += PulseAudio.cpp
endif

noinst_HEADERS = \
	About.h \
	AccentButton.h \
	AccentButtonGrid.h \
	ActionBinding.h \
	Action.h \
	Alsa.h \
	Application.h \
	Audio.h \
	AudioBackend.h \
	AudioBackendDummy.h \
	AudioBuffer.h \
	Auxiliary.h \
	Error.h \
	Filter.h \
	Generator.h \
	MainWindow.h \
	Message.h \
	Meter.h \
	ObjectLibrary.h \
	Oss.h \
	Pendulum.h \
	Physics.h \
	Profile.h \
	ProfileIOBase.h \
	ProfileIOLocalXml.h \
	ProfileListStore.h \
	ProfileManager.h \
	PulseAudio.h \
	Settings.h \
	SettingsDialog.h \
	SettingsList.h \
	Shortcut.h \
	SoundLibrary.h \
	SoundTheme.h \
	SoundThemeEditor.h \
	SpinLock.h \
	Synthesizer.h \
	TapAnalyser.h \
	TempoDisplay.h \
	Ticker.h \
	Wavetable.h \
	WavetableLibrary.h

resourcesdir = $(top_srcdir)/src/resources

resources = \
	$(resourcesdir)/ui/ShortcutDialog.glade \
	$(resourcesdir)/ui/MainWindow.glade \
	$(resourcesdir)/ui/SettingsDialog.glade \
	$(resourcesdir)/ui/SoundThemeEditor.glade \
	$(resourcesdir)/icons/gm-access-profile-symbolic.svg \
	$(resourcesdir)/icons/gm-access-accent-symbolic.svg \
	$(resourcesdir)/icons/gm-access-trainer-symbolic.svg \
	$(resourcesdir)/icons/gm-note-quarter-symbolic.svg \
	$(resourcesdir)/icons/gm-note-eighth-symbolic.svg \
	$(resourcesdir)/icons/gm-note-triplet-symbolic.svg \
	$(resourcesdir)/icons/gm-note-sixteenth-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-classic-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-modern-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-bell-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-timbre-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-crossfade-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-volume-muted-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-volume-zero-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-volume-low-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-volume-medium-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-volume-high-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-volume-full-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-balance-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-balance-left-symbolic.svg \
	$(resourcesdir)/icons/gm-snd-balance-right-symbolic.svg \
	$(resourcesdir)/icons/gm-curve-keep-symbolic.svg \
	$(resourcesdir)/icons/gm-curve-linear-up-symbolic.svg \
	$(resourcesdir)/icons/gm-curve-linear-down-symbolic.svg \
	$(resourcesdir)/icons/gm-curve-cubic-up-symbolic.svg \
	$(resourcesdir)/icons/gm-curve-cubic-up-flipped-symbolic.svg \
	$(resourcesdir)/icons/gm-curve-cubic-down-symbolic.svg \
	$(resourcesdir)/icons/gm-curve-cubic-down-flipped-symbolic.svg \
	$(resourcesdir)/icons/gm-curve-quartic-symbolic.svg \
	$(resourcesdir)/css/global.css

gresource_xml_file = resources/@PACKAGE_ID@.gresource.xml

resources_cpp_file = resources.cpp

nodist_gmetronome_SOURCES = $(resources_cpp_file)

BUILT_SOURCES = $(resources_cpp_file)

$(resources_cpp_file): $(gresource_xml_file) $(resources)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) $(gresource_xml_file) \
		--target=$@ --sourcedir=$(resourcesdir) --generate-source

gmetronome_CPPFLAGS = @GTKMM_CFLAGS@ @PULSEAUDIO_CFLAGS@ @ALSA_CFLAGS@ -DPGRM_LOCALEDIR=\"${PGRM_LOCALEDIR}\"
gmetronome_LDFLAGS = @GTKMM_CFLAGS@ @PULSEAUDIO_CFLAGS@ @ALSA_CFLAGS@
gmetronome_LDADD = @GTKMM_LIBS@ @PULSEAUDIO_LIBS@ @ALSA_LIBS@

EXTRA_DIST = $(gresource_xml_file) $(resources)

CLEANFILES = $(resources_cpp_file)
