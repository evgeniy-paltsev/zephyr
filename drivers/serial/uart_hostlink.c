/*
 * Copyright (c) 2019 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <string.h>

#define HL_SYSCALL_OPEN		0
#define HL_SYSCALL_CLOSE	1
#define HL_SYSCALL_READ		2
#define HL_SYSCALL_WRITE	3
#define HL_SYSCALL_LSEEK	4
#define HL_SYSCALL_UNLINK	5
#define HL_SYSCALL_ISATTY	6
#define HL_SYSCALL_TMPNAM	7
#define HL_SYSCALL_GETENV	8
#define HL_SYSCALL_CLOCK	9
#define HL_SYSCALL_TIME		10
#define HL_SYSCALL_RENAME	11
#define HL_SYSCALL_ARGC		12
#define HL_SYSCALL_ARGV		13
#define HL_SYSCALL_RETCODE	14
#define HL_SYSCALL_ACCESS	15
#define HL_SYSCALL_GETPID	16
#define HL_SYSCALL_GETCWD	17
#define HL_SYSCALL_USER		18


#ifndef __uncached
#define __uncached __attribute__((uncached))
#endif /* __uncached */

#ifndef __noinline
#define __noinline __attribute__((noinline))
#endif /* __noinline */


#define ALIGN(x, y) (((x) + ((y) - 1)) & ~((y) - 1))

volatile __uncached char *_hl_message (uint32_t syscall, const char *format, ...);

/* Fuctions for direct work with the Hostlink buffer.  */
volatile __uncached char *_hl_pack_int (volatile __uncached char *p, uint32_t x);
volatile __uncached char *_hl_pack_ptr (volatile __uncached char *p, const void *s, uint16_t len);
volatile __uncached char *_hl_pack_str (volatile __uncached char *p, const char *s);
volatile __uncached char *_hl_unpack_int (volatile __uncached char *p, int32_t *x);
volatile __uncached char *_hl_unpack_ptr (volatile __uncached char *p, void *s, uint32_t *plen);
volatile __uncached char *_hl_unpack_str (volatile __uncached char *p, char *s);

/* Low-level functions from hl_gw.  */
extern uint32_t _hl_iochunk_size (void);
extern void _hl_delete (void);

#define DT_DRV_COMPAT snps_hostlink_uart

// extern struct k_mutex rtt_term_mutex;

struct uart_hostlink_data {
#ifdef CONFIG_UART_ASYNC_API
	uart_callback_t callback;
	void *user_data;
#endif /* CONFIG_UART_ASYNC_API */
};


#define HL_VERSION 1

/*
 * Maximum message size without service information,
 * see also HL_PAYLOAD_RESERVED.
 */
#ifndef HL_IOCHUNK
#define HL_IOCHUNK 1024
#endif

/*
 * Each syscall argument have 4 bytes of service information in hostlink
 * protocol (2 bytes for type and 2 for size).  Here we reserve space for
 * 32 arguments.
 */
#define HL_PAYLOAD_RESERVED (32 * 4)

/* "No message here" mark.  */
#define HL_NOADDRESS 0xFFFFFFFF

#define HL_MAX_DCACHE_LINE 256

/* Hostlink gateway structure.  */
struct _hl_hdr {
	uint32_t version;		/* Current version is 1.  */
	uint32_t target2host_addr;	/* Packet address from target to host.  */
	uint32_t host2target_addr;	/* Packet address from host to target.  */
	uint32_t buf_addr;		/* Address for host to write answer.  */
	uint32_t payload_size;	/* Buffer size without packet header.  */
	uint32_t options;		/* For future use.  */
	uint32_t break_to_mon_addr;	/* For future use.  */
} __uncached __packed;

/* Hostlink packet header.  */
struct _hl_pkt_hdr {
	uint32_t packet_id;   /* Packet id.  Always set to 1 here.  */
	uint32_t total_size;  /* Size of packet including header.  */
	uint32_t priority;    /* For future use.  */
	uint32_t type;	/* For future use.  */
	uint32_t checksum;    /* For future use.  */
} __uncached __packed;

