AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = gmetronome

gmetronome_SOURCES = \
	AccentButton.cpp \
	AccentButtonGrid.cpp \
	ActionBinding.cpp \
	Action.cpp \
	Application.cpp \
	Audio.cpp \
	AudioBackend.cpp \
	AudioBuffer.cpp \
	Generator.cpp \
	main.cpp \
	MainWindow.cpp \
	Meter.cpp \
	Profile.cpp \
	ProfilesIOBase.cpp \
	ProfilesIOLocalXml.cpp \
	ProfilesListStore.cpp \
	ProfilesManager.cpp \
	PulseAudio.cpp \
	Shortcut.cpp \
	SettingsDialog.cpp \
	Ticker.cpp

nodist_gmetronome_SOURCES = \
	../data/resources.cpp

noinst_HEADERS = \
	AccentButton.h \
	AccentButtonGrid.h \
	ActionBinding.h \
	Action.h \
	Application.h \
	Audio.h \
	AudioBackend.h \
	AudioBuffer.h \
	AudioSource.h \
	Generator.h \
	MainWindow.h \
	Meter.h \
	Profile.h \
	ProfilesIOBase.h \
	ProfilesIOLocalXml.h \
	ProfilesListStore.h \
	ProfilesManager.h \
	PulseAudio.h \
	Settings.h \
	SettingsDialog.h \
	Shortcut.h \
	Ticker.h 

gmetronome_CPPFLAGS = @GTKMM_CFLAGS@ @PULSEAUDIO_CFLAGS@ -DPGRM_LOCALEDIR=\"${PGRM_LOCALEDIR}\"
gmetronome_LDFLAGS = @GTKMM_CFLAGS@ @PULSEAUDIO_CFLAGS@
gmetronome_LDADD = @GTKMM_LIBS@ @PULSEAUDIO_LIBS@

