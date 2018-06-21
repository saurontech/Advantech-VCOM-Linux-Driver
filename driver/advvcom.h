#include "advtype.h"

#ifndef _ADVVCOM_H
#define _ADVVCOM_H

#define ADV_RING_BUF_ENABLED		1
#define ADV_RING_BUF_CLEAN		2

#define ADV_ATTRBUF_NUM			2

#ifdef __USER_SPACE_IF

struct ring_buf{
	int head;
	int tail;
	int begin;
	int size;
	char * mbase;
};
#else

#include "advconf.h"
//#define VCOM_PORTS	10

struct ring_buf{
	spinlock_t lock;
	int head;
	int tail;
	int begin;
	int size;
	unsigned int status;
	wait_queue_head_t wait;
	void * data;
};



struct adv_port_att{
	spinlock_t lock;
	struct adv_port_info _attr;
	unsigned int mctrl; /*for input pins only*/
	int _newbie;
	wait_queue_head_t wait;
	int index;
	int throttled;
	int size;
	int begin;
	int usr_ptr;
	void * data;
};

struct adv_vcom{
	struct uart_port * adv_uart;
	struct adv_port_att attr;
	struct ring_buf tx;
	struct ring_buf rx;
	struct proc_dir_entry * entry;
	struct list_head list;
};

static inline int flush_attr_info(struct adv_port_att * attr)
{
	struct adv_port_info * usr_base = attr->data;

	if(attr->_newbie){
		attr->_newbie = 0;
	}else{
		attr->usr_ptr = (attr->usr_ptr + 1) % ADV_ATTRBUF_NUM;
	}

	memcpy(&usr_base[attr->usr_ptr], &attr->_attr, sizeof(struct adv_port_info));

	return (attr->usr_ptr);
}

static inline int move_rb_head(struct ring_buf * buf, int length)
{
	if(buf->status & ADV_RING_BUF_CLEAN){
		buf->status &= (~ADV_RING_BUF_CLEAN);
		buf->head = buf->tail;
	}else{
		buf->head += length;
		buf->head %= (buf->size);
	}

	return 0;
}

static inline int move_rb_tail(struct ring_buf * buf, int length)
{
	buf->tail += length;
	buf->tail %= (buf->size);

	return 0;
}

#endif

static inline int is_rb_empty(struct ring_buf buf)
{
	return (buf.tail == buf.head);
}

static inline int get_rb_length(struct ring_buf buf)
{
	return (buf.tail >= buf.head)?(buf.tail - buf.head ):(buf.size - buf.head + buf.tail);
}

static inline int get_rb_llength(struct ring_buf buf)
{
	int a = buf.size - buf.head;
	int b = get_rb_length(buf);

	return (a < b)?a:b;
}

static inline int get_rb_room(struct ring_buf buf)
{
	return buf.size - get_rb_length(buf) - 1;
}

static inline int get_rb_lroom(struct ring_buf buf)
{
	int a = (buf.size - buf.tail);
	int b = get_rb_room(buf);
	return a < b?a:b;
}

static inline int get_rb_tail(struct ring_buf buf)
{
	return buf.tail;
}

static inline int get_rb_head(struct ring_buf buf)
{
	return buf.head;
}

#endif

