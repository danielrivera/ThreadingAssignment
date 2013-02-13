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

#include <semaphore.h>

/**
 * A data block structure for the data store structure.
 */
struct data_block_t {
    pthread_mutex_t __processed_mutex; // If the mutex is valid it means it was not processed
    int (*process)(struct data_block_t *, void (*process_data_callback)(const char* data, int buffer_size)); // returns 1 on sucess, 0 on failure
    char* __data;
    int __data_size;
};

typedef struct data_block_t data_block_t;

/**
 * A data store structure.
 */
struct data_store_t {
    pthread_rwlock_t __rwlock;
    sem_t __data_available_sem;
    int (*wr_lock)(struct data_store_t *);
    int (*rd_lock)(struct data_store_t *);
    int (*unlock)(struct data_store_t *);
    void (*add_data_block)(struct data_store_t *, const char* buffer, int buffer_size);
    data_block_t* __data_blocks;
    int __data_block_count;
};

typedef struct data_store_t data_store_t;

/**
 * This helper function creates ready to use data_store_t
 * structures.
 */
data_store_t* data_store_create();

/**
 * This helper function cleans up a data_store structure
 * and then frees it.
 */
void data_store_destroy(data_store_t* data_store);

