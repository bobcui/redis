/*******************************************************************/
/* ziplist
/*******************************************************************/
// http://blog.jobbole.com/85185/
/* The ziplist is a specially encoded dually linked list that is designed
 * to be very memory efficient. It stores both strings and integer values,
 * where integers are encoded as actual integers instead of a series of
 * characters. It allows push and pop operations on either side of the list
 * in O(1) time. However, because every operation requires a reallocation of
 * the memory used by the ziplist, the actual complexity is related to the
 * amount of memory used by the ziplist.
 *
 * ----------------------------------------------------------------------------
 *
 * ZIPLIST OVERALL LAYOUT:
 * The general layout of the ziplist is as follows:
 * <zlbytes><zltail><zllen><entry><entry><zlend>
 *
 * <zlbytes> is an unsigned integer to hold the number of bytes that the
 * ziplist occupies. This value needs to be stored to be able to resize the
 * entire structure without the need to traverse it first.
 *
 * <zltail> is the offset to the last entry in the list. This allows a pop
 * operation on the far side of the list without the need for full traversal.
 *
 * <zllen> is the number of entries.When this value is larger than 2**16-2,
 * we need to traverse the entire list to know how many items it holds.
 *
 * <zlend> is a single byte special value, equal to 255, which indicates the
 * end of the list.
 *
 * ZIPLIST ENTRIES:
 * Every entry in the ziplist is prefixed by a header that contains two pieces
 * of information. First, the length of the previous entry is stored to be
 * able to traverse the list from back to front. Second, the encoding with an
 * optional string length of the entry itself is stored. 
 */
typedef struct zlentry {
    unsigned int prevrawlensize, prevrawlen;
    unsigned int lensize, len;
    unsigned int headersize;
    unsigned char encoding;
    unsigned char *p;
} zlentry;


/*******************************************************************/
/* intset
/*******************************************************************/
ordered integer vector



/*******************************************************************/
/* O(?) comparison
/*******************************************************************/
        set/add    get     getall    exist    del

ziplist    N        N        N         N       N
dict       1        1        N         1       1 
intset        1        1        N         1       1

list       N        N        N         N       N


/*******************************************************************/
/* O(?) comparison
/*******************************************************************/
t_hash      ziplist / dict
t_zset      dict / intset
t_list      
t_zset

/*******************************************************************/
/* HSet Command
/*******************************************************************/
// t_hash.c
void hsetCommand(client *c) {
    robj *o;

    // create if not exist, default is ziplist
    if ((o = hashTypeLookupWriteOrCreate(c,c->argv[1])) == NULL) return;

    // if keylen or valuelen > hash_max_ziplist_value, then change to hashtable 
    // Notice: OBJ_ENCODING_HT never change back to OBJ_ENCODING_ZIPLIST
    hashTypeTryConversion(o,c->argv,2,3);
    hashTypeTryObjectEncoding(o,&c->argv[2], &c->argv[3]);

    update = hashTypeSet(o,c->argv[2],c->argv[3]);
        // if (o->encoding == OBJ_ENCODING_ZIPLIST) {
        //      ziplistDelete()
        //      ziplistInsert()
        //      
        //      if (hashTypeLength(o) > server.hash_max_ziplist_entries)
        //          hashTypeConvert(o, OBJ_ENCODING_HT);
        // }
        // else if (o->encoding == OBJ_ENCODING_HT) {
        //      dictReplace()
        // }

    // add to reply buffer 
    addReply(c, update ? shared.czero : shared.cone);

    signalModifiedKey(c->db,c->argv[1]);
    notifyKeyspaceEvent(NOTIFY_HASH,"hset",c->argv[1],c->db->id);
    server.dirty++;
}

/*******************************************************************/
/* HGet Command
/*******************************************************************/
// t_hash.c
void hgetCommand(client *c) {
    robj *o;

    // will invoke expireIfNeeded() 
    if ((o = lookupKeyReadOrReply(c,c->argv[1],shared.nullbulk)) == NULL ||
        checkType(c,o,OBJ_HASH)) return;

    addHashFieldToReply(c, o, c->argv[2]);
}
