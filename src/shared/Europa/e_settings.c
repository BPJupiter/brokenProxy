#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "Clay/clay.h"

typedef enum {
    EUROPA_ST_UNDEFINED,
    EUROPA_ST_BOOLEAN,
    EUROPA_ST_INTEGER,
    EUROPA_ST_DOUBLE,
    EUROPA_ST_TEXT,
    EUROPA_ST_COUNT
} EuropaSettingType;

#define SETTINGS_NAME_LENGTH 64

typedef struct {
    char name[SETTINGS_NAME_LENGTH];
    EuropaSettingType type;
    const char *comment;
    union {
        boolean toggle;
        char *text;
        double real;
        int integer;
    } data;
} EuropaSetting;

struct {
    EuropaSetting *array;
    uint count;
    uint version;
} EuropaGlobalSettings;

static void init_setting(void)
{
    static boolean init = FALSE;
    if (init)
        return;
    init = TRUE;

    EuropaGlobalSettings.array = malloc((sizeof *EuropaGlobalSettings.array) * 128);
    EuropaGlobalSettings.count = 0;
    EuropaGlobalSettings.version = 1;
}

static void clear_setting(void)
{
    free(EuropaGlobalSettings.array);
    EuropaGlobalSettings.array = malloc((sizeof *EuropaGlobalSettings.array) * 128);
    EuropaGlobalSettings.count = 0;
    EuropaGlobalSettings.version = 1;
}

static EuropaSetting *europa_add_setting(const char *name, const char *comment)
{
    EuropaSetting *s = NULL;
    uint i, j;
    init_setting();
    for (i = 0; i < EuropaGlobalSettings.count; i++) {
        for (j = 0; EuropaGlobalSettings.array[i].name[j] == name[j] && name[j] != 0; j++);
        if (EuropaGlobalSettings.array[i].name[j] == name[j]) {
            s = &EuropaGlobalSettings.array[i];
            if (s->type == EUROPA_ST_UNDEFINED || s->type == EUROPA_ST_TEXT)
                free(s->data.text);
            break;
        }
    }
    if (s == NULL) {
        if (EuropaGlobalSettings.count % 64 == 0)
            EuropaGlobalSettings.array = realloc(EuropaGlobalSettings.array, (sizeof *EuropaGlobalSettings.array) * (EuropaGlobalSettings.count + 64));
        s = &EuropaGlobalSettings.array[EuropaGlobalSettings.count++];
        for (i = 0; i < SETTINGS_NAME_LENGTH - 1 && name[i] != 0; i++)
            s->name[i] = name[i];
        s->name[i] = 0;
    }
    s->comment = comment;
    s->type = EUROPA_ST_UNDEFINED;
    s->data.integer = -1;
    return s;
}

static EuropaSetting *europa_setting_get(const char *name)
{
    uint i;
    init_setting();
    for (i = 0; i < EuropaGlobalSettings.count; i++)
        if (strcmp(EuropaGlobalSettings.array[i].name, name) == 0)
            return &EuropaGlobalSettings.array[i];
    return NULL;
}

boolean europa_setting_boolean_get(const char *setting, boolean default_value, const char *comment)
{
    EuropaSetting *s;
    char *text;
    init_setting();
    s = europa_setting_get(setting);
    if (s == NULL) {
        s = europa_add_setting(setting, comment);
        s->data.toggle = default_value;
        s->type = EUROPA_ST_BOOLEAN;
    }
    else if (s->type != EUROPA_ST_BOOLEAN) {
        text = s->data.text;
        s->data.toggle = default_value;
        if (text[0] != 0) {
            if (text[0] == 'T' || text[0] == 't')
                s->data.toggle = TRUE;
            if (text[0] == 'F' || text[0] == 'f' || (text[0] > '0' && text[0] <= '9'))
                s->data.toggle = FALSE;
        }
        free(text);
        s->type = EUROPA_ST_BOOLEAN;
    }
    if (comment != NULL)
        s->comment = comment;
    return s->data.toggle;
}

void europa_setting_boolean_set(const char *setting, boolean value, const char *comment)
{
    EuropaSetting *s;
    init_setting();
    s = europa_setting_get(setting);
    if (s == NULL)
        s = europa_add_setting(setting, comment);
    else if (s->type == EUROPA_ST_UNDEFINED || s->type == EUROPA_ST_TEXT) {
        free(s->data.text);
    }
    s->data.toggle = value;
    if (comment != NULL)
        s->comment = comment;
    s->type = EUROPA_ST_BOOLEAN;
}

