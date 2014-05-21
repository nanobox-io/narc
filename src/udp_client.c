// -*- mode: c; tab-width: 8; indent-tabs-mode: 1; st-rulers: [70] -*-
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2013 Pagoda Box, Inc.  All rights reserved.
 */

#include "narc.h"
#include "udp_client.h"

#include "sds.h"	/* dynamic safe strings */
#include "zmalloc.h"	/* total memory usage aware version of malloc/free */

#include <stdio.h>	/* standard buffered input/output */
#include <stdlib.h>	/* standard library definitions */
#include <unistd.h>	/* standard symbolic constants and types */
#include <uv.h>		/* Event driven programming library */

/*============================ Utility functions ============================ */

void 
free_udp_write_req(uv_write_t *req) 
{
	sdsfree((char *)req->data);
	zfree(req->bufs);
	zfree(req);
}

narc_udp_client
*new_udp_client(void)
{
	narc_udp_client *client = (narc_udp_client *)zmalloc(sizeof(narc_udp_client));
	client->socket   = NULL;

	return client;
}

void
handle_udp_write(uv_write_t* req, int status)
{
	free_udp_write_req(req);
}

uv_buf_t 
handle_udp_read_alloc_buffer(uv_handle_t* handle, size_t size)
{
	return uv_buf_init(zmalloc(size), size);
}

void
handle_udp_read(uv_udp_t *req, ssize_t nread, uv_buf_t buf, struct sockaddr *addr, unsigned flags)
{
	if (nread >= 0){
		narc_log(NARC_WARNING, "dropping packet: %s", buf.base);

	}else {
		narc_log(NARC_WARNING, "Udp read error: %s", 
			uv_err_name(uv_last_error(server.loop)));
	}

	zfree(buf.base);
}

void
handle_udp_send(uv_udp_send_t* req, int status)
{
	if (status != 0){
		narc_log(NARC_WARNING, "Udp send error: %s", 
			uv_err_name(uv_last_error(server.loop)));
	}

	zfree(req->data);
}

void
start_udp_read()
{
	uv_udp_recv_start(server.client, handle_udp_read_alloc_buffer, handle_udp_read);
}

/*================================== API ==================================== */

void
init_udp_client(void)
{
	server.client = (void *)new_udp_client();
	
	uv_udp_init(server.loop, server.client);

	struct sockaddr_in recv_addr = uv_ip4_addr("0.0.0.0", 0);
	uv_udp_bind(server.client, recv_addr, 0);
	start_udp_read();
}

void
submit_udp_message(char *message)
{
	uv_udp_send_t *req = (uv_udp_send_t *)zmalloc(sizeof(uv_udp_send_t));
	uv_buf_t buf    = uv_buf_init(message, strlen(message));
	
	req->data = (void *)message;
	struct sockaddr_in send_addr = uv_ip4_addr("127.0.0.1", 1234);
    uv_udp_send(req, server.client, &buf, 1, send_addr, handle_udp_send);

}