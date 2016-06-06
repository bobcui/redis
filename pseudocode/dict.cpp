/*******************************************************************/
/* base structs
/*******************************************************************/
typedef struct dictEntry {
    void *key;
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    struct dictEntry *next;
} dictEntry;

typedef struct dictht {
    dictEntry **table;
    unsigned long size;
    unsigned long sizemask;
    unsigned long used;
} dictht;

typedef struct dict {
    dictType *type;
    void *privdata;
    dictht ht[2];
    long rehashidx; /* rehashing not in progress if rehashidx == -1 */
    int iterators; /* number of iterators currently running */
} dict;


/*******************************************************************/
/* Hash operations
/*******************************************************************/
// dictAdd() {
//     dictAddRaw()
// }
dictEntry *dictAddRaw(dict *d, void *key)
{
    // ...

    /* Get the index of the new element, or -1 if
     * the element already exists. */
    if ((index = _dictKeyIndex(d, key)) == -1)
        return NULL;

    /* Allocate the memory and store the new entry.
     * Insert the element in top, with the assumption that in a database
     * system it is more likely that recently added entries are accessed
     * more frequently. */
    ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];

    // insert entry
    entry = zmalloc(sizeof(*entry));
    entry->next = ht->table[index];
    ht->table[index] = entry;
    ht->used++;

    /* Set the hash entry fields. */
    dictSetKey(d, entry, key);
    return entry;
}


/*******************************************************************/
/* Hash桶增大
/*******************************************************************/
// dictAddRaw() {
//     _dictKeyIndex()
//         _dictKeyIndex()
//             _dictExpandIfNeeded()
// }
static int _dictExpandIfNeeded(dict *d)
{
    // ...

    /* If we reached the 1:1 ratio, and we are allowed to resize the hash
     * table (global setting) or we should avoid it but the ratio between
     * elements/buckets is over the "safe" threshold, we resize doubling
     * the number of buckets. */
    // dict_can_resize = 1
    // dict_force_resize_ratio = 5
    if (d->ht[0].used >= d->ht[0].size &&
        (dict_can_resize ||
         d->ht[0].used/d->ht[0].size > dict_force_resize_ratio))
    {
        return dictExpand(d, d->ht[0].used*2);  // hast桶扩大到 2^n >= used*2
    }
    return DICT_OK;
}

// 初始化 reshashing table
int dictExpand(dict *d, unsigned long size)
{
    dictht n; /* the new hash table */
    unsigned long realsize = _dictNextPower(size);

    // ...

    /* Allocate the new hash table and initialize all pointers to NULL */
    n.size = realsize;
    n.sizemask = realsize-1;
    n.table = zcalloc(realsize*sizeof(dictEntry*));
    n.used = 0;

    // ...

    /* Prepare a second hash table for incremental rehashing */
    d->ht[1] = n;
    d->rehashidx = 0;
    return DICT_OK;
}

/*******************************************************************/
/* Hash桶缩小
/*******************************************************************/
// aeProcessEvents() {
//     serverCron()  // invoked every 1000/server.hz milliseconds
//         databasesCron()
// }
void databasesCron() {
    // ...

    // 判断 db.dict和db.expires 是否可以压缩, 一次最多尝试16个db
    // resize if size > 4 && used/size < 10%, dictExpand(d, used)
    for (j = 0; j < dbs_per_call; j++) {
        tryResizeHashTables(resize_db % server.dbnum);
        resize_db++;
    }

    // ...
}

/*******************************************************************/
/* Rehash
/*******************************************************************/
// aeProcessEvents() {
//     serverCron()  // invoked every 1000/server.hz milliseconds
//         databasesCron()
// }
void databasesCron() {
    // ...

    if (server.activerehashing) {   // 当 config.activerehashing = yes 时进入
        for (j = 0; j < dbs_per_call; j++) {  // 一次最多尝试16个db
            int work_done = incrementallyRehash(rehash_db % server.dbnum);
            rehash_db++;
            if (work_done) {    // 最多只对一个db的dict或expires进行rehash
                /* If the function did some work, stop here, we'll do
                 * more at the next cron loop. */
                break;
            }
        }
    }    

    // ...
}

int incrementallyRehash(int dbid) {
    // ...
    // 理论上每次操作控制在1ms，但是时间无法精确控制
    dictRehashMilliseconds(server.db[dbid].dict,1);

    // if dict do not need to rehash, try expires
    dictRehashMilliseconds(server.db[dbid].expires,1);
    // ...
}

int dictRehashMilliseconds(dict *d, int ms) {
    // dictRehash 移动100个有效hash桶，或者检查了100*10个空桶，或者迁移完全部hash桶后返回
    // ret = 0 全部迁完
    while(dictRehash(d,100)) {
        rehashes += 100;
        // 检查耗时是否超过1ms, call gettimeofday()
        if (timeInMilliseconds()-start > ms) break;
    }
}

// called by add/find/replace/delete
static void _dictRehashStep(dict *d) {
    if (d->iterators == 0) dictRehash(d,1);
}