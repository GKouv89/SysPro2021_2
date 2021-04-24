FLAGS = -g -c
TRVLOBJ = build/hashmap.o build/country.o build/citizen.o build/virus.o build/bucketlist.o  build/bloomfilter.o

all: travelMonitor monitorProcess

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitor: build/travelMonitor.o $(TRVLOBJ)
	gcc -o $@ $^

monitorProcess: build/monitorProcess.o
	gcc -o $@ $^

clean_log:
	rm -f log_file.*

clean:
	rm -f travelMonitor monitorProcess build/*
