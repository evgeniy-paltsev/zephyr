/*
 * Copyright (c) 2023 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <string.h>

#define DT_DRV_COMPAT snps_hostlink_uart

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
 * protocol (2 bytes for type and 2 for size). Here we reserve space for
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
	uint32_t payload_size;		/* Buffer size without packet header.  */
	uint32_t options;		/* For future use.  */
	uint32_t break_to_mon_addr;	/* For future use.  */
} __uncached __packed;

/* Hostlink packet header.  */
struct _hl_pkt_hdr {
	uint32_t packet_id;	/* Packet id.  Always set to 1 here.  */
	uint32_t total_size;	/* Size of packet including header.  */
	uint32_t priority;	/* For future use.  */
	uint32_t type;		/* For future use.  */
	uint32_t checksum;	/* For future use.  */
} __uncached __packed;

struct _hl_packed_int {
	uint16_t type;
	uint16_t size;
	int32_t value;
} __uncached __packed;

struct _hl_packed_short_buff {
	uint16_t type;
	uint16_t size;
	uint8_t payload_short[4];
} __uncached __packed;

BUILD_ASSERT(sizeof(struct _hl_packed_int) == sizeof(struct _hl_packed_short_buff));

struct _hl_pkt_write_char_put {
	struct _hl_packed_int syscall_nr;
	struct _hl_packed_int fd;
	struct _hl_packed_short_buff buff;
	struct _hl_packed_int nbyte;
} __uncached __packed;

struct _hl_pkt_write_char_get {
	struct _hl_packed_int byte_written;
	struct _hl_packed_int host_errno;
} __uncached __packed;

/* Main hostlink structure.  */
struct _hl {
	volatile struct _hl_hdr hdr; /* General hostlink information.  */
	/* Start of the hostlink buffer.  */
	volatile struct _hl_pkt_hdr pkt_hdr;
	union {
		volatile char payload[HL_IOCHUNK + HL_PAYLOAD_RESERVED];
		struct _hl_pkt_write_char_put pkt_write_char_put;
		struct _hl_pkt_write_char_get pkt_write_char_get;
	};
} __aligned (HL_MAX_DCACHE_LINE) __uncached __packed;


/*
 * Main structure.  Do not rename because simulator will look for the
 * '__HOSTLINK__' symbol.
 */
// volatile __uncached struct _hl __HOSTLINK__ = {
// 	.hdr = {
// 		.version = 1,
// 		.target2host_addr = HL_NOADDRESS
// 	}
// };

volatile __uncached struct _hl __HOSTLINK__ = {
	.hdr = {
		.version = 1,
		.target2host_addr = HL_NOADDRESS
	}
};

// static inline void _hl_write32(uintptr_t addr, uint32_t val)
// {&__HOSTLINK__.pkt_write_char_

static inline void _hl_write32(volatile void *addr, uint32_t val)
{
	*(volatile __uncached uint32_t *)addr = val;
}

static inline void _hl_write16(volatile void *addr, uint16_t val)
{
	*(volatile __uncached uint16_t *)addr = val;
}

static inline void _hl_write8(volatile void *addr, uint8_t val)
{
	*(volatile __uncached uint8_t *)addr = val;
}

static inline uint32_t _hl_read32(volatile void *addr)
{
	return *(volatile __uncached uint32_t *)addr;
}

static inline uint16_t _hl_read16(volatile void *addr)
{
	return *(volatile __uncached uint16_t *)addr;
}

/* Get hostlink payload pointer.  */
static volatile __uncached void * _hl_payload(void)
{
	return (volatile __uncached void *) &__HOSTLINK__.payload[0];
}

/* Get hostlink payload size (iochunk + reserved space).  */
static uint32_t _hl_payload_size(void)
{
	return sizeof(__HOSTLINK__.payload);
}

/* Get used space size in the payload.  */
static uint32_t _hl_payload_used(volatile __uncached void *p)
{
	return (volatile __uncached char *)p - (volatile __uncached char *) _hl_payload();
}

// static uint32_t _hl_static_payload_used(volatile __uncached void *p)
// {
// 	return (volatile __uncached char *)p - (volatile __uncached char *) _hl_payload();
// }

