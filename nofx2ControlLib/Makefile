CFLAGS=-I../libmcp2221-master/ -I../libmcp/ -fPIC -std=c99
LDFLAGS=-L../libmcp2221-master/lib/unix/ -lmcp2221 -L../libmcp/ -lmcp

all: nofx2ControlLib.so

clean:
	rm -rf nofx2ControlLib.so

%.so: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -shared -o $@ $^
