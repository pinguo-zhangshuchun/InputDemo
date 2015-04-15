/******************************
 * file name:   protocol.h
 * description: define protocol
 * author:      Kari.Zhang
 * date:        2015-03-04
 *
 * modifications:
 *
 * ****************************/

#ifndef __PROTOCOL__H__
#define __PROTOCOL__H__

#include <comm.h>

#define DEFAULT_PORT 8099

typedef struct msgheader_t {
    enum {
        REQ_CONNECT,    // request connect
        RES_CONNECT,    // response connect
        REQ_RELAY,      // request relay
        RES_RELAY,      // response relay
    } message_type;

    enum {
        DEVICE_SLAVE,   // be-controlled device 
        DEVICE_MASTER,  // master device
        DEVICE_ADMIN    // administractor
    } device_type;

    uint32 ip;
    uint32 port;
    uint32 token;
} msgheader_t;

/**
 * validate the message header
 * return:
 *      0  success
 *      -1 failed
 */
int validate_msgheader(msgheader_t *header);

#endif