/* Fill hostlink packet header.  */
static void _hl_pkt_init(volatile __uncached struct _hl_pkt_hdr *pkt, int size)
{
	_hl_write32(&pkt->packet_id, 1);
	_hl_write32(&pkt->total_size, ALIGN(size, 4) + sizeof(*pkt));
	_hl_write32(&pkt->priority, 0);
	_hl_write32(&pkt->type, 0);
	_hl_write32(&pkt->checksum, 0);

	// pkt->packet_id = 1;
	// pkt->total_size = ALIGN(size, 4) + sizeof (*pkt);
	// pkt->priority = 0;
	// pkt->type = 0;
	// pkt->checksum = 0;
}

/* Get free space size in the payload.  */
static uint32_t _hl_payload_left(volatile __uncached void *p)
{
	return _hl_payload_size() - _hl_payload_used(p);
}

/* Send hostlink packet to the host.  */
static void _hl_static_send(size_t payload_used)
{
	// volatile __uncached struct _hl_hdr *hdr = &__HOSTLINK__.hdr;
	// volatile __uncached struct _hl_pkt_hdr *pkt_hdr = &__HOSTLINK__.pkt_hdr;

	uint32_t buf_addr = (uint32_t)(&__HOSTLINK__.pkt_hdr);

	_hl_pkt_init(&__HOSTLINK__.pkt_hdr, payload_used);

	_hl_write32(&__HOSTLINK__.hdr.buf_addr, buf_addr);
	_hl_write32(&__HOSTLINK__.hdr.payload_size, _hl_payload_size());
	_hl_write32(&__HOSTLINK__.hdr.host2target_addr, HL_NOADDRESS);
	_hl_write32(&__HOSTLINK__.hdr.version, HL_VERSION);
	_hl_write32(&__HOSTLINK__.hdr.options, 0);
	_hl_write32(&__HOSTLINK__.hdr.break_to_mon_addr, 0);

	compiler_barrier();

	/* This tells the debugger we have a command.
	 * It is responsibility of debugger to set this back to HL_NOADDRESS
	 * after receiving the packet.
	 * Please note that we don't wait here because some implementations
	 * use _hl_blockedPeek() function as a signal that we send a messege.
	 */
	_hl_write32(&__HOSTLINK__.hdr.target2host_addr, buf_addr);

	compiler_barrier();
}

static void _hl_send(volatile __uncached void *p)
{
	_hl_static_send(_hl_payload_used(p));
}

/*
 * Wait for host response and return pointer to hostlink payload.
 * Symbol _hl_blockedPeek() is used by the simulator as message signal.
 */
static void __noinline _hl_blockedPeek(void)
{
	while (_hl_read32(&__HOSTLINK__.hdr.host2target_addr) == HL_NOADDRESS) {
		/* TODO: Timeout.  */
	}
}

/* Get message from host.  */
static volatile __uncached char * _hl_recv(void)
{
	_hl_blockedPeek();

	return _hl_payload();
}

static void _hl_static_recv(void)
{
	compiler_barrier();

	_hl_blockedPeek();

	compiler_barrier();
}

/* Mark hostlink buffer as "No message here".  */
static void _hl_delete(void)
{
	_hl_write32(&__HOSTLINK__.hdr.target2host_addr, HL_NOADDRESS);
}

/* Parameter types.  */
#define PAT_CHAR	1
#define PAT_SHORT	2
#define PAT_INT		3
#define PAT_STRING	4
/* For future use.  */
#define PAT_INT64	5

/*
 * Pack integer value (uint32) to provided buffer.
 * Packing structure:
 *     uint16 type  (PAT_INT = 3)
 *     uint16 size  (4)
 *     uint32 value
 */
