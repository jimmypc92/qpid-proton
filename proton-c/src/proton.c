/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <proton/driver.h>
#include <proton/message.h>
#include <proton/util.h>
#include <unistd.h>
#include "util.h"
#include "pn_config.h"
#include <proton/codec.h>
#include <inttypes.h>

int buffer(int argc, char **argv)
{
  pn_buffer_t *buf = pn_buffer(16);

  pn_buffer_append(buf, "abcd", 4);
  pn_buffer_print(buf); printf("\n");
  pn_buffer_prepend(buf, "012", 3);
  pn_buffer_print(buf); printf("\n");
  pn_buffer_prepend(buf, "z", 1);
  pn_buffer_print(buf); printf("\n");
  pn_buffer_append(buf, "efg", 3);
  pn_buffer_print(buf); printf("\n");
  pn_buffer_append(buf, "hijklm", 6);
  pn_buffer_print(buf); printf("\n");
  pn_buffer_trim(buf, 1, 1);
  pn_buffer_print(buf); printf("\n");
  pn_buffer_trim(buf, 4, 0);
  pn_buffer_print(buf); printf("\n");
  pn_buffer_clear(buf);
  pn_buffer_print(buf); printf("\n");
  pn_buffer_free(buf);

  pn_dbuf_t *data = pn_dbuf(16);
  int err = pn_dbuf_fill(data, "Ds[iSi]", "desc", 1, "two", 3);
  if (err) {
    printf("%s\n", pn_error(err));
  }
  pn_dbuf_print(data); printf("\n");
  pn_bytes_t str;
  err = pn_dbuf_scan(data, "D.[.S.]", &str);
  if (err) {
    printf("%s\n", pn_error(err));
  } else {
    printf("%.*s\n", (int) str.size, str.start);
  }
  pn_dbuf_free(data);

  return 0;
}

int value(int argc, char **argv)
{
  pn_atom_t dbuf[1024];
  pn_atoms_t atoms = {1024, dbuf};

  int err = pn_fill_atoms(&atoms, "DDsL[i[i]i]{sfsf}@DLT[sss]", "blam", 21, 1, 2, 3,
                          "pi", 3.14159265359, "e", 2.7,
                          (uint64_t) 42, PN_SYMBOL, "one", "two", "three");
  if (err) {
    printf("err = %s\n", pn_error(err));
  } else {
    pn_print_atoms(&atoms);
    printf("\n");
  }

  pn_bytes_t blam;
  uint64_t n;
  int32_t one, two, three;

  pn_bytes_t key1;
  float val1;

  pn_bytes_t key2;
  float val2;

  uint64_t al;
  pn_type_t type;

  bool threeq;

  pn_bytes_t sym;

  err = pn_scan_atoms(&atoms, "DDsL[i[i]?i]{sfsf}@DLT[.s.]", &blam, &n, &one, &two, &threeq, &three,
                      &key1, &val1, &key2, &val2, &al, &type, &sym);
  if (err) {
    printf("err = %s\n", pn_error(err));
  } else {
    printf("scan=%.*s %" PRIu64 " %i %i %i %.*s %f %.*s %f %" PRIu64 " %i %.*s\n", (int) blam.size,
	   blam.start, n, one, two, three, (int) key1.size, key1.start, val1, (int) key2.size,
	   key2.start, val2, al, threeq, (int) sym.size, sym.start);
  }

  char str[1024*10];
  int ch, idx = 0;
  while ((ch = getchar()) != EOF) {
    str[idx++] = ch;
  }
  str[idx] = '\0';

  /*  str[0] = '\0';
  for (int i = 2; i < argc; i++) {
    strcat(str, argv[i]);
    if (i < argc - 1)
      strcat(str, " ");
      }*/

  printf("JSON: %s\n", str);

  pn_atom_t jsondata[1024];
  pn_atoms_t jsond = {1024, jsondata};

  printf("\n");

  pn_json_t *json = pn_json();

  err = pn_json_parse(json, str, &jsond);
  if (err) {
    printf("parse err=%s, %s\n", pn_error(err), pn_json_error_str(json));
  } else {
    printf("--\n");
    for (int i = 0; i < jsond.size; i++) {
      printf("%s: ", pn_type_str(jsond.start[i].type));
      err = pn_print_atom(jsond.start[i]);
      if (err) printf("err=%s", pn_error(err));
      printf("\n");
    }
    printf("--\n");
    err = pn_print_atoms(&jsond);
    if (err) printf("print err=%s", pn_error(err));
    printf("\n");
  }

  pn_json_free(json);

  return 0;
}

