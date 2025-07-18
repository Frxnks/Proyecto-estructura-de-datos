CXX = g++
CXXFLAGS = -std=c++17 -Wall
TARGET = main.exe
SRC = main.cpp
HEADERS = Grafo.h Document.h LRUCache.h

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	-rm -f $(TARGET) ResultQueries.txt ListaAdyacencia.txt