int europa_setting_integer_get(const char *setting, int default_value, const char *comment)
{
    EuropaSetting *s;
    char *text;
    init_setting();
    s = europa_setting_get(setting);
    if (s == NULL) {
        s = europa_add_setting(setting, comment);
        s->data.integer = default_value;
        s->type = EUROPA_ST_INTEGER;
    }
    else if (s->type != EUROPA_ST_INTEGER) {
        text = s->data.text;
        s->data.integer = default_value;
        if (sscanf(text, "%u", &s->data.integer) == 1)
            free(text);
        s->type = EUROPA_ST_INTEGER;
    }
    if (comment != NULL)
        s->comment = comment;
    return s->data.integer;
}

void europa_setting_integer_set(const char *setting, int value, const char *comment)
{
    EuropaSetting *s;
    init_setting();
    s = europa_setting_get(setting);
    if (s == NULL)
        s = europa_add_setting(setting, comment);
    else if (s->type == EUROPA_ST_UNDEFINED || s->type == EUROPA_ST_TEXT) {
        free(s->data.text);
    }
    s->data.integer = value;
    s->type = EUROPA_ST_INTEGER;
    if (comment != NULL)
        s->comment = comment;
}

double europa_setting_double_get(const char *setting, double default_value, const char *comment)
{
    EuropaSetting *s;
    char *text;
    init_setting();
    s = europa_setting_get(setting);
    if (s == NULL) {
        s = europa_add_setting(setting, comment);
        s->data.real = default_value;
        s->type = EUROPA_ST_DOUBLE;
    }
    else if (s->type != EUROPA_ST_DOUBLE) {
        float f;
        text = s->data.text;
        s->data.real = default_value;

        if (sscanf(text, "%f", &f) == 1) {
            s->data.real = f;
            free(text);
        }
        s->type = EUROPA_ST_DOUBLE;
    }
    if (comment != NULL)
        s->comment = comment;
    return s->data.real;
}

void europa_setting_double_set(const char *setting, double value, const char *comment)
{
    EuropaSetting *s;
    init_setting();
    s = europa_setting_get(setting);
    if (s == NULL)
        s = europa_add_setting(setting, comment);
    else if (s->type == EUROPA_ST_UNDEFINED || s->type == EUROPA_ST_TEXT) {
        free(s->data.text);
    }
    s->data.real = value;
    s->type = EUROPA_ST_DOUBLE;
    if (comment != NULL)
        s->comment = comment;
}

char *europa_setting_text_get(const char *setting, char *default_text, const char *comment)
{
    EuropaSetting *s;
    char *text;
    uint i;
    init_setting();
    s = europa_setting_get(setting);
    if (s == NULL) {
        s = europa_add_setting(setting, comment);
        for (i = 0; default_text[i] != 0; i++);
        text = malloc((sizeof *text) * (i + 1));
        for (i = 0; default_text[i] != 0; i++)
            text[i] = default_text[i];
        text[i] = 0;
        s->data.text = text;
        s->type = EUROPA_ST_TEXT;
    }
    if (comment != NULL)
        s->comment = comment;
    return s->data.text;
}

void europa_setting_text_set(const char *setting, char *text, const char *comment)
{
    EuropaSetting *s;
    char *t;
    uint i;
    init_setting();
    s = europa_setting_get(setting);
    if (s == NULL)
        s = europa_add_setting(setting, comment);
    else if (s->type == EUROPA_ST_UNDEFINED || s->type == EUROPA_ST_TEXT) {
        free(s->data.text);
    }
    for (i = 0; text[i] != 0; i++);
    t = malloc((sizeof *t) * (i + 1));
    for (i = 0; text[i] != 0; i++)
        t[i] = text[i];
    t[i] = 0;
    s->data.text = t;
    s->type = EUROPA_ST_TEXT;
    if (comment != NULL)
        s->comment = comment;
}

boolean europa_setting_test(const char *setting)
{
    return NULL != europa_setting_get(setting);
}

