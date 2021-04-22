FLAGS = -g -c

all: travelMonitor monitorProcess

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitor: build/travelMonitor.o
	gcc -o $@ $^

monitorProcess: build/monitorProcess.o
	gcc -o $@ $^

clean:
	rm -f travelMonitor build/*
