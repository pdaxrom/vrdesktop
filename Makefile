TARGET = vrdesktop

CFLAGS = -Wall -Wpedantic
CFLAGS += $(shell sdl2-config --cflags)

LIBS =
LIBS += $(shell sdl2-config --libs) -lSDL2_image
LIBS += -lxcb -lxcb-shm

OBJS = xcbgrabber.o

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJS) $(TARGET)
