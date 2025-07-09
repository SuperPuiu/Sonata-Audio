SOURCES = $(wildcard src/*.c)
CFLAGS = -Isrc/.. -Wall -Wextra -Wshadow -lSDL3 -lSDL3_mixer -lm

buildTest:
	mkdir -p bin
	gcc $(SOURCES) microui.c $(CFLAGS) -fsanitize=leak -g -ggdb3 -lX11 -o bin/SonataAudio

linux:
	mkdir -p bin
	gcc $(SOURCES) microui.c $(CFLAGS) -DNDEBUG -o3 -lX11 -o bin/SonataAudio

windows:
	x86_64-w64-mingw32-gcc -D WINDOWS="" $(SOURCES) microui.c $(CFLAGS) -lcomdlg32 -lgdi32 -lole32 -o3 -o bin/SonataAudio.exe

run:
	./bin/SonataAudio

clean:
	rm bin/SonataAudio
	rm bin/SonataAudio.exe
