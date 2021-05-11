FLAGS = -g -c
COMMON = build/hashmap.o build/bucketlist.o build/requests.o build/virusRequest.o build/country.o build/citizen.o  build/virus.o build/bloomfilter.o build/setofbfs.o build/skiplist.o build/linkedlist.o build/dateOps.o build/travelMonitorCommands.o build/readWriteOps.o
MONITOROBJ = build/inputParsing.o  build/monitorProcessCommands.o
LINK = -lm

TESTEXECS = testsetofbfs testBloomFilter testDateDiff testVirusRequest

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

testVirusRequest: testbuild/testVirusRequest.o build/requests.o build/virusRequest.o
	gcc -o $@ $^

run:
	./travelMonitor -m 3 -b 5 -s 100000 -i test_dir

run_debug:
	valgrind --trace-children=yes ./travelMonitor -m 3 -b 5 -s 100000 -i test_dir

run_check_leaks:
	valgrind --trace-children=yes --leak-check=full ./travelMonitor -m 2 -b 5 -s 100000 -i unbalanced_load

clean_all: clean_pipes clean_log clean_tests clean

clean_log:
	rm -f log_file.*

clean_tests:
	rm -f $(TESTEXECS) testbuild/*

clean_pipes:
	rm /tmp/*r
	rm /tmp/*w

clean:
	rm -f travelMonitor monitorProcess build/*
