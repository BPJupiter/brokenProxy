#include "Europa/europa.h"
#include "datastore.h"

#ifdef _WIN32
#define DB_DIRECTORY "db\\measurementinfo.vdb"
#else
#define DB_DIRECTORY "db/measurementinfo.vdb"
#endif

void *gDatastore_lock = NULL; /* unused */
EDBHandle *gDatastore = NULL;

void datastore_init()
{
    char pwd[256];
    gDatastore_lock = europa_mutex_create();
    gDatastore = europa_database_open(DB_DIRECTORY);
    europa_get_pwd(pwd, sizeof pwd);
    printf("PWD: %s\n", pwd);
}

void datastore_destroy()
{
    if (NULL != gDatastore)
    {
        europa_database_close(&gDatastore);
        gDatastore = NULL;
    }
    if (NULL != gDatastore_lock)
    {
        europa_mutex_destroy(gDatastore_lock);
        gDatastore_lock = NULL;
    }
    printf("Destroyed datastore\n");
}
