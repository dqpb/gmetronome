bin_PROGRAMS = gmetronome

gmetronome_SOURCES = \
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
	Generator.cpp \
	main.cpp \
	MainWindow.cpp \
	Message.cpp \
	Meter.cpp \
	Profile.cpp \
	ProfilesIOBase.cpp \
	ProfilesIOLocalXml.cpp \
	ProfilesListStore.cpp \
	ProfilesManager.cpp \
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
	Generator.h \
	MainWindow.h \
	Message.h \
	Meter.h \
	Oss.h \
	Profile.h \
	ProfilesIOBase.h \
	ProfilesIOLocalXml.h \
	ProfilesListStore.h \
	ProfilesManager.h \
	PulseAudio.h \
	Settings.h \
	SettingsDialog.h \
	Shortcut.h \
	SpinLock.h \
	Synthesizer.h \
	Ticker.h 

top_src_datadir = $(top_srcdir)/data

gresource_xml_file = $(top_src_datadir)/gmetronome.gresource.xml

resources_file = resources.cpp

nodist_gmetronome_SOURCES = $(resources_file)

BUILT_SOURCES = $(resources_file)

gresources_deps = \
	$(top_src_datadir)/ui/ShortcutDialog.glade \
	$(top_src_datadir)/icons/scalable/gmetronome.svg \
	$(top_src_datadir)/ui/MainWindow.glade \
	$(top_src_datadir)/ui/SettingsDialog.glade \
	$(top_src_datadir)/css/global.css

$(resources_file): $(gresource_xml_file) $(gresources_deps)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) $(gresource_xml_file) \
		--target=$@ --sourcedir=$(top_src_datadir) --generate-source

gmetronome_CPPFLAGS = @GTKMM_CFLAGS@ @PULSEAUDIO_CFLAGS@ @ALSA_CFLAGS@ -DPGRM_LOCALEDIR=\"${PGRM_LOCALEDIR}\"
gmetronome_LDFLAGS = @GTKMM_CFLAGS@ @PULSEAUDIO_CFLAGS@ @ALSA_CFLAGS@
gmetronome_LDADD = @GTKMM_LIBS@ @PULSEAUDIO_LIBS@ @ALSA_LIBS@

CLEANFILES = $(resources_file)
