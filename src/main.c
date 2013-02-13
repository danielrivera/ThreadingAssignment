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
#include "data_store.h"

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

/**
 * Make-believe function for getting external data, where buffer
 * is a pointer to memory of bufferSizeInBytes that can be filled
 * in. The return value indicates the number of bytes that have
 * been filled in, or < 0 on error.
 */
int get_external_data(char *buffer, int bufferSizeInBytes) {
    if(buffer) {
        //TODO Get some real data
        return bufferSizeInBytes; // Just pretend we filled the whole buffer
    }

    return -1; // Error
}

/**
 * Make-believe function for processing data in the data store,
 * where buffer is a pointer to the data to be processed with
 * a length of bufferSizeInBytes.
 */
void process_data(const char *buffer, int bufferSizeInBytes) {
    //TODO Do some funky processing.
}

/**
 * This thread is responsible for pulling data off of the shared data
 * area and processing it using the process_data() API.
 */
void *reader_thread(void *arg) {
    data_store_t* shared_data = (data_store_t*)arg;
    int data_block_index = 0;

    while(shared_data) {
        // Wait until some data is available
        sem_wait(&shared_data->__data_blocks_available_sem);

        // Lock for reading
        shared_data->rd_lock(shared_data);

        // Try to process a block until the end of the data set
        while (data_block_index < shared_data->__data_block_count) {
            data_block_t* data_block = &shared_data->__data_blocks[data_block_index];

            if (data_block->process(data_block, process_data)) {
                // The block was processed, exit
                break;
            }

            data_block_index++;
        }

        // Done processing a block, unlock the read lock.
        shared_data->unlock(shared_data);
    }

    return NULL;
}

/**
 * This thread is responsible for pulling data from a device using the
 * get_external_data() API and placing it into a shared area for later
 * processing by one of the reader threads.
 */
void *writer_thread(void *arg) {
    data_store_t* shared_data = (data_store_t*)arg;

    while(shared_data) {
        const int buffer_size = 256;
        char buffer[buffer_size];

        if (get_external_data(buffer, buffer_size) > 0) {
            // We got some data, add it to the data structure.
            shared_data->add_data_block(shared_data, buffer, buffer_size);
        }
    }

    return NULL;
}

#define M 10
#define N 20

int main(int argc, char **argv)
{
    int i;
    data_store_t* shared_data = data_store_create();

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
    // TODO Stop all the read/write threads
    data_store_destroy(shared_data);
    screen_stop_events(screen_ctx);
    bps_shutdown();
    screen_destroy_window(screen_win);
    screen_destroy_context(screen_ctx);
    return 0;
}

