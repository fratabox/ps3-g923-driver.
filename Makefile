PREFIX := ppu
TARGET := G923Diag.self
TITLE := Logitech G923 PS3 Diagnostic
APPID := G92300001
CONTENTID := UP0001-$(APPID)_00-0000000000000000
ICON0 := $(PS3DEV)/bin/ICON0.PNG
SFOXML := $(PS3DEV)/bin/sfo.xml

CC := $(PREFIX)-gcc
LD := $(PREFIX)-gcc
CFLAGS := -Wall -Wextra -O2 -std=gnu99 -I$(PSL1GHT)/$(PREFIX)/include
LDFLAGS := -L$(PSL1GHT)/$(PREFIX)/lib -lsysutil

SOURCES := src/main.c src/g923_protocol.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean pkg
all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET:.self=.elf): $(OBJECTS)
	$(LD) $(OBJECTS) $(LDFLAGS) -o $@

$(TARGET): $(TARGET:.self=.elf)
	make_self_npdrm $< $@ $(CONTENTID)

pkg: $(TARGET:.self=.elf)
	rm -rf pkg
	mkdir -p pkg/USRDIR
	cp $(ICON0) pkg/ICON0.PNG
	make_self_npdrm $< pkg/USRDIR/EBOOT.BIN $(CONTENTID)
	sfo --title "$(TITLE)" --appid "$(APPID)" -f $(SFOXML) pkg/PARAM.SFO
	pkg --contentid $(CONTENTID) pkg/ G923Diag.pkg
	rm -rf pkg

clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET:.self=.elf) G923Diag.pkg
	rm -rf pkg
