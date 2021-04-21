FLAGS = -g -c

all: travelMonitor 

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitor: build/travelMonitor.o
	gcc -o $@ $^

clean:
	rm -f travelMonitor build/*
