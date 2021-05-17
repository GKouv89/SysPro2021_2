FLAGS = -g -c
COMMON = build/hashmap.o build/bucketlist.o build/requests.o build/virusRequest.o build/country.o build/citizen.o  build/virus.o build/bloomfilter.o build/setofbfs.o build/skiplist.o build/linkedlist.o build/dateOps.o build/travelMonitorCommands.o build/readWriteOps.o
MONITOROBJ = build/inputParsing.o  build/monitorProcessCommands.o
LINK = -lm

all: travelMonitor Monitor

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitor: build/travelMonitor.o $(COMMON) 
	gcc -o $@ $^ $(LINK)

Monitor: build/monitorProcess.o $(COMMON) $(MONITOROBJ)
	gcc -o $@ $^ $(LINK)

clean_all: clean_log clean

clean_log:
	rm -f log_file.*

clean:
	rm -f travelMonitor Monitor build/*
