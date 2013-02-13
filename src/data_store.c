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

#include <pthread.h>
#include <stdlib.h>
#include "data_store.h"

/**
 * This helper function calls pthread_rwlock_rdlock on the
 * m_lock of a data_store_t structure.
 */
static int rd_lock_data_store(data_store_t* data_store) {
    return pthread_rwlock_rdlock(&data_store->m_lock);
}

/**
 * This helper function calls pthread_rwlock_wrlock on the
 * m_lock of a data_store_t structure.
 */
static int wr_lock_data_store(data_store_t* data_store) {
    return pthread_rwlock_wrlock(&data_store->m_lock);
}

/**
 * This helper function calls pthread_rwlock_unlock on the
 * m_lock of a data_store_t structure.
 */
static int unlock_data_store(data_store_t* data_store) {
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
