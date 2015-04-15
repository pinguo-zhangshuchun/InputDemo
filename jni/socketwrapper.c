/******************************
 * file name:	socketwrapper.c
 * description:	wrapper socket
 * author:		kari.zhang
 *
 ******************************/
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <protocol.h>
#include <stdio.h>
#include <string.h>

void print_ip(uint32 ip)
{
	printf("ip:%d.%d.%d.%d\n", 
			ip & 0xff, 
			(ip>>8) & 0xff, 
			(ip>>16) & 0xff,
			(ip>>24) & 0xff);

}

int prepare_tcp_client(const char *ip, int port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		LogE("Failed create socket\n");
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LogE("Failed connect to server");
		close(fd);
		return -1;
	}

	return fd;
}

int prepare_tcp_server(int port)
{
	int fd_srvr = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_srvr < 0) {
		LogE("Failed create server socket");
		return -1;
	}

	int opt = 1;
	if (setsockopt(
				fd_srvr, 
				SOL_SOCKET, 
				SO_REUSEADDR, 
				&opt, 
				sizeof(opt)) < 0) {
		LogE("Failed set socket reuse addr");
		close(fd_srvr);
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = port;

	if (bind(fd_srvr, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LogE("Failed bind addr:%s", strerror(errno));
		close(fd_srvr);
		return -1;
	}

	if (listen(fd_srvr, 1) < 0) {
		LogE("Failed listen");
		close(fd_srvr);
		return -1;
	}

	return fd_srvr;
}

int send_data(int fd, void *data, int len)
{
	int fin = 0, ret = 0;
	do {
		ret = send(fd, data, len - fin, 0);
		if (ret <= 0) {
			return -1;
		}
		data += ret;
		fin += ret;
	} while (fin < len);

	return 0;
}

int recv_data(int fd, void *data, int len)
{
	/*
	int fin = 0, ret = 0;
	do {
		ret = recv(fd, data, len - fin, 0);
		if (ret <= 0) {
			return -1;
		}
		data += ret;
		fin += ret;
	} while (fin < len);

	return 0;
	*/

	return recv(fd, data, len, 0);
}
