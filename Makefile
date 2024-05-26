SOURCES=$(main.c)
HEADERS=$()
FLAGS=-g

all: main 

build: $(SOURCES) $(HEADERS) Makefile
	mpicc main.c -o main

clean:
	rm main 

run: build 
	mpirun -oversubscribe -np 8 ./main

build_debug: $(SOURCES) $(HEADERS) Makefile
	mpicc $(FLAGS) main.c -o main -DDEBUG -Wall -Wextra -Werror -g

run_debug: build_debug
	mpirun -oversubscribe -np 8 ./main
