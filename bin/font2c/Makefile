CFLAGS += -O2 -Wall 
CFLAGS += $(shell freetype-config --cflags)
LIBS += $(shell freetype-config --libs) -I/usr/include/freetype2/freetype/

font2c: font2c.c
	    $(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	    -rm -f font2c
