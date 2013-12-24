/*
 * Copyright (C) 2011-2012 Gilles Chanteperdrix <gch@xenomai.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <mqueue.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/mman.h>

#include "check.h"

static const char *tunes[] = {
    "Surfing With The Alien",
    "Lords of Karma",
    "Banana Mango",
    "Psycho Monkey",
    "Luminous Flesh Giants",
    "Moroccan Sunset",
    "Satch Boogie",
    "Flying In A Blue Dream",
    "Ride",
    "Summer Song",
    "Speed Of Light",
    "Crystal Planet",
    "Raspberry Jam Delta-V",
    "Champagne?",
    "Clouds Race Across The Sky",
    "Engines Of Creation"
};

static void *task(void *cookie)
{
	mqd_t mqd = (mqd_t)(long)cookie;
	fd_set inset;
	unsigned i;

	FD_ZERO(&inset);
	FD_SET(mqd, &inset);

	for(i = 0; i < sizeof(tunes)/sizeof(tunes[0]); i++) {
		fd_set tmp_inset = inset;
		unsigned prio;
		char buf[128];

		check_unix(select(mqd + 1, &tmp_inset, NULL, NULL, NULL));

		check_unix(mq_receive(mqd, buf, sizeof(buf), &prio));

		if (strcmp(buf, tunes[i])) {
			fprintf(stderr, "Received %s instead of %s\n",
				buf, tunes[i]);
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "Received %s\n", buf);
	}

	return NULL;
}

int main(void)
{
	struct mq_attr qa;
	pthread_t tcb;
	mqd_t mq;
	int i;

	fprintf(stderr, "Checking select service with posix message queues\n");

	mq_unlink("/select_test_mq");

	qa.mq_maxmsg = 128;
	qa.mq_msgsize = 128;
	mq = mq_open("/select_test_mq", O_RDWR | O_CREAT | O_NONBLOCK, 0, &qa);
	check_unix(mq == -1 ? -1 : 0);

	check_pthread(pthread_create(&tcb, NULL, task, (void *)(long)mq));

	alarm(30);

	for(i = 0; i < sizeof(tunes) / sizeof(tunes[0]); i++) {
		check_unix(mq_send(mq, tunes[i], strlen(tunes[i]) + 1, 0));

		sleep(1);
	}

	check_pthread(pthread_join(tcb, NULL));

	fprintf(stderr, "select service with posix message queues: success\n");

	return EXIT_SUCCESS;
}
