all:
	gcc scrabbleLinux.c -L./Linux -lglfw3 -ldl -lm -lX11 -lglad -lGL -lGLU -lpthread -o scrabbleLinux.exe
win:
	gcc scrabbleWindows.c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -o scrabbleWindows.exe
clean:
	rm scrabbleLinux.o
	rm scrabbleWindows.exe