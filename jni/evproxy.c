#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <netinet/in.h>
#include <protocol.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "comm.h"
#include "evproxy.h"

struct evp_cntxt{
	int exit_flag;
	int fd_epoll;
	int	fd_remote;	// hearbeat
	int	fd_server;	// server socket
	int	fd_session;	// client session socket
	int	token;
	int fd_dev_max;
	int *fd_dev;
	cb_ask cb_ask;
	cb_cmd cb_cmd;
	cb_report cb_report;
};

evp_cntxt* create_evproxy(const char *ip, int port) 
{
	int fd = prepare_tcp_client(ip, port);
	if (fd < 0) {
		LogE("Failed prepare tcp client socket");
		return NULL;
	}

	int token = 0;
	if (get_token(fd, &token)  < 0) {
		LogE("Failed get toke");
		close(fd);
		return NULL;
	}

	printf("token=%d\n", token);

	int fd_srvr = prepare_server(fd);
	if (fd_srvr < 0) {
		LogE("Failed prepare server");
		return NULL;
	}

	evp_cntxt *cntxt = (struct evp_cntxt *)calloc(1, sizeof(*cntxt));
	if (NULL == cntxt) {
		return NULL;
	}

	cntxt->fd_remote = fd;
	cntxt->fd_server = fd_srvr;
	cntxt->token = token;

	return cntxt;
}

PRIVATE int get_token(int fd, int *token) {
	VALIDATE_NOT_NULL(token);

	msgheader_t header;
	memset(&header, 0, sizeof(header));
	header.message_type = REQ_CONNECT;
	header.device_type = DEVICE_SLAVE;
	header.token = *token;

	if (send(fd, &header, sizeof(header), 0) != sizeof(header)) {
		LogE("Failed send msgheader");
		return -1;
	}

	memset(&header, 0, sizeof(header));
	if (recv(fd, &header, sizeof(header), 0) != sizeof(header)) {
		LogE("Failed recv msgheader");
		return -1;
	}

	int tk = ntohl(header.token);
	printf("token:%d\n", tk);
	if (tk <= 0) { Log("server response error msg\n");
		return -1;
	} 

	*token = tk;
	return 0;
}

