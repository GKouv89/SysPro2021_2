FLAGS = -g -c
TYPES = build/country.o build/citizen.o build/virus.o build/bloomfilter.o build/setofbfs.o
TRVLOBJ = build/hashmap.o build/bucketlist.o

all: travelMonitor monitorProcess

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitor: build/travelMonitor.o $(TRVLOBJ) $(TYPES)
	gcc -o $@ $^

monitorProcess: build/monitorProcess.o
	gcc -o $@ $^

clean_log:
	rm -f log_file.*

clean:
	rm -f travelMonitor monitorProcess build/*
