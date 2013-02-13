/*
* Copyright (c) 2011-2012 Research In Motion Limited.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <bps/bps.h>
#include <bps/event.h>
#include <bps/navigator.h>
#include <bps/screen.h>
#include <fcntl.h>
#include <screen/screen.h>
#include <pthread.h>
#include <stdlib.h>

static bool shutdown;

static void
handle_screen_event(bps_event_t *event)
{
    int screen_val;

    screen_event_t screen_event = screen_event_get_event(event);
    screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &screen_val);

    switch (screen_val) {
    case SCREEN_EVENT_MTOUCH_TOUCH:
        fprintf(stderr,"Touch event");
        break;
    case SCREEN_EVENT_MTOUCH_MOVE:
        fprintf(stderr,"Move event");
        break;
    case SCREEN_EVENT_MTOUCH_RELEASE:
        fprintf(stderr,"Release event");
        break;
    default:
        break;
    }
    fprintf(stderr,"\n");
}

static void
handle_navigator_event(bps_event_t *event) {
    switch (bps_event_get_code(event)) {
    case NAVIGATOR_SWIPE_DOWN:
        fprintf(stderr,"Swipe down event");
        break;
    case NAVIGATOR_EXIT:
        fprintf(stderr,"Exit event");
        shutdown = true;
        break;
    default:
        break;
    }
    fprintf(stderr,"\n");
}

static void
handle_event()
{
    int domain;

    bps_event_t *event = NULL;
    if (BPS_SUCCESS != bps_get_event(&event, -1)) {
        fprintf(stderr, "bps_get_event() failed\n");
        return;
    }
    if (event) {
        domain = bps_event_get_domain(event);
        if (domain == navigator_get_domain()) {
            handle_navigator_event(event);
        } else if (domain == screen_get_domain()) {
            handle_screen_event(event);
        }
    }
}

//TODO Define global data structures to be used

/**
 * The shared data structure.
 */
typedef struct data_store_t {
    pthread_rwlock_t m_lock;
    int (*wr_lock)(struct data_store_t *);
    int (*rd_lock)(struct data_store_t *);
    int (*unlock)(struct data_store_t *);
} data_store_t;

/**
 * This helper function calls pthread_rwlock_rdlock on the
 * m_lock of a data_store_t structure.
 */
int rd_lock_data_store(data_store_t* data_store) {
    return pthread_rwlock_rdlock(&data_store->m_lock);
}

/**
 * This helper function calls pthread_rwlock_wrlock on the
 * m_lock of a data_store_t structure.
 */
int wr_lock_data_store(data_store_t* data_store) {
    return pthread_rwlock_wrlock(&data_store->m_lock);
}

/**
 * This helper function calls pthread_rwlock_unlock on the
 * m_lock of a data_store_t structure.
 */
int unlock_data_store(data_store_t* data_store) {
    return pthread_rwlock_unlock(&data_store->m_lock);
}

/**
 * This helper function creates ready to use data_store_t
 * structures.
 */
data_store_t* create_data_store() {
    data_store_t* retval = malloc(sizeof(data_store_t));

    pthread_rwlock_init(&retval->m_lock, NULL);
    retval->wr_lock = wr_lock_data_store;
    retval->rd_lock = rd_lock_data_store;
    retval->unlock = unlock_data_store;

    return retval;
}

/**
 * This helper function cleans up a data_store structure
 * and then frees it.
 */
void destroy_data_store(data_store_t* data_store) {
    if(NULL != data_store) {
        pthread_rwlock_destroy(&data_store->m_lock);
        free(data_store);
    }
}

/**
 * This thread is responsible for pulling data off of the shared data
 * area and processing it using the process_data() API.
 */
void *reader_thread(void *arg) {
    //TODO: Define set-up required
    data_store_t* shared_data = (data_store_t*)arg;

    while(1) {
        //TODO: Define data extraction (queue) and processing
    }

    return NULL;
}

/**
 * This thread is responsible for pulling data from a device using the
 * get_external_data() API and placing it into a shared area for later
 * processing by one of the reader threads.
 */
void *writer_thread(void *arg) {
    //TODO: Define set-up required
    data_store_t* shared_data = (data_store_t*)arg;

    while(1) {
        //TODO: Define data extraction (device) and storage
    }

    return NULL;
}

#define M 10
#define N 20

int main(int argc, char **argv)
{
    int i;
    data_store_t* shared_data = create_data_store();

    for(i = 0; i < N; i++) {
        pthread_create(NULL, NULL, reader_thread, shared_data);
    }

    for(i = 0; i < M; i++) {
        pthread_create(NULL, NULL, writer_thread, shared_data);
    }

    const int usage = SCREEN_USAGE_NATIVE;

    screen_context_t screen_ctx;
    screen_window_t screen_win;
    screen_buffer_t screen_buf = NULL;
    int rect[4] = { 0, 0, 0, 0 };

    /* Setup the window */
    screen_create_context(&screen_ctx, 0);
    screen_create_window(&screen_win, screen_ctx);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, &usage);
    screen_create_window_buffers(screen_win, 1);

    screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)&screen_buf);
    screen_get_window_property_iv(screen_win, SCREEN_PROPERTY_BUFFER_SIZE, rect+2);

    /* Fill the screen buffer with blue */
    int attribs[] = { SCREEN_BLIT_COLOR, 0xff0000ff, SCREEN_BLIT_END };
    screen_fill(screen_ctx, screen_buf, attribs);
    screen_post_window(screen_win, screen_buf, 1, rect, 0);

    /* Signal bps library that navigator and screen events will be requested */
    bps_initialize();
    screen_request_events(screen_ctx);
    navigator_request_events(0);

    while (!shutdown) {
        /* Handle user input */
        handle_event();
    }

    /* Clean up */
    // TODO stop all the read/write threads
    destroy_data_store(shared_data);
    screen_stop_events(screen_ctx);
    bps_shutdown();
    screen_destroy_window(screen_win);
    screen_destroy_context(screen_ctx);
    return 0;
}

