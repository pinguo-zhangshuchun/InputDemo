/******************************
 * file name:	socketwrapper.h
 * description:	wrapper socket
 * author:		kari.zhang
 *
 ******************************/

#ifndef __SOCKETWRAPPER__H__
#define __SOCKETWRAPPER__H__

void print_ip(uint32 ip);
int prepare_tcp_client(const char *ip, int port);
int prepare_tcp_server(int port);
int send_data(int fd, void *data, int len);
int recv_data(int fd, void *data, int len);

#endif
