/**************************
 * file name:	evproxy.h
 * description:	
 * author:		kari.zhang
 *
 **************************/

#ifndef __EVPROXY__H__
#define __EVPROXY__H__

#include "comm.h"

struct evp_cntxt;
typedef struct evp_cntxt evp_cntxt;

typedef void (*cb_ask)(evp_cntxt*, void*, int);		// the master ask me
typedef void (*cb_cmd)(evp_cntxt*, void*, int);		// the master command me
typedef void (*cb_report)(evp_cntxt*, void*, int);	// report to the master

PUBLIC evp_cntxt* create_evproxy(const char *ip, int port); 
PUBLIC int start_evproxy(evp_cntxt *cntxt); 
PUBLIC int loop_evproxy(evp_cntxt *cntxt); 
PUBLIC int stop_evproxy(evp_cntxt *cntxt); 
PUBLIC void free_evproxy(evp_cntxt *cntxt); 
PUBLIC void set_ask_cb(evp_cntxt *cntxt, cb_ask ask);
PUBLIC void set_cmd_cb(evp_cntxt *cntxt, cb_cmd cmd);
PUBLIC void set_report_cb(evp_cntxt *cntxt, cb_report report);

PRIVATE int get_token(int fd, int *token);
PRIVATE int prepare_server(int fd);
PRIVATE int scan_input_device(evp_cntxt *cntxt);
PRIVATE int read_event(evp_cntxt *cntxt, int *x, int *y);
PRIVATE int write_event(evp_cntxt *cntxt, int x, int y);
PRIVATE int accept_master_connect(evp_cntxt *cntxt);

#endif
