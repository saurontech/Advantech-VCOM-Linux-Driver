#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,10,0)
//@current
#else
void adv_uart_xmit(struct uart_port *port)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct circ_buf *xmit = &up->port.state->xmit;
	struct ring_buf *tx = up->tx;
	char * tx_base = up->tx->data;
	int tx_size, tx_lsize, count;
	int mcplen;
	int tx_tail;
	int max_retry;
	
	if(uart_circ_empty(xmit)){
		return;	
	}
	if(get_rb_room(*tx) == 0){
		return;
	}

	count = uart_circ_chars_pending(xmit);
	tx_size = get_rb_room(*tx);
	tx_lsize = get_rb_lroom(*tx);
	
	count = (count < tx_size)?count:tx_size;

	max_retry = 3;

	do{
		tx_tail = get_rb_tail(*tx);

		if(count > tx_lsize){
			mcplen = tx_lsize;
		}else{
			mcplen = count;
		}

		if(((xmit->tail + mcplen) & (UART_XMIT_SIZE - 1)) < xmit->tail){
			mcplen = UART_XMIT_SIZE - xmit->tail;
		}

		memcpy(&tx_base[tx_tail], &(xmit->buf[xmit->tail]), mcplen);

		count -= mcplen;
		move_rb_tail(tx, mcplen);
		xmit->tail += mcplen;
		xmit->tail &= (UART_XMIT_SIZE - 1);
		tx_lsize = get_rb_lroom(*tx);

		max_retry--;

	}while(count > 0 && max_retry > 0);

}
#endif