struct server_context {
  int count;
};

void server_callback(pn_connector_t *ctor)
{
  pn_sasl_t *sasl = pn_connector_sasl(ctor);

  while (pn_sasl_state(sasl) != PN_SASL_PASS) {
    switch (pn_sasl_state(sasl)) {
    case PN_SASL_IDLE:
      return;
    case PN_SASL_CONF:
      pn_sasl_mechanisms(sasl, "PLAIN ANONYMOUS");
      pn_sasl_server(sasl);
      break;
    case PN_SASL_STEP:
      {
        size_t n = pn_sasl_pending(sasl);
        char iresp[n];
        pn_sasl_recv(sasl, iresp, n);
        printf("%s", pn_sasl_remote_mechanisms(sasl));
        printf(" response = ");
        pn_print_data(iresp, n);
        printf("\n");
        pn_sasl_done(sasl, PN_SASL_OK);
        pn_connector_set_connection(ctor, pn_connection());
      }
      break;
    case PN_SASL_PASS:
      break;
    case PN_SASL_FAIL:
      return;
    }
  }

  pn_connection_t *conn = pn_connector_connection(ctor);
  struct server_context *ctx = pn_connector_context(ctor);
  char tagstr[1024];
  char msg[1024];
  char data[1024];

  if (pn_connection_state(conn) == (PN_LOCAL_UNINIT | PN_REMOTE_ACTIVE)) {
    pn_connection_open(conn);
  }

  pn_session_t *ssn = pn_session_head(conn, PN_LOCAL_UNINIT | PN_REMOTE_ACTIVE);
  while (ssn) {
    pn_session_open(ssn);
    ssn = pn_session_next(ssn, PN_LOCAL_UNINIT | PN_REMOTE_ACTIVE);
  }

  pn_link_t *link = pn_link_head(conn, PN_LOCAL_UNINIT | PN_REMOTE_ACTIVE);
  while (link) {
    printf("%s, %s\n", pn_remote_source(link), pn_remote_target(link));
    pn_set_source(link, pn_remote_source(link));
    pn_set_target(link, pn_remote_target(link));
    pn_link_open(link);
    if (pn_is_receiver(link)) {
      pn_flow(link, 100);
    } else {
      pn_delivery(link, pn_dtag("blah", 4));
    }

    link = pn_link_next(link, PN_LOCAL_UNINIT | PN_REMOTE_ACTIVE);
  }

  pn_delivery_t *delivery = pn_work_head(conn);
  while (delivery)
  {
    pn_delivery_tag_t tag = pn_delivery_tag(delivery);
    pn_quote_data(tagstr, 1024, tag.bytes, tag.size);
    pn_link_t *link = pn_link(delivery);
    if (pn_readable(delivery)) {
      printf("received delivery: %s\n", tagstr);
      printf("  payload = \"");
      while (true) {
        ssize_t n = pn_recv(link, msg, 1024);
        if (n == PN_EOS) {
          pn_advance(link);
          pn_disposition(delivery, PN_ACCEPTED);
          break;
        } else {
          pn_print_data(msg, n);
        }
      }
      printf("\"\n");
    } else if (pn_writable(delivery)) {
      sprintf(msg, "message body for %s", tagstr);
      size_t n = pn_message_data(data, 1024, msg, strlen(msg));
      pn_send(link, data, n);
      if (pn_advance(link)) {
        printf("sent delivery: %s\n", tagstr);
        char tagbuf[16];
        sprintf(tagbuf, "%i", ctx->count++);
        pn_delivery(link, pn_dtag(tagbuf, strlen(tagbuf)));
      }
    }

    if (pn_updated(delivery)) {
      printf("disposition for %s: %u\n", tagstr, pn_remote_disp(delivery));
      pn_settle(delivery);
    }

    delivery = pn_work_next(delivery);
  }

  if (pn_connection_state(conn) == (PN_LOCAL_ACTIVE | PN_REMOTE_CLOSED)) {
    pn_connection_close(conn);
  }

  ssn = pn_session_head(conn, PN_LOCAL_ACTIVE | PN_REMOTE_CLOSED);
  while (ssn) {
    pn_session_close(ssn);
    ssn = pn_session_next(ssn, PN_LOCAL_ACTIVE | PN_REMOTE_CLOSED);
  }

  link = pn_link_head(conn, PN_LOCAL_ACTIVE | PN_REMOTE_CLOSED);
  while (link) {
    pn_link_close(link);
    link = pn_link_next(link, PN_LOCAL_ACTIVE | PN_REMOTE_CLOSED);
  }
}

