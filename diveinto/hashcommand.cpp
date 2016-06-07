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