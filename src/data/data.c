#include "Europa/europa.h"
#include "data/datastore.h"

#ifdef _WIN32
#define DB_DIRECTORY "..\\..\\..\\db\\measurementinfo.vdb"
#else
#endif

void *gDatastore_lock; /* unused */
EDBHandle *gDatastore;

void datastore_init()
{
	gDatastore_lock = europa_mutex_create();
	gDatastore = europa_database_open(DB_DIRECTORY);
	char pwd[256];
	europa_pwd(pwd, sizeof pwd);
	printf("PWD: %s\n", pwd);
}

void datastore_destroy()
{
	europa_mutex_destroy(gDatastore_lock);
	europa_database_close(gDatastore);

	gDatastore_lock = NULL;
	gDatastore = NULL;
}