/* Main hostlink structure.  */
struct _hl {
	volatile struct _hl_hdr hdr; /* General hostlink information.  */
	/* Start of the hostlink buffer.  */
	volatile struct _hl_pkt_hdr pkt_hdr;
	volatile char payload[HL_IOCHUNK + HL_PAYLOAD_RESERVED];
} __aligned (HL_MAX_DCACHE_LINE) __uncached __packed;


/*
 * Main structure.  Do not rename because simulator will look for the
 * '__HOSTLINK__' symbol.
 */
volatile struct _hl __HOSTLINK__ = {
	.hdr = {
		.version = 1 ,
		.target2host_addr = HL_NOADDRESS
	}
};

/* Get hostlink payload pointer.  */
volatile __uncached void * _hl_payload(void)
{
	return (volatile __uncached void *) &__HOSTLINK__.payload[0];
}

/* Get hostlink payload size (iochunk + reserved space).  */
static uint32_t _hl_payload_size(void)
{
	return sizeof (__HOSTLINK__.payload);
}

/* Get used space size in the payload.  */
static uint32_t _hl_payload_used(volatile __uncached void *p)
{
	return (volatile __uncached char *)p - (volatile __uncached char *) _hl_payload();
}

/* Fill hostlink packet header.  */
static void _hl_pkt_init(volatile __uncached struct _hl_pkt_hdr *pkt, int size)
{
	pkt->packet_id = 1;
	pkt->total_size = ALIGN(size, 4) + sizeof (*pkt);
	pkt->priority = 0;
	pkt->type = 0;
	pkt->checksum = 0;
}

/* Get hostlink iochunk size.  */
uint32_t _hl_iochunk_size(void)
{
	return HL_IOCHUNK;
}

/* Get free space size in the payload.  */
uint32_t _hl_payload_left (volatile __uncached void *p)
{
	return _hl_payload_size() - _hl_payload_used(p);
}

/* Send hostlink packet to the host.  */
void _hl_send(volatile __uncached void *p)
{
	volatile __uncached struct _hl_hdr *hdr = &__HOSTLINK__.hdr;
	volatile __uncached struct _hl_pkt_hdr *pkt_hdr = &__HOSTLINK__.pkt_hdr;

	_hl_pkt_init (pkt_hdr, _hl_payload_used (p));

	hdr->buf_addr = (uint32_t) pkt_hdr;
	hdr->payload_size = _hl_payload_size ();
	hdr->host2target_addr = HL_NOADDRESS;
	hdr->version = HL_VERSION;
	hdr->options = 0;
	hdr->break_to_mon_addr = 0;

	/* This tells the debugger we have a command.
	* It is responsibility of debugger to set this back to HL_NOADDRESS
	* after receiving the packet.
	* Please note that we don't wait here because some implementations
	* use _hl_blockedPeek() function as a signal that we send a messege.
	*/
	hdr->target2host_addr = hdr->buf_addr;
}

/*
 * Wait for host response and return pointer to hostlink payload.
 * Symbol _hl_blockedPeek() is used by the simulator as message signal.
 */
volatile __uncached char * __noinline _hl_blockedPeek(void)
{
	while (__HOSTLINK__.hdr.host2target_addr == HL_NOADDRESS) {
		/* TODO: Timeout.  */
	}

	return _hl_payload();
}

/* Get message from host.  */
volatile __uncached char * _hl_recv (void)
{
	return _hl_blockedPeek();
}

/* Mark hostlink buffer as "No message here".  */
void _hl_delete (void)
{
	__HOSTLINK__.hdr.target2host_addr = HL_NOADDRESS;
}

/* Parameter types.  */
#define PAT_CHAR	1
#define PAT_SHORT	2
#define PAT_INT		3
#define PAT_STRING	4
/* For future use.  */
#define PAT_INT64	5

/* Used internally to pass user hostlink parameters to _hl_message().  */
struct _hl_user_info {
	uint32_t vendor_id;
	uint32_t opcode;
	uint32_t result;
};

