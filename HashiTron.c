#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_KEY_SIZE  64
#define MAX_VALUE_SIZE 256
#define HASH_BUCKETS  128
#define LOG_FILE_NAME "kvstore.log"
#define STORE_FILE_NAME "kvstore.data"

/* 
 * Data structure representing a single record in the store.
 * We keep key and value as fixed-size buffers for simplicity. 
 */
typedef struct Record {
    char key[MAX_KEY_SIZE];
    char value[MAX_VALUE_SIZE];
} Record;

/*
 * Hash Bucket for in-memory index. 
 * We store a pointer to a Record on disk, but for simplicity
 * we embed the record pointer in memory (you might store file offsets instead).
 */
typedef struct Bucket {
    Record record;
    struct Bucket *next;
} Bucket;

/* 
 * Our DB structure: 
 * - an array of Bucket pointers for the hash index
 * - a file handle for writing and reading the store data
 * - a file handle for a write-ahead log
 * - a mutex for concurrency
 */
typedef struct DB {
    Bucket *buckets[HASH_BUCKETS];
    FILE *data_file;
    FILE *log_file;
    pthread_mutex_t db_lock;
} DB;

/*
 * Simple hash function on the key to select a bucket 
 */
static unsigned int hash_key(const char *key) {
    unsigned int hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash % HASH_BUCKETS;
}

/*
 * Write the operation to the WAL (write-ahead log) for potential rollback.
 * Format: "PUT key value\n" or "DELETE key\n"
 */
static void log_operation(DB *db, const char *op, const char *key, const char *value) {
    if (!db->log_file) return;
    if (value) {
        fprintf(db->log_file, "%s %s %s\n", op, key, value);
    } else {
        fprintf(db->log_file, "%s %s\n", op, key);
    }
    fflush(db->log_file);
}

/*
 * Apply a "PUT" to the data file. For real usage, we'd store offsets, 
 * checksums, or length prefixes. Here, we simply append Record struct. 
 */
static bool write_record_to_data(DB *db, const Record *rec) {
    if (!db->data_file) return false;
    if (fwrite(rec, sizeof(Record), 1, db->data_file) != 1) {
        return false;
    }
    fflush(db->data_file);
    return true;
}

/*
 * Insert or update the record in the in-memory hash index. 
 */
static void insert_into_index(DB *db, const Record *rec) {
    unsigned int bucket_idx = hash_key(rec->key);
    Bucket *head = db->buckets[bucket_idx];

    for (Bucket *b = head; b != NULL; b = b->next) {
        if (strcmp(b->record.key, rec->key) == 0) {
            strcpy(b->record.value, rec->value);
            return;
        }
    }
    /* Not found, create a new bucket node. */
    Bucket *new_bucket = (Bucket *)calloc(1, sizeof(Bucket));
    memcpy(&new_bucket->record, rec, sizeof(Record));
    new_bucket->next = head;
    db->buckets[bucket_idx] = new_bucket;
}

/*
 * DB Initialization: open files, create empty index, recover from log if needed.
 */
DB *db_init(const char *data_path, const char *log_path) {
    DB *db = (DB *)calloc(1, sizeof(DB));
    pthread_mutex_init(&db->db_lock, NULL);

    /* Open data file and log file (append mode) */
    db->data_file = fopen(data_path, "ab+");
    if (!db->data_file) {
        perror("Unable to open data file");
        free(db);
        return NULL;
    }

    db->log_file = fopen(log_path, "ab+");
    if (!db->log_file) {
        perror("Unable to open log file");
        fclose(db->data_file);
        free(db);
        return NULL;
    }

    /* 
     * Optional: read existing data file to build the in-memory index, 
     * or replay the log to restore. 
     * For demonstration, we skip a thorough replay mechanism.
     */
    return db;
}

/*
 * "Begin transaction" is trivial here; we rely on the WAL to track changes.
 * For advanced usage, you’d maintain a transaction id or sequence number. 
 */