void europa_settings_save(const char *file_name)
{
    uint i;
    FILE *settings;
    init_setting();

    settings = fopen(file_name, "w");
    if (settings == NULL)
        return;
    fprintf(settings, "<?xml version=\"1.0\"?>\n<ROOT>\n");

    for (i = 0; i < EuropaGlobalSettings.count; i++) {
        switch (EuropaGlobalSettings.array[i].type) {
            case EUROPA_ST_UNDEFINED:
                fprintf(settings, "\t<%s>%s</%s>",
                        EuropaGlobalSettings.array[i].name,
                        EuropaGlobalSettings.array[i].data.text,
                        EuropaGlobalSettings.array[i].name);
            break;
            case EUROPA_ST_BOOLEAN:
                if (EuropaGlobalSettings.array[i].data.toggle)
                    fprintf(settings, "\t<%s>TRUE</%s>",
                            EuropaGlobalSettings.array[i].name,
                            EuropaGlobalSettings.array[i].name);
                else
                    fprintf(settings, "\t<%s>FALSE</%s>",
                            EuropaGlobalSettings.array[i].name,
                            EuropaGlobalSettings.array[i].name);
            break;
            case EUROPA_ST_INTEGER:
                fprintf(settings, "\t<%s>%i</%s>",
                        EuropaGlobalSettings.array[i].name,
                        EuropaGlobalSettings.array[i].data.integer,
                        EuropaGlobalSettings.array[i].name);
            break;
            case EUROPA_ST_DOUBLE:
                fprintf(settings, "\t<%s>%f</%s>",
                        EuropaGlobalSettings.array[i].name,
                        EuropaGlobalSettings.array[i].data.real,
                        EuropaGlobalSettings.array[i].name);
            break;
            case EUROPA_ST_TEXT:
                fprintf(settings, "\t<%s>%s</%s>",
                        EuropaGlobalSettings.array[i].name,
                        EuropaGlobalSettings.array[i].data.text,
                        EuropaGlobalSettings.array[i].name);
            break;
        }
        if (EuropaGlobalSettings.array[i].comment == NULL)
            fprintf(settings, "\n");
        else
            fprintf(settings, " <!-- %s -->\n", EuropaGlobalSettings.array[i].comment);
    }
    fprintf(settings, "</ROOT>\n");
    fclose(settings);
}

boolean europa_settings_load(const char *file_name)
{
    char *buffer, type[256], *text;
    uint i, j, size;
    FILE *settings;
    EuropaSetting *s;
    init_setting();

    if ((settings = fopen(file_name, "r")) != NULL) {
        fseek(settings, 0, SEEK_END);
        size = ftell(settings) + 1;
        fseek(settings, 0, SEEK_SET);
        buffer = malloc(size + 1);
        fread(buffer, sizeof(char), size, settings);
        buffer[size] = 0;
        for (i = 0; buffer[i] != 0; i++) {
            if (buffer[i] == '<') {
                i++;
                if (buffer[i] != '/' && buffer[i] != '!') {
                    for (j = 0; buffer[i] != 0 && buffer[i] != '>'; i++)
                        type[j++] = buffer[i];
                    type[j] = 0;
                    i++;
                    for (j = 0; buffer[i + j] != 0 && buffer[i + j] != '<'; j++);
                    if (buffer[i + j] != 0 && j != 0) {
                        j++;
                        if (buffer[i + j] == '/') {
                            text = malloc(sizeof(char) * j);
                            for (j = 0; buffer[i + j] != 0 && buffer[i + j] != '<'; j++)
                                text[j] = buffer[i + j];
                            text[j] = 0;
                            s = europa_add_setting(type, NULL);
                            s->data.text = text;
                        }
                    }
                }
            }
        }
        fclose(settings);
        free(buffer);
        return TRUE;
    }
    return FALSE;
}

void europa_settings_test(void)
{
    europa_settings_load("settings.xml");
    europa_setting_boolean_get("BOOLEAN", TRUE, "This is a switch!");
    europa_setting_integer_get("INTEGER", 1225, "This is an integer number.");
    europa_setting_double_get("DOUBLE", 3.14, "This is a double precision floating point.");
    europa_setting_text_get("TEXT", "Hello, Sailor!", "This is some text");
    europa_settings_save("settings.xml");
    europa_settings_load("settings2.xml");
    europa_setting_boolean_get("BOOLEAN2", FALSE, "This is a switch 2!");
    europa_setting_integer_get("INTEGER2", 5221, "This is an integer number 2.");
    europa_setting_double_get("DOUBLE2", 3.141, "This is a double precision floating point 2.");
    europa_setting_text_get("TEXT2", "Hello, Sailor2!", "This is some text2");
    europa_settings_save("settings2.xml");
}
