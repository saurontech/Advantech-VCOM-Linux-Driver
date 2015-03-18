#ifndef __ADV_TYPE_H
#define __ADV_TYPE_H

#define ADV_PAIR_NONE		0
#define ADV_PAIR_ODD		1
#define ADV_PAIR_EVEN		2
#define ADV_PAIR_MARK		3
#define ADV_PAIR_SPACE		4
#define ADV_PAIR_UNDEF		5

#define ADV_FLOW_NONE		0
#define ADV_FLOW_XONXOFF	1
#define ADV_FLOW_RTSCTS		2
#define ADV_FLOW_DTSDTR		3
#define ADV_FLOW_UNDEF		4

#define ADV_STOP_1		0
//#define ADV_STOP_1P5		1
#define ADV_STOP_2		2
#define ADV_STOP_UNDEF		3

#define ADV_MS_DTR	0x002
#define ADV_MS_RTS	0x004
#define ADV_MS_CTS	0x010
#define ADV_MS_DSR	0x020
#define ADV_MS_RNG	0x040
#define ADV_MS_CAR	0x080
#define ADV_MS_DCD	ADV_MS_CAR
#define ADV_MS_RI        ADV_MS_RNG

struct adv_port_info{
	int is_open;
	int baud;
	int flowctl;
	int byte;
	int pair;
	int stop;
	unsigned int ms;
};
#endif
