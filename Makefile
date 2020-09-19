TARGET = vrdesktop

CFLAGS = -Wall -Wpedantic
CFLAGS += $(shell sdl2-config --cflags)

LIBS =
LIBS += $(shell sdl2-config --libs) -lSDL2_image -lGL
LIBS += -lxcb -lxcb-shm

CFLAGS += -g
LDFLAGS += -g

OBJS = xcbgrabber.o sdl2out.o shader.o vrdesktop.o

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJS) $(TARGET)
