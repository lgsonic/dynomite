#include <dyn_core.h>

#ifdef DN_HAVE_SELECT

struct event_base *
event_base_create(int nevent, event_cb_t cb)
{
    struct event_base *evb;
    struct select_event *event;

    ASSERT(nevent > 0);

    event = dn_calloc(nevent, sizeof(*event));
    if (event == NULL) {
        return NULL;
    }

    evb = dn_alloc(sizeof(*evb));
    if (evb == NULL) {
        dn_free(event);
        return NULL;
    }

    evb->event = event;
    evb->nevent = nevent;
	evb->totalsds = 0;
    evb->cb = cb;

	FD_ZERO(&evb->readset);
	FD_ZERO(&evb->writeset);
	FD_ZERO(&evb->errset);

    log_debug(LOG_INFO, "select with nevent %d", evb->nevent);

    return evb;
}

void
event_base_destroy(struct event_base *evb)
{
    if (evb == NULL) {
        return;
    }

    dn_free(evb->event);

    dn_free(evb);
}

int
event_add_in(struct event_base *evb, struct conn *c)
{
    int status;

    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

    if (c->recv_active) {
        return 0;
    }

	log_debug(LOG_DEBUG, "adding conn %p(%s) to recv", c, conn_get_type_string(c));
#ifdef WIN32
	FD_SET(c->sd, &evb->readset);
	status = 0;
#else
	status = FD_SET(c->sd, &evb->readset);
#endif
    if (status < 0) {
        log_error("select ctl on sd %d failed: %s", c->sd,
                  socket_strerror(errno));
    } else {
        c->recv_active = 1;
    }

    return status;
}

int
event_del_in(struct event_base *evb, struct conn *c)
{
	int status;

	ASSERT(c != NULL);
	ASSERT(c->sd > 0);
	ASSERT(c->recv_active);

	if (!c->recv_active) {
		return 0;
	}

	log_debug(LOG_DEBUG, "removing conn %p(%s) from recv", c, conn_get_type_string(c));
#ifdef WIN32
	FD_CLR(c->sd, &evb->readset);
	status = 0;
#else
	status = FD_CLR(c->sd, &evb->readset);
#endif
	if (status < 0) {
		log_error("select ctl on sd %d failed: %s", c->sd,
			socket_strerror(errno));
	}
	else {
		c->recv_active = 0;
	}

	return status;
}

int
event_add_out(struct event_base *evb, struct conn *c)
{
    int status;

    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (c->send_active) {
        return 0;
    }

    log_debug(LOG_DEBUG, "adding conn %p(%s) to active", c, conn_get_type_string(c));
#ifdef WIN32
	FD_SET(c->sd, &evb->writeset);
	status = 0;
#else
	status = FD_SET(c->sd, &evb->writeset);
#endif
    if (status < 0) {
        log_error("select ctl on sd %d failed: %s", c->sd,
                  socket_strerror(errno));
    } else {
        c->send_active = 1;
    }

    return status;
}

int
event_del_out(struct event_base *evb, struct conn *c)
{
    int status;

    ASSERT(c != NULL);
    ASSERT(c->sd > 0);
    ASSERT(c->recv_active);

    if (!c->send_active) {
        return 0;
    }

    log_debug(LOG_DEBUG, "removing conn %p(%s) from active", c, conn_get_type_string(c));
#ifdef WIN32
	FD_CLR(c->sd, &evb->writeset);
	status = 0;
#else
	status = FD_CLR(c->sd, &evb->writeset);
#endif
    if (status < 0) {
        log_error("select ctl on sd %d failed: %s", c->sd,
                  socket_strerror(errno));
    } else {
        c->send_active = 0;
    }

    return status;
}

int
event_add_conn(struct event_base *evb, struct conn *c)
{
    int status;

    ASSERT(c != NULL);
    ASSERT(c->sd > 0);

	if (evb->totalsds >= evb->nevent) {
		log_error("select ctl sd %d failed: events exceed limit", c->sd);
	}

	status = event_add_in(evb, c);
	if (status < 0) {
		return status;
	} 
	
	status = event_add_out(evb, c);
	if (status < 0) {
		return status;
	}

#ifdef WIN32
	FD_SET(c->sd, &evb->errset);
#endif

	evb->event[evb->totalsds].sd = c->sd;
	evb->event[evb->totalsds].data = c;
	evb->totalsds++;

    return status;
}