static volatile __uncached char *
_hl_message_va (uint32_t syscall, struct _hl_user_info *user,
		const char *format, va_list ap)
{
  const char *f = format;
  volatile __uncached char *p = _hl_payload ();
  int get_answer = 0;

  p = _hl_pack_int (p, syscall);

  if (syscall == HL_SYSCALL_USER)
    {
      p = _hl_pack_int (p, user->vendor_id);
      p = _hl_pack_int (p, user->opcode);
      p = _hl_pack_str (p, format);
    }

  for (; *f; f++)
    {
      void *ptr;
      uint32_t len;

      if (*f == ':')
	{
	  f++;
	  get_answer = 1;
	  break;
	}

      switch (*f)
	{
	case 'i':
	case '4':
	  p = _hl_pack_int (p, va_arg (ap, uint32_t));
	  break;
	case 's':
	  p = _hl_pack_str (p, va_arg (ap, char *));
	  break;
	case 'p':
	  ptr = va_arg (ap, void *);
	  len = va_arg (ap, uint32_t);
	  p = _hl_pack_ptr (p, ptr, len);
	  break;
	default:
	  return NULL;
	}

      if (p == NULL)
	return NULL;
    }

  _hl_send (p);

  p = _hl_recv ();

  if (syscall == HL_SYSCALL_USER && p)
    p = _hl_unpack_int (p, &user->result);

  if (p && get_answer)
    {
      for (; *f; f++)
	{
	  void *ptr;
	  uint32_t *plen;

	  switch (*f)
	    {
	    case 'i':
	    case '4':
	      p = _hl_unpack_int (p, va_arg (ap, uint32_t *));
	      break;
	    case 's':
	      p = _hl_unpack_str (p, va_arg (ap, char *));
	      break;
	    case 'p':
	      ptr = va_arg (ap, void *);
	      plen = va_arg (ap, uint32_t *);
	      p = _hl_unpack_ptr (p, ptr, plen);
	      break;
	    default:
	      return NULL;
	    }

	  if (p == NULL)
	    return NULL;
	}
    }

  return p;
}

/*
 * Pack integer value (uint32) to provided buffer.
 * Packing structure:
 *     uint16 type  (PAT_INT = 3)
 *     uint16 size  (4)
 *     uint32 value
 */
volatile __uncached char * _hl_pack_int (volatile __uncached char *p, uint32_t x)
{
	volatile __uncached uint16_t *type = (volatile __uncached uint16_t *) p;
	volatile __uncached uint16_t *size = (volatile __uncached uint16_t *) (p + 2);
	volatile __uncached uint32_t *val = (volatile __uncached uint32_t *) (p + 4);
	const uint32_t payload_used = 8;

	if (_hl_payload_left (p) < payload_used){
		return NULL;
	}

	*type = PAT_INT;
	*size = 4;
	*val = x;

	return p + payload_used;
}

/*
 * Pack data (pointer and legth) to provided buffer.
 * Packing structure:
 *     uint16 type  (PAT_STRING = 4)
 *     uint16 size  (length)
 *     char   buf[length]
 */
volatile __uncached char * _hl_pack_ptr(volatile __uncached char *p, const void *s, uint16_t len)
{
	volatile __uncached uint16_t *type = (volatile __uncached uint16_t *)p;
	volatile __uncached uint16_t *size = (volatile __uncached uint16_t *)(p + 2);
	volatile __uncached char *buf = p + 4;
	const uint32_t payload_used = 4 + ALIGN(len, 4);

	if (_hl_payload_left(p) < payload_used) {
		return NULL;
	}

	*type = PAT_STRING;
	*size = len;

	/* _vdmemcpy(buf, s, len); */
	for (uint16_t i = 0; i < len; i++) {
		buf[i] = ((const char *) s)[i];
	}

	return p + payload_used;
}

/*
 * Pack NUL-terminated string to provided buffer.
 * Packing structure:
 *     uint16 type  (PAT_STRING = 4)
 *     uint16 size  (length)
 *     char   buf[length]
 */
volatile __uncached char *
_hl_pack_str (volatile __uncached char *p, const char *s)
{
  return _hl_pack_ptr (p, s, strlen (s) + 1);
}

