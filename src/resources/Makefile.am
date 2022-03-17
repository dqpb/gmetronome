noinst_LIBRARIES = resources.a

gresource_xml_file = gmetronome.gresource.xml

resources_file = resources.cpp

gresources_deps = \
	ui/ShortcutDialog.glade \
	ui/MainWindow.glade \
	ui/SettingsDialog.glade \
	icons/note-quarter-symbolic.svg \
	icons/note-eighth-symbolic.svg \
	icons/note-triplet-symbolic.svg \
	icons/note-sixteenth-symbolic.svg \
	icons/profile-access-symbolic.svg \
	css/global.css

$(resources_file): $(gresource_xml_file) $(gresources_deps)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) $(gresource_xml_file) \
		--target=$@ --sourcedir=. --generate-source

nodist_gmetronome_SOURCES = $(resources_file)

BUILT_SOURCES = $(resources_file)

resources_a_SOURCES = $(resources_file)

CLEANFILES = $(resources_file)
