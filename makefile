all: host player
host: host.c
	gcc host.c -o host
player: player.c
	gcc player.c -o player
clean:
	rm host player