APP=main.cpp SearchEngine.h SearchEngine.cpp
OBJ=KrovetzStemmer.o SearchEngine.o
CXXFLAGS=-g

search-engine: $(OBJ) $(APP)
	$(CXX) $(CXXFLAGS) main.cpp $(OBJ) -o search-engine

all: search-engine

clean:
	rm -rf search-engine *~ $(OBJ)


