all:client server
 
client:src/client.cpp
	mkdir -p build
	g++ -g -o build/client src/client.cpp -std=c++17
server:src/server.cpp src/Socket.cpp src/InetAddress.cpp src/Epoll.cpp src/Channel.cpp src/EventLoop.cpp src/TcpServer.cpp src/Acceptor.cpp src/Connection.cpp
	g++ -g -o build/server src/server.cpp    src/Socket.cpp    src/InetAddress.cpp \
						   src/Epoll.cpp     src/Channel.cpp   src/EventLoop.cpp \
						   src/TcpServer.cpp src/Acceptor.cpp  src/Connection.cpp \
						   -std=c++17
clean:
	rm -rf build/