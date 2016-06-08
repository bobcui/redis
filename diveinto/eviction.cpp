/*******************************************************************/
/* Set expire
/*******************************************************************/
// set command with expire
// void setCommand(client *c) {
//     setGenericCommand()
// }
void setGenericCommand(client *c, int flags, robj *key, ...) {
    // ...

    // call expireIfNeeded
    // dictReplace(db->dict, key, val);
    setKey(c->db,key,val);
    server.dirty++;

    // dictReplaceRaw(db->expires, key);
    if (expire) setExpire(c->db,key,mstime()+milliseconds);

    // notify events
    notifyKeyspaceEvent(NOTIFY_STRING,"set",key,c->db->id);
    if (expire) notifyKeyspaceEvent(NOTIFY_GENERIC,
        "expire",key,c->db->id);
    
    // add to reply buffer
    addReply(c, ok_reply ? ok_reply : shared.ok);
}

// expire commands
// void expireCommand(client *c) {
//     expireGenericCommand()
// }
void expireGenericCommand(client *c, long long basetime, int unit) {
    // ...
    setExpire(c->db,key,when);
    // ...
}

// 底层实现 插入key到expires hashtable
void setExpire(redisDb *db, robj *key, long long when) {
    // ...
    de = dictReplaceRaw(db->expires, key);
    dictSetSignedIntegerVal(de,when);
}

/*******************************************************************/
/* check expiration when access
/*******************************************************************/
// invoked by setkey and getkey
// return 1 if expired
int expireIfNeeded(redisDb *db, robj *key) {
    mstime_t when = getExpire(db,key);
    mstime_t now;

    if (when < 0) return 0; /* No expire for this key */

    /* Don't expire anything while loading. It will be done later. */
    if (server.loading) return 0;

    /* If we are in the context of a Lua script, we claim that time is
     * blocked to when the Lua script started. This way a key can expire
     * only the first time it is accessed and not in the middle of the
     * script execution, making propagation to slaves / AOF consistent.
     * See issue #1525 on Github for more information. */
    now = server.lua_caller ? server.lua_time_start : mstime();

    /* If we are running in the context of a slave, return ASAP:
     * the slave key expiration is controlled by the master that will
     * send us synthesized DEL operations for expired keys.
     *
     * Still we try to return the right information to the caller,
     * that is, 0 if we think the key should be still valid, 1 if
     * we think the key is expired at this time. */
    if (server.masterhost != NULL) return now > when;

    /* Return when this key has not expired */
    if (now <= when) return 0;

    /* Delete the key */
    server.stat_expiredkeys++;
    propagateExpire(db,key);
    notifyKeyspaceEvent(NOTIFY_EXPIRED,
        "expired",key,db->id);
    return dbDelete(db,key);
}

/*******************************************************************/
/* active expire in event loop
/*******************************************************************/
// void beforesleep() {    // invoked every event loop
//     activeExpireCycle(ACTIVE_EXPIRE_CYCLE_FAST)
// }
// void aeProcessEvents() {
//     serverCron()    // invoked every 1000/server.hz milliseconds
//         databasesCron()
//             activeExpireCycle(ACTIVE_EXPIRE_CYCLE_SLOW)
// }
void activeExpireCycle(int type) {
    if (FAST) {
        timelimit = 1ms
    }
    else {  // SLOW
        timelimit = 25% hz time
    }

    while (time < timelimit) {
        de = dictGetRandomKey(db->expires))
        tryExpire(de)
        consume time
    }
}

/*******************************************************************/
/* eviction
/*******************************************************************/
// aeProcessEvents() {
//     readQueryFromClient()
//         processInputBuffer()
//             processCommand()
// }
int processCommand(client *c) {
    // ...
    if (server.maxmemory) {
        int retval = freeMemoryIfNeeded();
        // ...
    }
    // ...
}

int freeMemoryIfNeeded(void) {
    mem_used = zmalloc_used_memory();
    mem_tofree = mem_used - server.maxmemory;
    mem_freed = 0;
    while (mem_freed < mem_tofree) {
        for (j = 0; j < server.dbnum; j++) {

            if (allkeys-random) {
                key = randmonKeyFromDict();
            }

            if (volatile-random) {
                key = randmonKeyFromExpires();
            }

            if (allkeys-lru) {
                samples = randomKeysFromDict(maxmemory_samples)
                key = selectIdlest(samples)
            }

            if (volatile-lru) {
                samples = randomKeysFromExpires(maxmemory_samples)
                key = selectIdlest(samples)
            }

            if (volatile-ttl) {
                samples = randomKeysFromExpires(maxmemory_samples)
                key = selectMinTimestamps(samples)
            }

            /* Finally remove the selected key. */
            if (key) {
                dbDelete(db,key);
            }
        }
    }
}