static void begin_transaction(DB *db) {
    pthread_mutex_lock(&db->db_lock);
    /* Mark the start of a transaction in the log. */
    fprintf(db->log_file, "BEGIN\n");
    fflush(db->log_file);
}

/*
 * "Commit transaction": finalize changes. In a real system, 
 * you'd use careful ordering (fsync calls, etc.).
 */
static void commit_transaction(DB *db) {
    fprintf(db->log_file, "COMMIT\n");
    fflush(db->log_file);
    pthread_mutex_unlock(&db->db_lock);
}

/*
 * "Rollback transaction": naive approach is to simply write a ROLLBACK marker.
 * A real approach would require storing old values so we could revert them in the index and data file.
 */
static void rollback_transaction(DB *db) {
    fprintf(db->log_file, "ROLLBACK\n");
    fflush(db->log_file);
    pthread_mutex_unlock(&db->db_lock);
}

/*
 * Put a key-value pair into the store. 
 * In a real system, you'd also store a transaction ID and handle concurrency carefully.
 */
bool db_put(DB *db, const char *key, const char *value) {
    Record rec;
    memset(&rec, 0, sizeof(Record));
    strncpy(rec.key, key, MAX_KEY_SIZE-1);
    strncpy(rec.value, value, MAX_VALUE_SIZE-1);

    /* Log the operation before doing the actual write (WAL principle). */
    log_operation(db, "PUT", key, value);

    if (!write_record_to_data(db, &rec)) {
        return false;
    }

    /* Update in-memory index. */
    insert_into_index(db, &rec);
    return true;
}

/*
 * Get a value from the store. 
 */
char *db_get(DB *db, const char *key) {
    unsigned int bucket_idx = hash_key(key);
    Bucket *b = db->buckets[bucket_idx];
    while (b) {
        if (strcmp(b->record.key, key) == 0) {
            return b->record.value;
        }
        b = b->next;
    }
    return NULL;
}

/*
 * Delete a key from the store (naive approach). 
 * We simply log the delete, remove it from memory, no immediate data-file compaction.
 */
bool db_delete(DB *db, const char *key) {
    log_operation(db, "DELETE", key, NULL);

    unsigned int bucket_idx = hash_key(key);
    Bucket *b = db->buckets[bucket_idx];
    Bucket *prev = NULL;
    while (b) {
        if (strcmp(b->record.key, key) == 0) {
            if (prev) {
                prev->next = b->next;
            } else {
                db->buckets[bucket_idx] = b->next;
            }
            free(b);
            return true;
        }
        prev = b;
        b = b->next;
    }
    return false;
}

void db_close(DB *db) {
    if (!db) return;
    /* In a real scenario, ensure all transactions are committed */
    fclose(db->data_file);
    fclose(db->log_file);
    pthread_mutex_destroy(&db->db_lock);

    /* Free in-memory index. */
    for (int i = 0; i < HASH_BUCKETS; i++) {
        Bucket *b = db->buckets[i];
        while (b) {
            Bucket *next = b->next;
            free(b);
            b = next;
        }
    }
    free(db);
}

/* 
 * Demo usage 
 */
int main(void) {
    DB *db = db_init(STORE_FILE_NAME, LOG_FILE_NAME);
    if (!db) {
        fprintf(stderr, "Failed to initialize DB.\n");
        return 1;
    }

    begin_transaction(db);
    db_put(db, "foo", "Hello, World!");
    db_put(db, "bar", "C programming is fun.");
    commit_transaction(db);

    printf("GET foo: %s\n", db_get(db, "foo"));
    printf("GET bar: %s\n", db_get(db, "bar"));

    begin_transaction(db);
    db_delete(db, "foo");
    rollback_transaction(db); /* Just rolling back to demonstrate */

    printf("After rollback, GET foo: %s\n", db_get(db, "foo"));

    db_close(db);
    return 0;
}
