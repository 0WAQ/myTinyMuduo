all:client tcp_epoll
 
client:src/client.cpp
	mkdir -p build
	g++ -g -o build/client src/client.cpp -std=c++17
tcp_epoll:src/tcp_epoll.cpp src/InetAddress.cpp
	g++ -g -o build/tcp_epoll src/tcp_epoll.cpp src/InetAddress.cpp -std=c++17
clean:
	rm -rf build/