int
event_del_conn(struct event_base *evb, struct conn *c)
{
	ASSERT(c != NULL);
    ASSERT(c->sd > 0);

	for (int i = 0; i < evb->totalsds; i++) {
		if (evb->event[i].sd == c->sd) {
			event_del_in(evb, c);
			event_del_out(evb, c);

#ifdef WIN32
			FD_CLR(c->sd, &evb->errset);
#endif

			for (int j = i; j < evb->totalsds - 1; j++)
			{
				evb->event[j] = evb->event[j + 1];
			}

			evb->totalsds--;
			
			break;
		}
	}

    return 0;
}

int
event_wait(struct event_base *evb, int timeout)
{
	struct timeval tv;
    struct select_event *event = evb->event;
    int nevent = evb->nevent;
	fd_set readset;
	fd_set writeset;
	fd_set errset;

    ASSERT(event != NULL);
    ASSERT(nevent > 0);

    for (;;) {
        int i, nsd;

		memcpy(&readset, &evb->readset, sizeof(fd_set));
		memcpy(&writeset, &evb->writeset, sizeof(fd_set));
		memcpy(&errset, &evb->errset, sizeof(fd_set));
		tv.tv_sec = timeout/1000;
		tv.tv_usec = 0;
		nsd = select(1024, &readset, &writeset, &errset, &tv);
        if (nsd > 0) {
            for (i = 0; i < evb->totalsds; i++) {
				struct select_event *ev = &evb->event[i];
                uint32_t events = 0;

                log_debug(LOG_VVVERB, "select %d triggered on conn %p",
                          ev->sd, ev->data);

				if (FD_ISSET(ev->sd, &readset)) {
                    events |= EVENT_READ;
                }

				if (FD_ISSET(ev->sd, &writeset)) {
                    events |= EVENT_WRITE;
                }

				if (FD_ISSET(ev->sd, &errset)) {
					events |= EVENT_ERR;
				}

                if (evb->cb != NULL) {
                    evb->cb(ev->data, events);
                }
            }
            return nsd;
        }

        if (nsd == 0) {
            if (timeout == -1) {
               log_error("select wait with %d events and %d timeout "
                         "returned no events", nevent, timeout);
                return -1;
            }

            return 0;
        }
#ifdef WIN32
		if (errno == WSAEINTR) {
#else
        if (errno == EINTR) {
#endif
            continue;
        }

        log_error("select wait with %d events failed: %s", nevent,
                  socket_strerror(errno));
        return -1;
    }

    NOT_REACHED();
}

void
event_loop_stats(event_stats_cb_t cb, void *arg)
{
    struct stats *st = arg;
    int status;
	fd_set readset;

	FD_ZERO(&readset);
#ifdef WIN32
	FD_SET(st->sd, &readset);
	status = 0;
#else
	status = FD_SET(st->sd, &readset);
#endif
    if (status < 0) {
        log_error("select ctl on sd %d failed: %s", st->sd,
                  socket_strerror(errno));
        goto error;
    }

    for (;;) {
        int n;
		struct timeval tv = { st->interval, 0 };

		n = select(0, &readset, NULL, NULL, &tv);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            log_error("select wait with m %d failed: %s",
                      st->sd, socket_strerror(errno));
            break;
        }

        cb(st, &n);
    }

error:
#ifdef WIN32
	FD_CLR(st->sd, &readset);
	status = 0;
#else
	FD_CLR(st->sd, &readset);
#endif
    if (status < 0) {
		log_error("select ctl on sd %d failed, ignored: %s", st->sd, socket_strerror(errno));
    }
}

void
event_loop_entropy(event_entropy_cb_t cb, void *arg)
{
    struct entropy *ent = arg;
	int status;
    ent->interval = 30;
	fd_set readset;

	FD_ZERO(&readset);
#ifdef WIN32
	FD_SET(ent->sd, &readset);
	status = 0;
#else
	status = FD_SET(ent->sd, &readset);
#endif
    if (status < 0) {
        log_error("entropy select ctl on sd %d failed: %s", ent->sd,
                  socket_strerror(errno));
        goto error;
    }

    for (;;) {
        int n;
		struct timeval tv = { ent->interval, 0 };

		n = select(0, &readset, NULL, NULL, &tv);
        if (n < 0) {
#ifdef WIN32
			if (errno == WSAEINTR) {
#else
			if (errno == EINTR) {
#endif
                continue;
            }
            log_error("entropy select wait with m %d failed: %s",
            		ent->sd, socket_strerror(errno));
            break;
        }

        cb(ent, &n);
    }

error:
#ifdef WIN32
	FD_CLR(ent->sd, &readset);
	status = 0;
#else
	FD_CLR(ent->sd, &readset);
#endif
    if (status < 0) {
		log_error("select ctl on sd %d failed, ignored: %s", ent->sd, socket_strerror(errno));
    }
}

#endif /* DN_HAVE_SELECT */
