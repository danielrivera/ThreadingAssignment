/*
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

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "data_store.h"

static void data_block_init(data_block_t* data_block, const char* data, int data_size);

/**
 * This member function calls pthread_rwlock_rdlock on the
 * pthread_rwlock of a data_store_t structure.
 */
static int __data_store_rd_lock(data_store_t* data_store) {
    assert(data_store);
    return pthread_rwlock_rdlock(&data_store->__rwlock);
}

/**
 * This member function calls pthread_rwlock_unlock on the
 * pthread_rwlock of a data_store_t structure.
 */
static int __data_store_unlock(data_store_t* data_store) {
    assert(data_store);
    return pthread_rwlock_unlock(&data_store->__rwlock);
}

/**
 * This member function adds a data block to the data store.
 */
static void __data_store_add_data_block(data_store_t* data_store, const char* buffer, int buffer_size) {
    assert(data_store);

    // Firstly we want to get a write lock.
    pthread_rwlock_wrlock(&data_store->__rwlock);

    // We got the write lock, let's modify the data struct to add a new data block.
    data_store->__data_block_count++;
    data_store->__data_blocks = realloc(data_store->__data_blocks, data_store->__data_block_count * sizeof(data_block_t));

    // Initialize the new data block with the data provided in the arguments.
    data_block_init(&data_store->__data_blocks[data_store->__data_block_count - 1], buffer, buffer_size);

    // We're done, now unlock the read lock and increment the semaphore by 1 to indicate a new block is available.
    data_store->unlock(data_store);
    sem_post(&data_store->__data_blocks_available_sem);
}

/**
 * This member function processes a data block using the provided callback function.
 */
static int __data_block_process(data_block_t* data_block, void (*process_data_callback)(const char* data, int buffer_size)) {
    assert(data_block);

    // First we need to check if the block has been processed, and if not then set it to processed.
    // This has to be done atomically or else another thread might grab it in between the processed
    // state checking and setting.
    if (EOK == pthread_mutex_trylock(&data_block->__processed_mutex)) {
        // We got the mutex, destroy it immediately which will indicate to other threads
        // that this block is processed.
        pthread_mutex_destroy(&data_block->__processed_mutex);

        // Call the process callback function provided.
        if (process_data_callback) {
            process_data_callback(data_block->__data, data_block->__data_size);
        }

        // If there was any data, free it.
        if (data_block->__data) {
            free(data_block->__data);
            data_block->__data = NULL;
            data_block->__data_size = 0;
        }

        return 1;
    } else {
        // The mutex wasn't good meaning that this blocked was processed.
        return 0;
    }
}

/**
 * Initializes a data_block instance with the data provided in the arguments.
 */
static void data_block_init(data_block_t* data_block, const char* data, int data_size) {
    assert(data_block);

    // Initialize it to 0
    memset(data_block, 0, sizeof(data_block_t));

    // Create a processed flag mutex for it
    pthread_mutex_init(&data_block->__processed_mutex, NULL);

    // Assign the process function
    data_block->process = __data_block_process;

    // Initialize the actual data buffer.
    if (data && data_size > 0) {
        data_block->__data = memcpy(malloc(data_size), data, data_size);
        data_block->__data_size = data_size;
    }
}

/**
 * This helper function creates ready to use data_store_t
 * structures.
 */
data_store_t* data_store_create() {
    data_store_t* retval = malloc(sizeof(data_store_t));

    // Initialize the sync tools.
    pthread_rwlock_init(&retval->__rwlock, NULL);
    sem_init(&retval->__data_blocks_available_sem, 0, 0);

    // Assign the member functions.
    retval->rd_lock = __data_store_rd_lock;
    retval->unlock = __data_store_unlock;
    retval->add_data_block = __data_store_add_data_block;

    // Init the data blocks.
    retval->__data_block_count = 0;
    retval->__data_blocks = NULL;

    return retval;
}

/**
 * This helper function cleans up a data_store structure
 * and then frees it.
 */
void data_store_destroy(data_store_t* data_store) {
    assert(data_store);

    // Lock the data store for writing.
    pthread_rwlock_wrlock(&data_store->__rwlock);

    // Destroy the data blocks.
    if (data_store->__data_blocks) {
        //TODO Clean up all the data blocks
        free(data_store->__data_blocks);
        data_store->__data_blocks = NULL;
        data_store->__data_block_count = 0;
    }

    // Destroy the sync tools.
    pthread_rwlock_destroy(&data_store->__rwlock);
    sem_destroy(&data_store->__data_blocks_available_sem);

    // Finally, free the data store.
    free(data_store);
}
