#ifndef _ADVTTY_H
#define _ADVTTY_H

#include "advconf.h"

#define CF_MAXSTRLEN	32
#define CF_LOGNAME  	("advttyd.log")
#define CF_CONFNAME 	("advttyd.conf")
#define CF_PORTPROG 	("vcomd")
#define CF_VERSION	("1.20")

#define CF_MAXPORTS	VCOM_PORTS

#define ADVTTYD_DEBUG
#ifdef ADVTTYD_DEBUG
#define ADV_LOGMSG(FMT, ...) \
        do { \
        char buf[1024]; \
        sprintf(buf, FMT, ## __VA_ARGS__); \
        log_msg(buf); \
        } while(0)
#else
#define ADV_LOGMSG(FMT, ...) while(0)
#endif

#endif
