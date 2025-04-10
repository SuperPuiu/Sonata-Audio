SOURCES = $(wildcard src/*.c)
CFLAGS = -Isrc/.. -Wall -Wextra -Wshadow -pedantic -std=c11 -lSDL3 -lSDL3_mixer -lm

buildTest:
	mkdir -p bin
	gcc $(SOURCES) microui.c $(CFLAGS) -g -ggdb3 -lX11 -o bin/PuiusAudioPlayer_Linux

buildLinux:
	mkdir -p bin
	gcc $(SOURCES) microui.c $(CFLAGS) -o3 -lX11 -o bin/PuiusAudioPlayer_Linux

windows:
	x86_64-w64-mingw32-gcc -D WINDOWS="" $(SOURCES) microui.c $(CFLAGS) -lcomdlg32 -lgdi32 -lole32 -o3 -o bin/PuiusAudioPlayer_Windows.exe

run:
	./bin/PuiusAudioPlayer_Linux

clean:
	rm bin/PuiusAudioPlayer_Linux
	rm bin/PuiusAudioPlayer_Windows.exe
