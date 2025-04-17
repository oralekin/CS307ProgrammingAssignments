CXX = g++
CXXFLAGS = -I. -std=c++20 -Wall -fdiagnostics-color=always -pedantic -pthread
SRC=src/app.cpp
HEADERS=$(wildcard src/**.h)

.PHONY: run all

all: build/app

build/app: $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

build/debug: $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ -g3 $(SRC)

.SILENT: run
run: build/app
ifeq ($(input), )
	$^
else
	$^ < $(input)
endif



SAMPLES = sample1Level sampleMultiLevel sampleQueue sampleMultiLevelPrint
samples: $(SAMPLES)

%: src/%.cpp  $(HEADERS)
	$(CXX) -o $@ $< $(CXXFLAGS) -g3

# clean:
# 	rm -f ./sample1Level
# 	rm -f ./sampleMultiLevel
# 	rm -f ./sampleQueue
# 	rm -f ./sampleMultiLevelPrint
