CC = gcc

CFLAGS = -Wall -Wextra -g \
  $(shell pkg-config --cflags wayland-client cairo alsa libmpg123)

LDFLAGS = \
  $(shell pkg-config --libs wayland-client cairo alsa libmpg123) \
  -lpthread -lm

OBJS = main.o keyboard.o xdg-shell-protocol.o cube.o audio.o wayland_client.o

line: $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o line
