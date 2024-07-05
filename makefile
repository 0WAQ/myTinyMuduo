all:client server
 
client:base/client.cpp
	mkdir -p build
	g++ -g -o build/client base/client.cpp -std=c++17
server:base/server.cpp src/Socket.cpp src/InetAddress.cpp src/Epoll.cpp src/Channel.cpp src/EventLoop.cpp src/TcpServer.cpp src/Acceptor.cpp src/Connection.cpp src/Buffer.cpp src/EchoServer.cpp
	g++ -g -o build/server base/server.cpp    src/Socket.cpp    src/InetAddress.cpp \
						   src/Epoll.cpp     src/Channel.cpp   src/EventLoop.cpp \
						   src/TcpServer.cpp src/Acceptor.cpp  src/Connection.cpp \
						   src/Buffer.cpp    src/EchoServer.cpp\
						   -std=c++17
clean:
	rm -rf build/