static volatile __uncached char * _hl_pack_int(volatile __uncached char *p, uint32_t x)
{
	// volatile __uncached uint16_t *type = (volatile __uncached uint16_t *) p;
	// volatile __uncached uint16_t *size = (volatile __uncached uint16_t *) (p + 2);
	// volatile __uncached uint32_t *val = (volatile __uncached uint32_t *) (p + 4);
	const uint32_t payload_used = 8;

	if (_hl_payload_left(p) < payload_used){
		return NULL;
	}

	/* type */
	_hl_write16(p, PAT_INT);

	/* size */
	_hl_write16(p + 2, 4);

	/* value */
	_hl_write32(p + 4, x);

	// *type = PAT_INT;
	// *size = 4;
	// *val = x;

	return p + payload_used;
}

static void _hl_static_pack_int(volatile struct _hl_packed_int *pack, int32_t value)
{
	_hl_write16(&pack->type, PAT_INT);
	_hl_write16(&pack->size, 4);
	_hl_write32(&pack->value, value);
}

static void _hl_static_pack_char(volatile struct _hl_packed_short_buff *pack, unsigned char c)
{
	_hl_write16(&pack->type, PAT_STRING);
	_hl_write16(&pack->size, 1);
	_hl_write8(&pack->payload_short, c);
}

static int _hl_static_unpack_int(volatile struct _hl_packed_int *pack, int32_t *value)
{
	uint16_t type = _hl_read16(&pack->type);
	uint16_t size = _hl_read16(&pack->size);

	if (type != PAT_INT) {
		return -1;
	}

	if (size != 4)  {
		return -1;
	}

	*value = _hl_read32(&pack->value);

	return 0;
}

/*
 * Pack data (pointer and legth) to provided buffer.
 * Packing structure:
 *     uint16 type  (PAT_STRING = 4)
 *     uint16 size  (length)
 *     char   buf[length]
 */
static volatile __uncached char * _hl_pack_ptr(volatile __uncached char *p, const void *s, uint16_t len)
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

/* Unpack integer value (uint32_t) from a buffer.  */
static volatile __uncached char * _hl_unpack_int(volatile __uncached char *p, int32_t *x)
{
	volatile __uncached uint16_t *type = (volatile __uncached uint16_t *) p;
	volatile __uncached uint16_t *size = (volatile __uncached uint16_t *) (p + 2);
	volatile __uncached uint32_t *val = (volatile __uncached uint32_t *) (p + 4);
	const uint32_t payload_used = 8;

	if (_hl_payload_left(p) < payload_used || *type != PAT_INT || *size != 4) {
		return NULL;
	}

	if (x) {
		*x = *val;
	}

	return p + payload_used;
}

