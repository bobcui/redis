
void beforeSleep(struct aeEventLoop *eventLoop) {
    // ...
    // aof增量buffer写入到磁盘
    flushAppendOnlyFile(0);
    // ...
}

// void aeProcessEvents() {
//     serverCron()    // invoked every 1000/server.hz milliseconds
// }
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {

    if ( 当前没有进行aof备份 && 没有进行rdb备份 ） {

        if ( 收到bgrewriteaof命令 || 触发了自动备份 ) {
            // 全量备份
            rewriteAppendOnlyFileBackground()
        }

        // 自动备份见配置
        // auto-aof-rewrite-percentage 100
        // auto-aof-rewrite-min-size 64mb
    }
 
    // 遇到慢fsync慢的情况，每个hz检查是否能写入，最多延迟2s
    if (server.aof_flush_postponed_start) flushAppendOnlyFile(0);

    // 出错后每一秒进行重试
    run_with_period(1000) {
        if (server.aof_last_write_status == C_ERR)
            flushAppendOnlyFile(0);
    }
}


// aof.c
void flushAppendOnlyFile(int force) {
    // if appendonly == no  return

    if (server.aof_fsync == AOF_FSYNC_EVERYSEC) {
        sync_in_progress = bioPendingJobsOfType(BIO_AOF_FSYNC) != 0;
        if ( sync_in_progress && sync时间不超过2s ) {     // bio thread calling fsync()
            return
        }
    }

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

    /* Perform the fsync if needed. */
    if (server.aof_fsync == AOF_FSYNC_ALWAYS) {
        // ...
        /* aof_fsync is defined as fdatasync() for Linux in order to avoid
         * flushing metadata. */
        aof_fsync(server.aof_fd); /* Let's try to get this data on the disk */
        // ...
    } else if (server.aof_fsync == AOF_FSYNC_EVERYSEC && server.unixtime > server.aof_last_fsync)
        // ...
        // aof_background_fsync will put fd to the bg thread queue, will call pthread_mutex_lock()
        if (!sync_in_progress) aof_background_fsync(server.aof_fd);
        // ...
    }
}

