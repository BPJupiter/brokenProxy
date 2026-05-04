#include "Clay/clay.h"
#include "Europa/europa.h"

#include "vedis/vedis.h"

EDBHandle *europa_database_open(const char *filename)
{
	static boolean initialised = FALSE;
	static vedis *database = NULL;
	if (!initialised)
	{
		int rv;
		const char *err_buf;
		int err_len;
		rv = vedis_open(&database, filename);
		if (rv == VEDIS_OK)
		{
			initialised = TRUE;
			printf("Created DB at %s\n", filename);
			printf("Is threadsafe: %d\n", vedis_lib_is_threadsafe());
			vedis_lib_config(VEDIS_LIB_CONFIG_THREAD_LEVEL_MULTI);
			return database;
		}

		initialised = FALSE;
		vedis_config(database, VEDIS_CONFIG_ERR_LOG, &err_buf, &err_len);
		if (err_len > 0)
			printf("%s\n", err_buf);
	}
	return database;
}

void europa_database_close(EDBHandle **database)
{
	int rv;
	const char *err_buf;
	int err_len;
	if (NULL == *database)
		return;

	rv = vedis_close(*database);
	if (rv == VEDIS_OK)
	{
		*database = NULL;
		return;
	}

	vedis_config(*database, VEDIS_CONFIG_ERR_LOG, &err_buf, &err_len);
	if (err_len > 0)
		printf("%s\n", err_buf);
}

boolean europa_database_store(EDBHandle *database, const void *key, int key_length, const void *data, uint64 data_length)
{
	int rv;
	const char *err_buf;
	int err_len;
	rv = vedis_kv_store(database, key, key_length, data, data_length);
	if (rv == VEDIS_OK)
		return TRUE;

	printf("Storage failed! Vedis returned code: %d\n", rv);

	vedis_config(database, VEDIS_CONFIG_ERR_LOG, &err_buf, &err_len);
	if (err_len > 0)
		printf("%s\n", err_buf);
	return FALSE;
}

boolean europa_database_fetch(EDBHandle *database, const void *key, int key_length, void *buffer, uint64 *buffer_size)
{
	int rv;
	const char *err_buf;
	int err_len;
	rv = vedis_kv_fetch(database, key, key_length, buffer, (vedis_int64 *)buffer_size);
	if (rv == VEDIS_OK)
		return TRUE;

	switch (rv)
	{
		case VEDIS_NOTFOUND:
			printf("Inserting record for key: %s\n", (char *)key);
		break;
		default:
			printf("Fetch failed! Vedis returned code: %d\n", rv);
		break;
	}

	vedis_config(database, VEDIS_CONFIG_ERR_LOG, &err_buf, &err_len);
	if (err_len > 0)
		printf("%s\n", err_buf);
	return FALSE;
}

boolean europa_database_delete(EDBHandle *database, const void *key, int key_length)
{
	int rv;
	const char *err_buf;
	int err_len;
	rv = vedis_kv_delete(database, key, key_length);
	if (rv == VEDIS_OK)
		return TRUE;

	vedis_config(database, VEDIS_CONFIG_ERR_LOG, &err_buf, &err_len);
	if (err_len > 0)
		printf("%s\n", err_buf);
	return FALSE;
}

