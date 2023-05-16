run:
	@rm -rf cache_simulate
	@g++ -std=c++11 -o cache_simulate main.cpp
	@./cache_simulate 64 1024 2 65536 8 memory_trace_files/trace1.txt
	@rm -rf cache_simulate