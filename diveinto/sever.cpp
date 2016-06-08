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
        
        // register listen fd read event
        // for (j = 0; j < server.ipfd_count; j++) {
        //     if (aeCreateFileEvent(server.el, server.ipfd[j], AE_READABLE,
        //         acceptTcpHandler,NULL) == AE_ERR)
        //         {
        //             serverPanic(
        //                 "Unrecoverable error creating server.ipfd file event.");
        //         }
        // }

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


/* Process every pending time event, then every pending file event
 * (that may be registered by time event callbacks just processed).
 * Without special flags the function sleeps until some file event
 * fires, or when the next time event occurs (if any).
 *
 * If flags is 0, the function does nothing and returns.
 * if flags has AE_ALL_EVENTS set, all the kind of events are processed.
 * if flags has AE_FILE_EVENTS set, file events are processed.
 * if flags has AE_TIME_EVENTS set, time events are processed.
 * if flags has AE_DONT_WAIT set the function returns ASAP until all
 * the events that's possible to process without to wait are processed.
 *
 * The function returns the number of events processed. */
int aeProcessEvents(aeEventLoop *eventLoop, int flags)
{
    numevents = aeApiPoll(eventLoop, tvp);  // block here
    for (j = 0; j < numevents; j++) {

        if (j is read event) {
            rfileProc(j) 
        }
        if (j is write event) {
            wfileProc(j)
        }
    }

    timeProc()
}

