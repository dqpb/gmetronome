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
	Error.cpp \
	Filter.cpp \
	Generator.cpp \
	main.cpp \
	MainWindow.cpp \
	Message.cpp \
	Meter.cpp \
	Pendulum.cpp \
	Profile.cpp \
	ProfileIOBase.cpp \
	ProfileIOLocalXml.cpp \
	ProfileListStore.cpp \
	ProfileManager.cpp \
	Settings.cpp \
	SettingsDialog.cpp \
	Shortcut.cpp \
	Synthesizer.cpp \
	Ticker.cpp

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
	Error.h \
	Filter.h \
	Generator.h \
	MainWindow.h \
	Message.h \
	Meter.h \
	Oss.h \
	Pendulum.h \
	Profile.h \
	ProfileIOBase.h \
	ProfileIOLocalXml.h \
	ProfileListStore.h \
	ProfileManager.h \
	PulseAudio.h \
	Settings.h \
	SettingsDialog.h \
	Shortcut.h \
	SpinLock.h \
	Synthesizer.h \
	Ticker.h

resourcesdir = $(top_srcdir)/src/resources

resources = \
	$(resourcesdir)/ui/ShortcutDialog.glade \
	$(resourcesdir)/ui/MainWindow.glade \
	$(resourcesdir)/ui/SettingsDialog.glade \
	$(resourcesdir)/icons/note-quarter-symbolic.svg \
	$(resourcesdir)/icons/note-eighth-symbolic.svg \
	$(resourcesdir)/icons/note-triplet-symbolic.svg \
	$(resourcesdir)/icons/note-sixteenth-symbolic.svg \
	$(resourcesdir)/icons/profile-access-symbolic.svg \
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
