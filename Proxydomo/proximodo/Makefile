CC = g++
WXCFG = wx-config

# Change to 1 for debug build with no optimization
DEBUG = 0
ifeq ($(DEBUG),0)
WXINC = $(shell $(WXCFG) --cxxflags)
WXLIB = $(shell $(WXCFG) --libs)
CFLAGS = -Wall -g -O2
LDFLAGS =
else
WXINC = $(shell $(WXCFG) --cxxflags --debug)
WXLIB = $(shell $(WXCFG) --libs --debug)
CFLAGS = -Wall -g -O0 -fno-default-inline
LDFLAGS =
endif

# Uncomment these for profiling with gprof
#CFLAGS += -pg
#LDFLAGS += -pg

SRC = configscreen.cpp controls.cpp descriptor.cpp editscreen.cpp events.cpp \
	expander.cpp filter.cpp filterowner.cpp giffilter.cpp headerfilter.cpp \
	log.cpp logframe.cpp main.cpp mainframe.cpp managerthread.cpp \
	matcher.cpp nodes.cpp platform.cpp proxy.cpp requestmanager.cpp \
	settings.cpp settingsscreen.cpp testframe.cpp textbuffer.cpp \
	textfilter.cpp trayicon.cpp url.cpp util.cpp welcomescreen.cpp \
	windowcontent.cpp zlibbuffer.cpp
OBJ = $(SRC:.cpp=.o)
HDR = configscreen.h const.h controls.h descriptor.h editscreen.h events.h \
	expander.h filter.h filterowner.h giffilter.h headerfilter.h \
	logframe.h log.h logviewer.h mainframe.h managerthread.h matcher.h \
	memory.h node.h nodes.h platform.h Proximodo_private.h proxy.h \
	receptor.h requestmanager.h settings.h settingsscreen.h testframe.h \
	textbuffer.h textfilter.h trayicon.h url.h util.h welcomescreen.h \
	windowcontent.h zlibbuffer.h
PROXIMODO = distrib/proximodo

.phony: default all depend clean

default: all

all: $(PROXIMODO) tags

depend: $(SRC) $(HDR)
	makedepend -Y $(SRC)

clean:
	rm -f $(OBJ) $(PROXIMODO) tags

$(PROXIMODO): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(OBJ) $(WXLIB) -lz

.cpp.o:
	$(CC) $(CFLAGS) $(WXINC) -c -o $@ $<

tags: $(SRC) $(HDR)
	ctags -o $@ $(SRC) $(HDR)

# DO NOT DELETE THIS LINE -- make depend depends on it.

configscreen.o: configscreen.h windowcontent.h descriptor.h mainframe.h
configscreen.o: settings.h const.h util.h proxy.h testframe.h controls.h
configscreen.o: editscreen.h images/box_on.xpm images/box_off.xpm
configscreen.o: images/box_half.xpm images/btn_addfold32.xpm
configscreen.o: images/btn_addfilt32.xpm images/btn_trash32.xpm
configscreen.o: images/btn_edit32.xpm images/btn_ok32.xpm
configscreen.o: images/btn_undo32.xpm
controls.o: util.h controls.h events.h
descriptor.o: descriptor.h matcher.h util.h settings.h
editscreen.o: editscreen.h matcher.h settings.h descriptor.h const.h util.h
editscreen.o: controls.h testframe.h images/icon32.xpm
events.o: events.h
expander.o: expander.h url.h memory.h const.h util.h settings.h descriptor.h
expander.o: matcher.h log.h events.h logframe.h filter.h filterowner.h
filter.o: filter.h memory.h log.h events.h
filterowner.o: filterowner.h url.h util.h
giffilter.o: giffilter.h receptor.h
headerfilter.o: headerfilter.h filter.h memory.h url.h expander.h util.h
headerfilter.o: log.h events.h descriptor.h filterowner.h matcher.h
log.o: log.h events.h const.h trayicon.h logframe.h mainframe.h
logframe.o: logframe.h images/icon32.xpm util.h log.h events.h const.h
logframe.o: settings.h descriptor.h controls.h
main.o: proxy.h const.h settings.h descriptor.h mainframe.h util.h log.h
main.o: events.h trayicon.h
mainframe.o: mainframe.h settings.h descriptor.h proxy.h log.h events.h
mainframe.o: util.h const.h platform.h welcomescreen.h windowcontent.h
mainframe.o: settingsscreen.h configscreen.h logframe.h trayicon.h
mainframe.o: images/btn_settings.xpm images/btn_log.xpm images/btn_config.xpm
mainframe.o: images/btn_help.xpm images/btn_monitor.xpm images/btn_quit.xpm
mainframe.o: images/icon32.xpm
managerthread.o: managerthread.h log.h events.h requestmanager.h receptor.h
managerthread.o: filterowner.h url.h
matcher.o: matcher.h settings.h descriptor.h nodes.h node.h memory.h const.h
matcher.o: util.h url.h log.h events.h filter.h filterowner.h
nodes.o: nodes.h node.h memory.h const.h util.h settings.h descriptor.h
nodes.o: expander.h log.h events.h logframe.h url.h matcher.h filter.h
nodes.o: filterowner.h
platform.o: platform.h
proxy.o: proxy.h util.h settings.h descriptor.h events.h log.h platform.h
proxy.o: managerthread.h requestmanager.h receptor.h filterowner.h url.h
requestmanager.o: requestmanager.h receptor.h filterowner.h url.h util.h
requestmanager.o: settings.h descriptor.h log.h events.h const.h matcher.h
requestmanager.o: giffilter.h headerfilter.h filter.h memory.h textbuffer.h
requestmanager.o: zlibbuffer.h
settings.o: settings.h descriptor.h util.h const.h matcher.h nodes.h node.h
settings.o: memory.h
settingsscreen.o: settingsscreen.h windowcontent.h mainframe.h settings.h
settingsscreen.o: descriptor.h util.h const.h proxy.h controls.h matcher.h
settingsscreen.o: images/btn_add20.xpm images/btn_view20.xpm
settingsscreen.o: images/btn_file20.xpm images/btn_trash20.xpm
testframe.o: testframe.h settings.h descriptor.h filterowner.h url.h
testframe.o: matcher.h expander.h filter.h memory.h const.h log.h events.h
testframe.o: util.h controls.h images/icon32.xpm
textbuffer.o: textbuffer.h receptor.h textfilter.h filter.h memory.h
textbuffer.o: matcher.h filterowner.h url.h util.h settings.h descriptor.h
textbuffer.o: log.h events.h
textfilter.o: textfilter.h filter.h memory.h receptor.h url.h expander.h
textfilter.o: const.h log.h events.h descriptor.h filterowner.h matcher.h
trayicon.o: trayicon.h images/icon32.xpm settings.h descriptor.h log.h
trayicon.o: events.h proxy.h mainframe.h util.h const.h logframe.h
url.o: settings.h descriptor.h url.h
util.o: util.h platform.h const.h settings.h descriptor.h
welcomescreen.o: welcomescreen.h windowcontent.h mainframe.h proxy.h log.h
welcomescreen.o: events.h util.h settings.h descriptor.h controls.h
windowcontent.o: windowcontent.h util.h settings.h descriptor.h
zlibbuffer.o: zlibbuffer.h const.h
