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
	PulseAudio.cpp \
	Shortcut.cpp \
	SettingsDialog.cpp \
	Synthesizer.cpp \
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
	AudioBackendDummy.h \
	AudioBuffer.h \
	Error.h \
	Generator.h \
	MainWindow.h \
	Message.h \
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
	SpinLock.h \
	Synthesizer.h \
	Ticker.h 

gmetronome_CPPFLAGS = @GTKMM_CFLAGS@ @PULSEAUDIO_CFLAGS@ -DPGRM_LOCALEDIR=\"${PGRM_LOCALEDIR}\"
gmetronome_LDFLAGS = @GTKMM_CFLAGS@ @PULSEAUDIO_CFLAGS@
gmetronome_LDADD = @GTKMM_LIBS@ @PULSEAUDIO_LIBS@