/* Unpack integer value (uint32_t) from a buffer.  */
volatile __uncached char * _hl_unpack_int (volatile __uncached char *p, int32_t *x)
{
	volatile __uncached uint16_t *type = (volatile __uncached uint16_t *) p;
	volatile __uncached uint16_t *size = (volatile __uncached uint16_t *) (p + 2);
	volatile __uncached uint32_t *val = (volatile __uncached uint32_t *) (p + 4);
	const uint32_t payload_used = 8;

	if (_hl_payload_left (p) < payload_used || *type != PAT_INT || *size != 4) {
		return NULL;
	}

	if (x) {
		*x = *val;
	}

	return p + payload_used;
}

/* Unpack data from a buffer.  */
volatile __uncached char * _hl_unpack_ptr(volatile __uncached char *p, void *s, uint32_t *plen)
{
	volatile __uncached uint16_t *type = (volatile __uncached uint16_t *)p;
	volatile __uncached uint16_t *size = (volatile __uncached uint16_t *)(p + 2);
	volatile __uncached char *buf = p + 4;
	uint32_t payload_used;
	uint32_t len;

	if (_hl_payload_left (p) < 4 || *type != PAT_STRING) {
		return NULL;
	}

	len = *size;
	payload_used = 4 + ALIGN(len, 4);

	if (_hl_payload_left (p) < payload_used) {
		return NULL;
	}

	if (plen) {
		*plen = len;
	}

	/* _vsmemcpy(s, buf, len); */
	if (s) {
		for (uint32_t i = 0; i < len; i++){
			((char *) s)[i] = buf[i];
		}
	}

	return p + payload_used;
}

/*
 * Unpack data from a buffer.
 *
 * No difference compared to _hl_unpack_ptr, except that this function
 * does not return a length.
 */
volatile __uncached char * _hl_unpack_str (volatile __uncached char *p, char *s)
{
  return _hl_unpack_ptr (p, s, NULL);
}

volatile __uncached char *
_hl_message (uint32_t syscall, const char *format, ...)
{
  va_list ap;
  volatile __uncached char *p;

  va_start (ap, format);

  p = _hl_message_va (syscall, 0, format, ap);

  va_end (ap);

  return p;
}


static int32_t _hl_pack_write(int fd, const char *buf, size_t nbyte, int32_t *nwrite)
{
	uint32_t host_errno;
	volatile __uncached char *p = _hl_payload ();

	/*
	 * Format:
	 * in, int -> syscall (HL_SYSCALL_WRITE)
	 * in, int -> file descriptor
	 * in, ptr -> buffer
	 * in, int -> bytes number
	 * out, int -> bytes written
	 * out, int, host errno
	 */

	p = _hl_pack_int(p, HL_SYSCALL_WRITE);
	if (p == NULL) {
		return -1;
	}

	p = _hl_pack_int(p, fd);
	if (p == NULL) {
		return -1;
	}

	p = _hl_pack_ptr(p, buf, nbyte);
	if (p == NULL) {
		return -1;
	}

	p = _hl_pack_int(p, nbyte);
	if (p == NULL) {
		return -1;
	}

	_hl_send (p);
	p = _hl_recv();

	p = _hl_unpack_int(p, nwrite);
	if (p == NULL) {
		return -1;
	}

	p = _hl_unpack_int(p, &host_errno);
	if (p == NULL) {
		return -1;
	}

	return 0;
}

static int32_t _hl_write (int fd, const char *buf, size_t nbyte)
{
	int32_t nwrite = 0;

	int32_t ret = _hl_pack_write(fd, buf, nbyte, &nwrite);

	if (nwrite <= 0) {
		ret = -1;
	}

	_hl_delete ();

  	return ret;
}

