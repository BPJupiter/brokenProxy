#include "Europa/europa.h"
#include "data/datastore.h"

#ifdef _WIN32
#define DB_DIRECTORY "..\\..\\..\\db\\measurementinfo.vdb"
#else
#define DB_DIRECTORY "db/measurementinfo.vdb"
#endif

void *gDatastore_lock; /* unused */
EDBHandle *gDatastore;

void datastore_init()
{
	char pwd[256];
	gDatastore_lock = europa_mutex_create();
	gDatastore = europa_database_open(DB_DIRECTORY);
	europa_pwd(pwd, sizeof pwd);
	printf("PWD: %s\n", pwd);
}

void datastore_destroy()
{
	europa_database_close(&gDatastore);
	europa_mutex_destroy(gDatastore_lock);

	gDatastore_lock = NULL;
	gDatastore = NULL;
}