PRIVATE int prepare_server(int fd)
{
	struct sockaddr_in addr;
	int addr_len = sizeof(addr);
	if (getsockname(fd, (struct sockaddr*)&addr, &addr_len) < 0) {
		LogE("Failed get peer name");
	}
	else {
		Log("%s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	}

	int opt = 1;
	if (setsockopt(
				fd, 
				SOL_SOCKET, 
				SO_REUSEADDR, 
				&opt, 
				sizeof(opt)) < 0) {
		LogE("Failed set socket reuse addr");
	}

	int fd_srvr = prepare_tcp_server(addr.sin_port);
	if (fd_srvr < 0) {
		LogE("Failed create server socket");
		return -1;
	}

	return fd_srvr;
}

PUBLIC void free_evproxy(evp_cntxt *cntxt)
{
	if (NULL != cntxt) {
		if (cntxt->fd_remote >= 0) {
			close(cntxt->fd_remote);
		} 

		if (cntxt->fd_server >= 0) {
			close(cntxt->fd_server);
		} 

		if (cntxt->fd_session >= 0) {
			close(cntxt->fd_session);
		} 
	}
}

PUBLIC int start_evproxy(evp_cntxt *cntxt)
{
	if (scan_input_device(cntxt) < 0) {
		return -1;
	}

#define EPOLL_MAX 20
	cntxt->fd_epoll = epoll_create(EPOLL_MAX);
	if (cntxt->fd_epoll < 0) {
		LogE("Failed create epoll");
		return -1;
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = cntxt->fd_server;
	if (epoll_ctl(
				cntxt->fd_epoll, 
				EPOLL_CTL_ADD, 
				cntxt->fd_server, 
				&ev) < 0) {
		LogE("Failed epoll add fd_server");
		return -1;
	}

	ev.data.fd = cntxt->fd_dev[0];
	if (epoll_ctl(
				cntxt->fd_epoll, 
				EPOLL_CTL_ADD, 
				cntxt->fd_dev[0], 
				&ev) < 0) {
		LogE("Failed epoll add fd_dev[0]");
		return -1;
	}
}

PUBLIC int stop_evproxy(evp_cntxt *cntxt)
{
	cntxt->exit_flag = 1;
}

PUBLIC int loop_evproxy(evp_cntxt *cntxt)
{
#define MAX_EVENTS 16
	struct epoll_event events[MAX_EVENTS];
	int i, nfds;

	while (!cntxt->exit_flag) {
		nfds = epoll_wait(cntxt->fd_epoll, events, MAX_EVENTS, -1);
		if (nfds < 0) {
			LogE("Failed epoll_wait");
			break;
		}

		for (i = 0; i < nfds; ++i) {
			// client connect
			if (events[i].data.fd == cntxt->fd_server) {
				accept_master_connect(cntxt);
			}
			// /dev/input	
			else if (events[i].data.fd == cntxt->fd_dev[0]) {
				// read event & push
				int x, y;
				if ((read_event, cntxt, &x, &y) < 0) {
					LogE("Failed read event.");
				}

				uint32 buffer[4];
				buffer[0] = htonl(CMD_PSH);
				buffer[1] = 8;
				buffer[2] = htonl(x);
				buffer[3] = htonl(y);

				if (send_data(cntxt->fd_session, buffer, 16) < 0) {
					printf("Failed push event\n");
				}
			}
			// master device message
			else if (events[i].data.fd == cntxt->fd_session) {
				// master device message
				talk_with_master(cntxt);
			}
		}
	}
}
	
PRIVATE int scan_input_device(evp_cntxt *cntxt)
{
#define PATH "/dev/input/event3"
	int fd = open(PATH, O_RDWR);
	if (fd < 0) {
		printf("Failed scan input device, We only support HongMi now.\n");
		return -1;
	}

	cntxt->fd_dev_max = 1;
	cntxt->fd_dev = (int *)calloc(1, sizeof(int));
	if (NULL == cntxt->fd_dev) {
		LogE("Failed calloc mem for fd_dev");
		close (fd);
		return -1;
	}

	cntxt->fd_dev[0] = fd;

	return 0;
}

PRIVATE int read_event(evp_cntxt *cntxt, int *x, int *y)
{
	VALIDATE_NOT_NULL(cntxt);
	VALIDATE_NOT_NULL(x);
	VALIDATE_NOT_NULL(y);
	
	struct input_event ev;
	if (sizeof(ev) != read(cntxt->fd_dev[0], &ev, sizeof(ev))) {
		LogE("Failed read input event");
		return -1;
	}

	*x = ev.code;
	*y = ev.value;
}

PRIVATE int write_event(evp_cntxt *cntxt, int x, int y)
{
	struct input_event ev;
}

PRIVATE int accept_master_connect(evp_cntxt *cntxt)
{
	struct sockaddr addr;
	int addr_len = sizeof(addr);
	int fd = accept(cntxt->fd_server, &addr, &addr_len);
	if (fd < 0) {
		LogE("Failed accept master connect");
		return -1;
	}

	struct sockaddr_in sin;
	memcpy(&sin, &addr, sizeof(addr));
	printf("accept master connect %s:%d\n",
			inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

	if (cntxt->fd_session > 0) {
		printf("Only one master device supported now.\n");
		close (fd);
		return -1;
	}
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(
				cntxt->fd_epoll, 
				EPOLL_CTL_ADD, 
				fd, 
				&ev) < 0) {
		LogE("Failed epoll add master fd");
		close (fd);
		return -1;
	}

	cntxt->fd_session = fd;

	return 0;
}

PRIVATE int talk_with_master(evp_cntxt *cntxt)
{
	VALIDATE_NOT_NULL(cntxt);

	uint32 buff[4];
	uint32 type;
	uint32 len;

	if (4 != recv_data(cntxt->fd_session, &buff, 16)) {
		printf("master device disconnect.\n");
		struct epoll_event ev;
		ev.data.fd = cntxt->fd_session;
		if (epoll_ctl(
					cntxt->fd_epoll, 
					EPOLL_CTL_DEL, 
					cntxt->fd_session, 
					&ev) < 0) {
			LogE("Failed epoll delete fd_sessionr");
			return -1;
		}

		close (cntxt->fd_session);
		cntxt->fd_session = -1;	
		return -1;
	}

	type = buff[0];
	type = ntohl(type);
	if (type <= CMD_MIN || type >= CMD_MAX) {
		LogE("Invalid cmd type");
		return -1;
	}

	len = buff[1];
	len = ntohl(len);
	printf("cmd type=%d,len=%d\n", type, len);


	int x, y;

	switch (type) {
		case CMD_GET:
			buff[2] = htonl(480);
			buff[3] = htonl(800);
			if (send_data(cntxt->fd_session, buff, 16) < 0) {
				printf("Failed send widht, height\n");
			}
			break;

		case CMD_SET:
			x = ntohl(buff[2]);
			y = ntohl(buff[3]);
			printf("x=%d, y=%d\n", x, y);
			write_event(cntxt, x, y);
			break;
	}
	
	return 0;
}