#ifdef CONFIG_UART_HOSTLINK_INPUT
/* Unpack data from a buffer.  */
static volatile __uncached char * _hl_unpack_ptr(volatile __uncached char *p, void *s, uint32_t *plen)
{
	volatile __uncached uint16_t *type = (volatile __uncached uint16_t *)p;
	volatile __uncached uint16_t *size = (volatile __uncached uint16_t *)(p + 2);
	volatile __uncached char *buf = p + 4;
	uint32_t payload_used;
	uint32_t len;

	if (_hl_payload_left(p) < 4 || *type != PAT_STRING) {
		return NULL;
	}

	len = *size;
	payload_used = 4 + ALIGN(len, 4);

	if (_hl_payload_left(p) < payload_used) {
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
#endif

#if 0
static int _hl_pack_write(int fd, const char *buf, size_t nbyte, int32_t *nwrite)
{
	int ret = 0;

	/*
	 * Format:
	 * in, int -> syscall (HL_SYSCALL_WRITE)
	 * in, int -> file descriptor
	 * in, ptr -> buffer
	 * in, int -> bytes number
	 * out, int -> bytes written
	 * out, int, host errno
	 */

	_hl_static_pack_int(&__HOSTLINK__.pkt_write_char_put.syscall_nr, HL_SYSCALL_WRITE);

	_hl_static_pack_int(&__HOSTLINK__.pkt_write_char_put.fd, fd);

	_hl_static_pack_char(&__HOSTLINK__.pkt_write_char_put.buff, *buf);

	_hl_static_pack_int(&__HOSTLINK__.pkt_write_char_put.nbyte, nbyte);

	_hl_static_send(sizeof(struct _hl_pkt_write_char_put));
	_hl_static_recv();

	ret = _hl_static_unpack_int(&__HOSTLINK__.pkt_write_char_get.byte_written, nwrite);
	if (ret) {
		return ret;
	}

	/* we can get host errno here with:
	 * _hl_static_unpack_int(&__HOSTLINK__.pkt_write_char_get.host_errno, &host_errno);
	 * but won't need it for UART emulation.
	 */

	return 0;
}

static int32_t _hl_write(int fd, const char *buf, size_t nbyte)
{
	int32_t nwrite = 0;

	int32_t ret = _hl_pack_write(fd, buf, nbyte, &nwrite);

	if (nwrite <= 0) {
		ret = -1;
	}

	_hl_delete ();

  	return ret;
}
#endif

static int32_t _hl_write_char(int fd, const char c)
{
	/*
	 * Format:
	 * in, int -> syscall (HL_SYSCALL_WRITE)
	 * in, int -> file descriptor
	 * in, ptr -> buffer
	 * in, int -> bytes number
	 * out, int -> bytes written
	 * out, int, host errno
	 */

	_hl_static_pack_int(&__HOSTLINK__.pkt_write_char_put.syscall_nr, HL_SYSCALL_WRITE);

	_hl_static_pack_int(&__HOSTLINK__.pkt_write_char_put.fd, fd);

	_hl_static_pack_char(&__HOSTLINK__.pkt_write_char_put.buff, c);

	_hl_static_pack_int(&__HOSTLINK__.pkt_write_char_put.nbyte, 1);

	_hl_static_send(sizeof(struct _hl_pkt_write_char_put));
	_hl_static_recv();

	int32_t nwrite = 0;
	int ret = _hl_static_unpack_int(&__HOSTLINK__.pkt_write_char_get.byte_written, &nwrite);

	/* we can get host errno here with:
	 * _hl_static_unpack_int(&__HOSTLINK__.pkt_write_char_get.host_errno, &host_errno);
	 * but we don't need it for UART emulation.
	 */

	if (nwrite <= 0) {
		ret = -1;
	}

	_hl_delete();

  	return ret;
}

#ifdef CONFIG_UART_HOSTLINK_INPUT
static volatile __uncached char * _hl_pack_read(int fd, size_t count, int32_t *hl_n)
{
	volatile __uncached char *p = _hl_payload();

	p = _hl_pack_int(p, fd);
	if (p == NULL) {
		return NULL;
	}

	p = _hl_pack_int(p, count);
	if (p == NULL) {
		return NULL;
	}

	_hl_send(p);
	p = _hl_recv();

	p = _hl_unpack_int(p, hl_n);
	if (p == NULL) {
		return NULL;
	}

	return p;
}

/* Read one chunk.  Implements HL_SYSCALL_READ.  */
static int32_t _hl_read(int fd, void *buf, size_t count)
{
	int32_t ret;
	int32_t hl_n;

	volatile __uncached char *p = _hl_pack_read(fd, count, &hl_n);

	if (p == NULL) {
		ret = -1;
	} else if (hl_n < 0) {
		/* we can get host errno here with
		 * p = _hl_unpack_int(p, &host_errno);
		 * however it's unused currently
		 */
		ret = -1;
	} else {
		int32_t actual_read;

		/* data read */
		p = _hl_unpack_ptr(p, buf, &actual_read);
		ret = actual_read;

		if (p == NULL || actual_read != hl_n) {
			ret = -1;
		}
	}

	_hl_delete ();

	return ret;
}
#endif

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

#ifdef CONFIG_UART_HOSTLINK_INPUT
	unsigned int ret = _hl_read(1, c, 1);

	return ret > 0 ? 0 : -1;
#else
	return -1;
#endif
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

	// _hl_write(1, &c, 1);
	_hl_write_char(1, c);
}

static const struct uart_driver_api uart_hostlink_driver_api = {
	.poll_in = uart_hostlink_poll_in,
	.poll_out = uart_hostlink_poll_out,
};

DEVICE_DT_DEFINE(DT_NODELABEL(hostlink), uart_hostlink_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_SERIAL_INIT_PRIORITY, &uart_hostlink_driver_api);