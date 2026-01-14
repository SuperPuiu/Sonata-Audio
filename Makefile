SOURCES = $(wildcard src/*.c)
CFLAGS = -Isrc/.. -Isrc/include/ -Wall -flto -Wextra -Wshadow -ffunction-sections -Wl,--gc-sections -lSDL3 -lSDL3_mixer

buildTest:
	mkdir -p bin
	gcc $(SOURCES) microui.c $(CFLAGS) -g -fsanitize=address -ggdb3 -lX11 -o bin/SonataAudio

linux:
	mkdir -p bin
	gcc $(SOURCES) microui.c $(CFLAGS) -DNDEBUG -Os -lX11 -o bin/SonataAudio

windows:
	x86_64-w64-mingw32-gcc -D WINDOWS="" $(SOURCES) microui.c $(CFLAGS) -lcomdlg32 -lgdi32 -lole32 -o3 -o bin/SonataAudio.exe

linuxRPC:
	gcc $(SOURCES) microui.c DiscordRPC/build/libdiscordrpc.a $(CFLAGS) -DNDEBUG -o3 -lX11 -o bin/SonataAudio -IDiscordRPC/inc/

run:
	./bin/SonataAudio

clean:
	rm bin/SonataAudio
	rm bin/SonataAudio.exe
