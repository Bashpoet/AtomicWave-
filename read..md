This project is a lean, mean key-value store, written in C, that fuses an in-memory hash index with a disk-based datastore. We strive for the ACID Holy Grail—(A)tomic commits, (C)onsistent updates, (I)solation from concurrency mishaps, and (D)urability via a write-ahead log (WAL).

Think of it as a proving ground for modern database fundamentals, sporting:

Transaction Logging: Every PUT or DELETE is appended to a WAL. Commit or Rollback? We’ve got you covered.
Hash-Based Index: A straightforward in-memory hash that directs you to the right place nearly every time.
Naive Concurrency: A global mutex encloses the entire system. (If you’re the adventurous type, dig into finer-grained locks, or open the concurrency floodgates via read-write locks. We won’t judge.)
Data File Storage: Each record is appended to a .data file for easy retrieval, leaving a clear trail of your kv expansions and shenanigans.
Architecture at a Glance
Index: Buckets of singly linked lists store your key-value pairs in memory, anchored by a simple (but classic) DJB hash function.
Log: All operations are written out to the .log file before touching the data store. That’s the WAL principle, ensuring data resilience even when the gremlins cause power failures.
Transactions: Atomic blocks revolve around BEGIN, COMMIT, and ROLLBACK. This sample code only logs them, but you can extend it to do real-time undone operations if your data demands unbreakable consistency.
Persistence: The .data file is structured by appending Record structs. You’d typically want extra metadata (checksums, lengths, version IDs) in a serious system.
Feel free to expand or refine any portion—from forging B+ trees in place of the simplistic hash to layering advanced concurrency or offering fancy user-friendly analytics. This is your sandbox to experiment with the building blocks of data integrity, concurrency, and storage design.

Getting Started
Clone the repository:
bash
Copy code
git clone https://github.com/yourusername/YourDBName.git
Build it:
bash
Copy code
cd YourDBName
gcc -pthread -o kvstore main.c
Run the example:
bash
Copy code
./kvstore
You’ll see output indicating the PUT, the COMMIT, a few GETs, then a sneaky DELETE that gets rolled back, proving our micro-transactional muscle.

Next Steps & Ideas
Enhanced Recovery: On startup, read the WAL in detail. Reapply or revert operations that were left in limbo.
Multithreaded Concurrency: Replace the single lock with a per-bucket spinlock or a read-write lock. If you’re confident, explore multi-version concurrency control to handle simultaneous queries and writes.
Index Upgrades: Switch from a hash to a B+ tree or an LSM tree for faster lookups and simpler merges.
Sharding & Replication: Scale out horizontally or replicate your store to multiple nodes for improved fault tolerance.
Mix, match, and marvel in your own database engineering feats!