struct client_context {
  bool init;
  bool done;
  int recv_count;
  int send_count;
  pn_driver_t *driver;
  const char *mechanism;
  const char *username;
  const char *password;
  const char *hostname;
  const char *address;
};

void client_callback(pn_connector_t *ctor)
{
  struct client_context *ctx = pn_connector_context(ctor);
  if (pn_connector_closed(ctor)) {
    ctx->done = true;
  }

  pn_sasl_t *sasl = pn_connector_sasl(ctor);

  while (pn_sasl_state(sasl) != PN_SASL_PASS) {
    pn_sasl_state_t st = pn_sasl_state(sasl);
    switch (st) {
    case PN_SASL_IDLE:
      return;
    case PN_SASL_CONF:
      if (ctx->mechanism && !strcmp(ctx->mechanism, "PLAIN")) {
        pn_sasl_plain(sasl, ctx->username, ctx->password);
      } else {
        pn_sasl_mechanisms(sasl, ctx->mechanism);
        pn_sasl_client(sasl);
      }
      break;
    case PN_SASL_STEP:
      if (pn_sasl_pending(sasl)) {
        fprintf(stderr, "challenge failed\n");
        ctx->done = true;
      }
      return;
    case PN_SASL_FAIL:
      fprintf(stderr, "authentication failed\n");
      ctx->done = true;
      return;
    case PN_SASL_PASS:
      break;
    }
  }

  pn_connection_t *connection = pn_connector_connection(ctor);
  char tagstr[1024];
  char msg[1024];
  char data[1024];

  if (!ctx->init) {
    ctx->init = true;

    char container[1024];
    if (gethostname(container, 1024)) pn_fatal("hostname lookup failed");

    pn_connection_set_container(connection, container);
    pn_connection_set_hostname(connection, ctx->hostname);

    pn_session_t *ssn = pn_session(connection);
    pn_connection_open(connection);
    pn_session_open(ssn);

    if (ctx->send_count) {
      pn_link_t *snd = pn_sender(ssn, "sender");
      pn_set_target(snd, ctx->address);
      pn_link_open(snd);

      char buf[16];
      for (int i = 0; i < ctx->send_count; i++) {
        sprintf(buf, "%c", 'a' + i);
        pn_delivery(snd, pn_dtag(buf, strlen(buf)));
      }
    }

    if (ctx->recv_count) {
      pn_link_t *rcv = pn_receiver(ssn, "receiver");
      pn_set_source(rcv, ctx->address);
      pn_link_open(rcv);
      pn_flow(rcv, ctx->recv_count);
    }
  }

  pn_delivery_t *delivery = pn_work_head(connection);
  while (delivery)
  {
    pn_delivery_tag_t tag = pn_delivery_tag(delivery);
    pn_quote_data(tagstr, 1024, tag.bytes, tag.size);
    pn_link_t *link = pn_link(delivery);
    if (pn_writable(delivery)) {
      sprintf(msg, "message body for %s", tagstr);
      ssize_t n = pn_message_data(data, 1024, msg, strlen(msg));
      pn_send(link, data, n);
      if (pn_advance(link)) printf("sent delivery: %s\n", tagstr);
    } else if (pn_readable(delivery)) {
      printf("received delivery: %s\n", tagstr);
      printf("  payload = \"");
      while (true) {
        size_t n = pn_recv(link, msg, 1024);
        if (n == PN_EOS) {
          pn_advance(link);
          pn_disposition(delivery, PN_ACCEPTED);
          pn_settle(delivery);
          if (!--ctx->recv_count) {
            pn_link_close(link);
          }
          break;
        } else {
          pn_print_data(msg, n);
        }
      }
      printf("\"\n");
    }

    if (pn_updated(delivery)) {
      printf("disposition for %s: %u\n", tagstr, pn_remote_disp(delivery));
      pn_clear(delivery);
      pn_settle(delivery);
      if (!--ctx->send_count) {
        pn_link_close(link);
      }
    }

    delivery = pn_work_next(delivery);
  }

  if (!ctx->send_count && !ctx->recv_count) {
    printf("closing\n");
    // XXX: how do we close the session?
    //pn_close((pn_endpoint_t *) ssn);
    pn_connection_close(connection);
  }

  if (pn_connection_state(connection) == (PN_LOCAL_CLOSED | PN_REMOTE_CLOSED)) {
    ctx->done = true;
  }
}

