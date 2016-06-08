
// called when start a new replication
int rdbSaveBackground(char *filename) {
    /*
    Memory of fork():
        The Linux kernel does implement Copy-on-Write when fork() is called. 
        When the syscall is executed, the pages that the parent and child share are marked read-only.
        If a write is performed on the read-only page, it is then copied, 
        as the memory is no longer identical between the two processes. 
        Therefore, if only read-operations are being performed, the pages will not be copied at all.
     */
    fork()

    if ( child ) {
        rdbSave(filename)

        // cat /proc/self/smaps | grep Private_Dirty
        size_t private_dirty = zmalloc_get_private_dirty();

        if (private_dirty) {
            serverLog(LL_NOTICE,
                "RDB: %zu MB of memory used by copy-on-write",
                private_dirty/(1024*1024));
        }
    }
    else {
        stat_fork_time
    }
}

// void aeProcessEvents() {
//     serverCron()    // invoked every 1000/server.hz milliseconds
//         backgroundSaveDoneHandlerDisk()
//             updateSlavesWaitingBgsave()
// }
void updateSlavesWaitingBgsave(int bgsaveerr, int type) {
    for every replica {

        // open is a blocking io api
        replica->repldbfd = open(server.rdb_filename,O_RDONLY)
        aeCreateFileEvent(server.el, slave->fd, AE_WRITABLE, sendBulkToSlave, slave)
    }
}

// void aeProcessEvents() {
//     sendBulkToSlave()
// }
void sendBulkToSlave(aeEventLoop *el, int fd, void *privdata, int mask) {
    /* Before sending the RDB file, we send the preamble as configured by the
     * replication process. Currently the preamble is just the bulk count of
     * the file in the form "$<length>\r\n". */
    write(fd,slave->replpreamble,sdslen(slave->replpreamble))

    // read file PROTO_IOBUF_LEN = 16k
    buflen = read(slave->repldbfd,buf,PROTO_IOBUF_LEN);

    // send data to replica
    write(fd,buf,buflen);

    // have sent all data
    if (slave->repldboff == slave->repldbsize) {
        close(slave->repldbfd);
        putSlaveOnline(slave);
    }
}

