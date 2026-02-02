#include "Clay/clay.h"
#include "Europa/europa.h"
#include "shared_context/shared_context.h"

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
			return database;
		}

		initialised = FALSE;
		vedis_config(database, VEDIS_CONFIG_ERR_LOG, &err_buf, &err_len);
		if (err_len > 0)
			printf(err_buf);
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

	vedis_config(database, VEDIS_CONFIG_ERR_LOG, &err_buf, &err_len);
	if (err_len > 0)
		printf(err_buf);
}

boolean europa_database_store(EDBHandle *database, const void *key, int key_length, const void *data, uint64 data_length)
{
	int rv;
	const char *err_buf;
	int err_len;
	rv = vedis_kv_store(database, key, key_length, data, data_length);
	if (rv == VEDIS_OK)
		return TRUE;

	vedis_config(database, VEDIS_CONFIG_ERR_LOG, &err_buf, &err_len);
	if (err_len > 0)
		printf(err_buf);
	return FALSE;
}

boolean europa_database_fetch(EDBHandle *database, const void *key, int key_length, void *buffer, uint64 *buffer_size)
{
	int rv;
	const char *err_buf;
	int err_len;
	rv = vedis_kv_fetch(database, key, key_length, buffer, buffer_size);
	if (rv == VEDIS_OK)
		return TRUE;

	vedis_config(database, VEDIS_CONFIG_ERR_LOG, &err_buf, &err_len);
	if (err_len > 0)
		printf(err_buf);
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
		printf(err_buf);
	return FALSE;
}

