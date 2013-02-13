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

/**
 * A data store structure.
 */
struct data_store_t {
    pthread_rwlock_t pthread_rwlock;
    int (*wr_lock)(struct data_store_t *);
    int (*rd_lock)(struct data_store_t *);
    int (*unlock)(struct data_store_t *);
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