/* Read one chunk.  Implements HL_SYSCALL_READ.  */
static int32_t _hl_read (int fd, void *buf, size_t count)
{
  int32_t ret;
  int32_t hl_n;
  uint32_t host_errno;
  volatile __uncached char *p;

  p = _hl_message (HL_SYSCALL_READ, "ii:i",
		   (uint32_t) fd,      /* i */
		   (uint32_t) count,   /* i */
		   (uint32_t *) &hl_n  /* :i */);

  if (p == NULL)
    {
      errno = ETIMEDOUT;
      ret = -1;
    }
  else if (hl_n < 0)
    {
      p = _hl_unpack_int (p, &host_errno);
      errno = p == NULL ? EIO : host_errno;
      ret = -1;
    }
  else
    {
      uint32_t n;

      p = _hl_unpack_ptr (p, buf, &n);
      ret = n;

      if (p == NULL || n != (uint32_t) hl_n)
	{
	  errno = EIO;
	  ret = -1;
	}
    }

  _hl_delete ();

  return ret;
}


static int uart_hostlink_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */

static int uart_hostlink_poll_in(const struct device *dev, unsigned char *c)
{
	ARG_UNUSED(dev);

	unsigned int ret = _hl_read(1, c, 1);

	return ret > 0 ? 0 : -1;
}

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_hostlink_poll_out(const struct device *dev, unsigned char c)
{
	ARG_UNUSED(dev);

	_hl_write(1, &c, 1);
}

#ifdef CONFIG_UART_ASYNC_API

static int uart_hostlink_callback_set(const struct device *dev,
				 uart_callback_t callback, void *user_data)
{
	struct uart_hostlink_data *data = dev->data;

	data->callback = callback;
	data->user_data = user_data;
	return 0;
}

static int uart_hostlink_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	const struct uart_hostlink_config *cfg = dev->config;
	struct uart_hostlink_data *data = dev->data;
	unsigned int ch = cfg ? cfg->channel : 0;

	ARG_UNUSED(timeout);

	/* RTT mutex cannot be claimed in ISRs */
	if (k_is_in_isr()) {
		return -ENOTSUP;
	}

	/* Claim the RTT lock */
	if (k_mutex_lock(&rtt_term_mutex, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	/* Output the buffer */
	SEGGER_RTT_WriteNoLock(ch, buf, len);

	/* Return RTT lock */
	SEGGER_RTT_UNLOCK();

	/* Send the TX complete callback */
	if (data->callback) {
		struct uart_event evt = {
			.type = UART_TX_DONE,
			.data.tx.buf = buf,
			.data.tx.len = len
		};
		data->callback(dev, &evt, data->user_data);
	}

	return 0;
}

static int uart_hostlink_tx_abort(const struct device *dev)
{
	/* There is never a transmission to abort */
	ARG_UNUSED(dev);

	return -EFAULT;
}

static int uart_hostlink_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
				   int32_t timeout)
{
	/* SEGGER RTT reception is implemented as a direct memory write to RAM
	 * by a connected debugger. As such there is no hardware interrupt
	 * or other mechanism to know when the debugger has added data to be
	 * read. Asynchronous RX does not make sense in such a context, and is
	 * therefore not supported.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);

	return -ENOTSUP;
}

static int uart_hostlink_rx_disable(const struct device *dev)
{
	/* Asynchronous RX not supported, see uart_hostlink_rx_enable */
	ARG_UNUSED(dev);

	return -EFAULT;
}

static int uart_hostlink_rx_buf_rsp(const struct device *dev,
			       uint8_t *buf, size_t len)
{
	/* Asynchronous RX not supported, see uart_hostlink_rx_enable */
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	return -ENOTSUP;
}
#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api uart_hostlink_driver_api = {
	.poll_in = uart_hostlink_poll_in,
	.poll_out = uart_hostlink_poll_out,
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_hostlink_callback_set,
	.tx = uart_hostlink_tx,
	.tx_abort = uart_hostlink_tx_abort,
	.rx_enable = uart_hostlink_rx_enable,
	.rx_buf_rsp = uart_hostlink_rx_buf_rsp,
	.rx_disable = uart_hostlink_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

#define uart_hostlink(idx)                   DT_NODELABEL(hostlink)

#define uart_hostlink_init(idx, config)						\
	struct uart_hostlink_data uart_hostlink##idx##_data;			\
										\
	DEVICE_DT_DEFINE(uart_hostlink(idx), uart_hostlink_init, NULL,		\
			    &uart_hostlink##idx##_data, config,			\
			    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,		\
			    &uart_hostlink_driver_api)

uart_hostlink_init(0, NULL);
