/*******************************************************************/
/* base structs
/*******************************************************************/
// server.h
struct redisServer {
    // ...
    redisDb *db;                /* server.db = zmalloc(sizeof(redisDb)*server.dbnum); */
    // ...
};

// server.h
typedef struct redisDb {
    dict *dict;                 /* The keyspace for this DB */
    dict *expires;              /* Timeout of keys with a timeout set */
    dict *blocking_keys;        /* Keys with clients waiting for data (BLPOP) */
    dict *ready_keys;           /* Blocked keys that received a PUSH */
    dict *watched_keys;         /* WATCHED keys for MULTI/EXEC CAS */
    struct evictionPoolEntry *eviction_pool;    /* Eviction pool of keys */
    int id;                     /* Database ID */
    long long avg_ttl;          /* Average TTL, just for stats */
} redisDb;

/*******************************************************************/
/* base server
/*******************************************************************/
int main(int argc, char **argv) {
    // ...

    initServer();
        // Create the Redis databases, and initialize other internal state. 
        // for (j = 0; j < server.dbnum; j++) {
        //     server.db[j].dict = dictCreate(&dbDictType,NULL);
        //     server.db[j].expires = dictCreate(&keyptrDictType,NULL);
        //     server.db[j].blocking_keys = dictCreate(&keylistDictType,NULL);
        //     server.db[j].ready_keys = dictCreate(&setDictType,NULL);
        //     server.db[j].watched_keys = dictCreate(&keylistDictType,NULL);
        //     server.db[j].eviction_pool = evictionPoolAlloc();
        //     server.db[j].id = j;
        //     server.db[j].avg_ttl = 0;
        // } 

    // ...

    aeSetBeforeSleepProc(server.el,beforeSleep);
    aeMain(server.el);
    aeDeleteEventLoop(server.el);
    return 0;
}

// Reactor模型 linux下使用epoll
// server.c
void aeMain(aeEventLoop *eventLoop) {
    eventLoop->stop = 0;
    while (!eventLoop->stop) {
        if (eventLoop->beforesleep != NULL)
            eventLoop->beforesleep(eventLoop);

        aeProcessEvents(eventLoop, AE_ALL_EVENTS);
            // aeApiPoll()  handle IO events
            // processTimeEvents()
                // serverCron()  invoked every 1000/hz ms
    }
}


/*******************************************************************/
/* deep in server
/*******************************************************************/
// server.c
void beforeSleep(struct aeEventLoop *eventLoop) {
    // ...
    /* Run a fast expire cycle (the called function will return
     * ASAP if a fast cycle is not needed). */
    activeExpireCycle(ACTIVE_EXPIRE_CYCLE_FAST);
    // ...
    /* Write the AOF buffer on disk */
    flushAppendOnlyFile(0);
    // ...
}

// aof.c
void flushAppendOnlyFile(int force) {
    // if appendonly == yes

    // ...
    // bioPendingJobsOfType() will call pthread_mutex_lock()
    if (server.aof_fsync == AOF_FSYNC_EVERYSEC)
        sync_in_progress = bioPendingJobsOfType(BIO_AOF_FSYNC) != 0;
    // ...

    /* We want to perform a single write. This should be guaranteed atomic
     * at least if the filesystem we are writing is a real physical one.
     * While this will save us against the server being killed I don't think
     * there is much to do about the whole server stopping for power problems
     * or alike */

    // write blocking or nonblocking?
    // from Advanced Programming in the UNIX Environment 2nd Ed:
    // "We also said that system calls related to disk I/O are not considered slow, 
    // even though the read or write of a disk file can block the caller temporarily."
    // see more:
    //   http://stackoverflow.com/questions/4434223/non-blocking-write-to-file-in-c-c
    //   http://blog.empathybox.com/post/35088300798/why-does-fwrite-sometimes-block

    // But the background io thread maybe call fsync() at the same time
    // man fsync:
    //   This normally results in all in-core modified copies of buffers 
    //   for the associated file to be written to a disk.
    nwritten = write(server.aof_fd,server.aof_buf,sdslen(server.aof_buf));
    // ...

    /* Don't fsync if no-appendfsync-on-rewrite is set to yes and there are
     * children doing I/O in the background. */
    if (server.aof_no_fsync_on_rewrite &&
        (server.aof_child_pid != -1 || server.rdb_child_pid != -1))
            return;

    /* Perform the fsync if needed. */
    if (server.aof_fsync == AOF_FSYNC_ALWAYS) {
        // ...
        /* aof_fsync is defined as fdatasync() for Linux in order to avoid
         * flushing metadata. */
        aof_fsync(server.aof_fd); /* Let's try to get this data on the disk */
        // ...
    } else if ((server.aof_fsync == AOF_FSYNC_EVERYSEC &&
        // ...
        // aof_background_fsync will put fd to the bg thread queue, will call pthread_mutex_lock()
        if (!sync_in_progress) aof_background_fsync(server.aof_fd);
        // ...
    }
}


/* This is our timer interrupt, called server.hz times per second.
 * Here is where we do a number of things that need to be done asynchronously.
 * For instance:
 *
 * - Active expired keys collection (it is also performed in a lazy way on
 *   lookup).
 * - Software watchdog.
 * - Update some statistic.
 * - Incremental rehashing of the DBs hash tables.
 * - Triggering BGSAVE / AOF rewrite, and handling of terminated children.
 * - Clients timeout of different kinds.
 * - Replication reconnection.
 * - Many more...
 *
 * Everything directly called here will be called server.hz times per second,
 * so in order to throttle execution of things we want to do less frequently
 * a macro is used: run_with_period(milliseconds) { .... }
 */
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    // ...

    /* We need to do a few operations on clients asynchronously. */
    // close timeout client
    // resize client query buffer 
    clientsCron();

    /* Handle background operations on Redis databases. */
    databasesCron();

    /* Start a scheduled AOF rewrite if this was requested by the user while
     * a BGSAVE was in progress. */
    if (server.rdb_child_pid == -1 && server.aof_child_pid == -1 &&
        server.aof_rewrite_scheduled)
    {
        rewriteAppendOnlyFileBackground();
    }

}

/*
Memory of fork():
    The Linux kernel does implement Copy-on-Write when fork() is called. 
    When the syscall is executed, the pages that the parent and child share are marked read-only.
    If a write is performed on the read-only page, it is then copied, 
    as the memory is no longer identical between the two processes. 
    Therefore, if only read-operations are being performed, the pages will not be copied at all.
 */

