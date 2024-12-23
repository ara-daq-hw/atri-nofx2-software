CFLAGS=-I../libmcp2221-master/ -fPIC
LDFLAGS=-L../libmcp2221-master/lib/unix/ -lmcp2221

all: libmcp.so

%.so: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -shared -o $@ $^
