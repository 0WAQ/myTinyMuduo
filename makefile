all:client server
 
client:main/client.cpp
	mkdir -p build
	g++ -g -o build/client main/client.cpp -std=c++17
server:main/server.cpp src/Socket.cpp src/InetAddress.cpp src/Epoll.cpp src/Channel.cpp src/EventLoop.cpp src/TcpServer.cpp src/Acceptor.cpp src/Connection.cpp src/Buffer.cpp src/EchoServer.cpp
	g++ -g -o build/server main/server.cpp    src/Socket.cpp    src/InetAddress.cpp \
						   src/Epoll.cpp     src/Channel.cpp   src/EventLoop.cpp \
						   src/TcpServer.cpp src/Acceptor.cpp  src/Connection.cpp \
						   src/Buffer.cpp    src/EchoServer.cpp\
						   -std=c++17
clean:
	rm -rf build/