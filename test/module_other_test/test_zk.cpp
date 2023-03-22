#include <string.h>
#include <errno.h>

#include <zookeeper.h>

#include "logger.h"

static zhandle_t *zh;

/** Watcher function -- empty for this example, not something you should
 * do in real code */
void watcher(zhandle_t *zzh, int type, int state, const char *path,
             void *watcherCtx) {

}

int main() {
    char buffer[512];
    char p[2048];
    char *cert=0;

    zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);

    zh = zookeeper_init("localhost:2181", watcher, 10000, 0, 0, 0);
    if (!zh) {
        return errno;
    }

    struct ACL CREATE_ONLY_ACL[] = {{ZOO_PERM_CREATE, ZOO_AUTH_IDS}};
    struct ACL_vector CREATE_ONLY = {1, CREATE_ONLY_ACL};
    int rc = zoo_create(zh,"/rpc/xyz","value", 5, &CREATE_ONLY, ZOO_EPHEMERAL,
                        buffer, sizeof(buffer)-1);

    if (rc) {
        MyRPC::Logger::info("Error {} for {}", rc, __LINE__);
    }

    /** this operation will fail with a ZNOAUTH error */
    int buflen= sizeof(buffer);
    struct Stat stat;
    rc = zoo_get(zh, "/rpc/xyz", 0, buffer, &buflen, &stat);
    if (rc) {
        MyRPC::Logger::info("Error {} for {}", rc, __LINE__);
    }

    zookeeper_close(zh);
    return 0;
}