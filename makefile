all:client server
 
client:base/test-client.cpp base/chat-client.cpp
	mkdir -p build
	g++ -g -o build/test-client base/test-client.cpp -std=c++17
	g++ -g -o build/chat-client base/chat-client.cpp -std=c++17
server:base/server.cpp src/Socket.cpp src/InetAddress.cpp src/Epoll.cpp src/Channel.cpp src/EventLoop.cpp src/TcpServer.cpp src/Acceptor.cpp src/Connection.cpp src/Buffer.cpp src/EchoServer.cpp src/ThreadPool.cpp
	g++ -g -o build/server base/server.cpp   src/Socket.cpp     src/InetAddress.cpp \
						   src/Epoll.cpp     src/Channel.cpp    src/EventLoop.cpp \
						   src/TcpServer.cpp src/Acceptor.cpp   src/Connection.cpp \
						   src/Buffer.cpp    src/EchoServer.cpp src/ThreadPool.cpp\
						   -std=c++17 -lpthread
clean:
	rm -rf build/