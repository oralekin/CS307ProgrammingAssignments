CXX = g++
CXXFLAGS = -I. -std=c++20 -Wall -fdiagnostics-color=always -g3 -pedantic -lpthread
HEADERS=$(wildcard src/**.h)


# from bundle:
TARGET1 = build/court_test2
TARGET2 = build/court_test

SOURCE1 = src/court_test2.cpp
SOURCE2 = src/court_test.cpp

tests: $(TARGET1) $(TARGET2)

$(TARGET1): $(SOURCE1) $(HEADERS)
	$(CXX) $(SOURCE1) -o $(TARGET1) $(CXXFLAGS)

$(TARGET2): $(SOURCE2) $(HEADERS)
	$(CXX) $(SOURCE2) -o $(TARGET2) $(CXXFLAGS)

.PHONY: run all tests
