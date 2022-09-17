all: load exp

load: LoadTimes.cpp
	g++ -std=c++14 LoadTimes.cpp -o LoadTimes.exe -g -static

exp: ExportTimes.cpp
	g++ -std=c++14 ExportTimes.cpp -o ExportTimes.exe -g -static
