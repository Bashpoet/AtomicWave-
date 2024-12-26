# AtomicWave: A Lean, Mean, In-Memory Key-Value Store (with Disk Persistence)

**AtomicWave** is a lightweight key-value database built in C, designed to be a hands-on learning tool and a platform for experimentation with core database concepts. It combines an in-memory hash index for fast lookups with a disk-based data store for persistence.  This project aims to provide a clear and understandable implementation of fundamental database principles while striving for the coveted **ACID** properties:

*   **Atomicity:**  All operations within a transaction succeed or fail as a single unit.
*   **Consistency:** Data remains valid and adheres to defined rules after each transaction.
*   **Isolation:** Concurrent transactions operate as if they were executed sequentially, preventing interference.
*   **Durability:** Committed data survives even system failures, thanks to our Write-Ahead Log (WAL).

## Core Features & Design Philosophy

This project serves as a practical exploration of modern database fundamentals, including:

*   **Write-Ahead Logging (WAL):**  Every data modification (`PUT` or `DELETE`) is first recorded in a persistent log file (`.log`). This ensures data durability and enables recovery in case of crashes or power outages.
*   **In-Memory Hash Index:** A simple yet efficient hash index, powered by the classic DJB hash function, resides in memory to provide rapid key-based lookups.
*   **Basic Transaction Support:**  Transactions are managed through `BEGIN`, `COMMIT`, and `ROLLBACK` operations. The current implementation logs these operations, paving the way for more sophisticated rollback/recovery mechanisms.
*   **Disk-Based Data Storage:** Data records are appended to a `.data` file, providing a persistent record of all key-value pairs. This allows for data recovery and the ability to reload the database after a restart.
*   **Concurrency Control (Rudimentary):**  Currently, a global mutex protects the entire system to ensure data consistency in a multi-threaded environment. (See "Next Steps & Ideas" for more advanced concurrency models to explore.)

## Architecture at a Glance

*   **Index:** An in-memory hash table organized into buckets of singly linked lists. Each bucket stores key-value pairs. Hashing, using the DJB function, distributes keys across buckets.
*   **Log:** The `.log` file is the heart of our durability strategy.  All operations are appended to this file *before* they are applied to the data file. This ensures that we can recover from failures by replaying the log.
*   **Transactions:** `BEGIN`, `COMMIT`, and `ROLLBACK` define the boundaries of atomic transactions. Extend the current logging implementation to include real undo operations for enhanced data integrity.
*   **Persistence:** The `.data` file stores `Record` structs sequentially. In a production-ready system, you would likely enhance this with additional metadata (e.g., checksums, record lengths, version numbers) for robustness and efficient data management.

## Getting Started

1.  **Clone the Repository:**

    ```bash
    git clone [https://github.com/yourusername/AtomicWave.git](https://github.com/yourusername/AtomicWave.git)
    ```

2.  **Build the Project:**

    ```bash
    cd AtomicWave
    gcc -pthread -o kvstore main.c
    ```

3.  **Run the Example:**

    ```bash
    ./kvstore
    ```

    You'll see output demonstrating basic operations: `PUT`, `COMMIT`, `GET`, and a `DELETE` followed by a `ROLLBACK`, showcasing the basic transactional behavior.

## Next Steps & Ideas: Your Path to Database Mastery

This project is a starting point. We encourage you to extend it and explore more advanced database concepts:

*   **Enhanced Recovery:** Implement a robust recovery mechanism that reads the WAL on startup and intelligently reapplies or reverts incomplete transactions, ensuring data consistency after failures.
*   **Advanced Concurrency Control:**
    *   **Fine-Grained Locking:** Replace the global mutex with per-bucket or even per-record locks to increase concurrency.
    *   **Read-Write Locks:** Allow multiple readers to access data concurrently while ensuring exclusive access for writers.
    *   **Multi-Version Concurrency Control (MVCC):** Implement MVCC to enable non-blocking reads and greater parallelism for concurrent read and write operations.
*   **Index Upgrades:**
    *   **B+ Trees:**  Replace the hash index with a B+ tree for efficient range queries and ordered data retrieval.
    *   **LSM Trees (Log-Structured Merge Trees):** Explore LSM trees for write-optimized performance, especially beneficial in write-heavy workloads.
*   **Sharding and Replication:**
    *   **Sharding:**  Distribute the data across multiple nodes (horizontal scaling) to improve performance and handle larger datasets.
    *   **Replication:**  Create copies of the database on multiple nodes to enhance fault tolerance and read performance.
*   **Data Integrity Enhancements:** Add checksums, record length validation, and versioning to the `.data` file format for increased robustness and error detection.
*   **Query Language/API:** Implement a simple query language or a richer API to provide more flexible data access and manipulation.
*   **Performance Profiling and Optimization:** Use profiling tools to identify bottlenecks and optimize critical sections of the code for improved performance.

## Contribute!

We welcome contributions! If you're passionate about databases and want to dive into the inner workings of a key-value store, feel free to fork the repository, experiment with these ideas, and submit pull requests. Let's build something awesome together!
