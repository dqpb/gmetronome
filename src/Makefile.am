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
	TempoMarkings.cpp \
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
	TempoMarkings.h \
	Ticker.h \
	Wavetable.h \
	WavetableLibrary.h

resourcesdir = $(top_srcdir)/src/resources

resources = \
	$(resourcesdir)/ui/ShortcutDialog.glade \
	$(resourcesdir)/ui/MainWindow.glade \
	$(resourcesdir)/ui/SettingsDialog.glade \
	$(resourcesdir)/ui/SoundThemeEditor.glade \
	$(resourcesdir)/icons/access-profile-symbolic.svg \
	$(resourcesdir)/icons/access-accent-symbolic.svg \
	$(resourcesdir)/icons/access-trainer-symbolic.svg \
	$(resourcesdir)/icons/note-quarter-symbolic.svg \
	$(resourcesdir)/icons/note-eighth-symbolic.svg \
	$(resourcesdir)/icons/note-triplet-symbolic.svg \
	$(resourcesdir)/icons/note-sixteenth-symbolic.svg \
	$(resourcesdir)/icons/sound-classic-symbolic.svg \
	$(resourcesdir)/icons/sound-modern-symbolic.svg \
	$(resourcesdir)/icons/sound-bell-symbolic.svg \
	$(resourcesdir)/icons/sound-timbre-symbolic.svg \
	$(resourcesdir)/icons/sound-crossfade-symbolic.svg \
	$(resourcesdir)/icons/sound-volume-symbolic.svg \
	$(resourcesdir)/icons/sound-volume-low-symbolic.svg \
	$(resourcesdir)/icons/sound-volume-high-symbolic.svg \
	$(resourcesdir)/icons/sound-balance-symbolic.svg \
	$(resourcesdir)/icons/sound-balance-left-symbolic.svg \
	$(resourcesdir)/icons/sound-balance-right-symbolic.svg \
	$(resourcesdir)/icons/curve-keep-symbolic.svg \
	$(resourcesdir)/icons/curve-linear-up-symbolic.svg \
	$(resourcesdir)/icons/curve-linear-down-symbolic.svg \
	$(resourcesdir)/icons/curve-cubic-up-symbolic.svg \
	$(resourcesdir)/icons/curve-cubic-up-flipped-symbolic.svg \
	$(resourcesdir)/icons/curve-cubic-down-symbolic.svg \
	$(resourcesdir)/icons/curve-cubic-down-flipped-symbolic.svg \
	$(resourcesdir)/icons/curve-quartic-symbolic.svg \
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
