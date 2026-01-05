#ifndef MEMORY_USAGE_H
#define MEMORY_USAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file memory_usage.h
 * @brief Header-only utility to get current process memory usage (KB or MB), thread-safe.
 *
 * @details
 * - Cross-platform: Linux and Windows.
 * - No heap allocation; no free() needed.
 * - Thread-safe using _Thread_local (C11 or supported compiler).
 * - Multiple output formats: KB, MB, JSON string.
 *
 * @return const char* Pointer to thread-local buffer containing memory usage string.
 */

/// Get memory usage in KB (e.g., "12345 KB")
const char *get_memory_usage_str_kb();

/// Get memory usage in MB (e.g., "12.06 MB")
const char *get_memory_usage_str_mb();

/// Get memory usage as JSON (e.g., {"memory_kb":12345,"memory_mb":12.06})
const char *get_memory_usage_json();

#ifdef __cplusplus
}
#endif

// ============ IMPLEMENTATION ============

#if defined(_WIN32)

#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#pragma comment(lib, "psapi.lib")

static size_t get_memory_kb()
{
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
  {
    return pmc.WorkingSetSize / 1024;
  }
  return 0;
}

#elif defined(__linux__)

#include <stdio.h>
#include <string.h>

static size_t get_memory_kb()
{
  FILE *file = fopen("/proc/self/status", "r");
  if (!file) return 0;

  char line[256];
  size_t memory_kb = 0;

  while (fgets(line, sizeof(line), file))
  {
    if (strncmp(line, "VmRSS:", 6) == 0)
    {
      sscanf(line + 6, "%zu", &memory_kb);
      break;
    }
  }

  fclose(file);
  return memory_kb;
}

#else

static size_t get_memory_kb()
{
  return 0;
}

#endif

// ---- Thread-local buffer ----
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__)
#define THREAD_LOCAL __thread
#else
#define THREAD_LOCAL
#warning "No thread-local support: buffer may not be thread-safe"
#endif

THREAD_LOCAL static char mem_buf[64];

const char *get_memory_usage_str_kb()
{
  size_t kb = get_memory_kb();
  snprintf(mem_buf, sizeof(mem_buf), "%zu KB", kb);
  return mem_buf;
}

const char *get_memory_usage_str_mb()
{
  size_t kb = get_memory_kb();
  double mb = kb / 1024.0;
  snprintf(mem_buf, sizeof(mem_buf), "%.2f MB", mb);
  return mem_buf;
}

const char *get_memory_usage_json()
{
  size_t kb = get_memory_kb();
  double mb = kb / 1024.0;
  snprintf(mem_buf, sizeof(mem_buf), "{\"memory_kb\":%zu,\"memory_mb\":%.2f}", kb, mb);
  return mem_buf;
}

#endif // MEMORY_USAGE_H
