FLAGS = -g -c
COMMON = build/hashmap.o build/bucketlist.o build/country.o build/citizen.o  build/virus.o build/bloomfilter.o build/setofbfs.o build/skiplist.o build/linkedlist.o build/dateOps.o
MONITOROBJ = build/inputParsing.o  
LINK = -lm

TESTEXECS = testsetofbfs testBloomFilter testDateDiff

all: travelMonitor monitorProcess

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitor: build/travelMonitor.o $(COMMON)
	gcc -o $@ $^ $(LINK)

monitorProcess: build/monitorProcess.o $(COMMON) $(MONITOROBJ)
	gcc -o $@ $^ $(LINK)

testbuild/%.o: test/%.c
	gcc $(FLAGS) $< -o $@

testDateDiff: testbuild/testDateDiff.o build/dateOps.o
	gcc -o $@ $^

testsetofbfs: testbuild/testsetofbfs.o $(COMMON) $(MONITOROBJ) 
	gcc -o $@ $^ $(LINK)

testBloomFilter: testbuild/testBloomFilter.o build/bloomfilter.o
	gcc -o $@ $<  build/bloomfilter.o -DK=2

clean_all: clean_log clean_tests clean

clean_log:
	rm -f log_file.*

clean_tests:
	rm -f $(TESTEXECS) testbuild/*

clean:
	rm -f travelMonitor monitorProcess build/*