#include <ctype.h>

int main(int argc, char **argv)
{
  char *url = NULL;
  char *address = "queue";
  char *mechanism = "ANONYMOUS";

  int opt;
  while ((opt = getopt(argc, argv, "c:a:m:hVXY")) != -1)
  {
    switch (opt) {
    case 'c':
      if (url) pn_fatal("multiple connect urls not allowed\n");
      url = optarg;
      break;
    case 'a':
      address = optarg;
      break;
    case 'm':
      mechanism = optarg;
      break;
    case 'V':
      printf("proton version %i.%i\n", PN_VERSION_MAJOR, PN_VERSION_MINOR);
      exit(EXIT_SUCCESS);
    case 'X':
      value(argc, argv);
      exit(EXIT_SUCCESS);
    case 'Y':
      buffer(argc, argv);
      exit(EXIT_SUCCESS);
    case 'h':
      printf("Usage: %s [-h] [-c [user[:password]@]host[:port]] [-a <address>] [-m <sasl-mech>]\n", argv[0]);
      printf("\n");
      printf("    -c    The connect url.\n");
      printf("    -a    The AMQP address.\n");
      printf("    -m    The SASL mechanism.\n");
      printf("    -h    Print this help.\n");
      exit(EXIT_SUCCESS);
    default: /* '?' */
      pn_fatal("Usage: %s -h\n", argv[0]);
    }
  }

  char *user = NULL;
  char *pass = NULL;
  char *host = "0.0.0.0";
  char *port = "5672";

  parse_url(url, &user, &pass, &host, &port);

  pn_driver_t *drv = pn_driver();
  if (url) {
    struct client_context ctx = {false, false, 10, 10, drv};
    ctx.username = user;
    ctx.password = pass;
    ctx.mechanism = mechanism;
    ctx.hostname = host;
    ctx.address = address;
    pn_connector_t *ctor = pn_connector(drv, host, port, &ctx);
    if (!ctor) pn_fatal("connector failed\n");
    pn_connector_set_connection(ctor, pn_connection());
    while (!ctx.done) {
      pn_driver_wait(drv, -1);
      pn_connector_t *c;
      while ((c = pn_driver_connector(drv))) {
        pn_connector_process(c);
        client_callback(c);
        if (pn_connector_closed(c)) {
	  pn_connection_destroy(pn_connector_connection(c));
          pn_connector_destroy(c);
        } else {
          pn_connector_process(c);
        }
      }
    }
  } else {
    struct server_context ctx = {0};
    if (!pn_listener(drv, host, port, &ctx)) pn_fatal("listener failed\n");
    while (true) {
      pn_driver_wait(drv, -1);
      pn_listener_t *l;
      pn_connector_t *c;

      while ((l = pn_driver_listener(drv))) {
        c = pn_listener_accept(l);
        pn_connector_set_context(c, &ctx);
      }

      while ((c = pn_driver_connector(drv))) {
        pn_connector_process(c);
        server_callback(c);
        if (pn_connector_closed(c)) {
	  pn_connection_destroy(pn_connector_connection(c));
          pn_connector_destroy(c);
        } else {
          pn_connector_process(c);
        }
      }
    }
  }

  pn_driver_destroy(drv);

  return 0;
}
