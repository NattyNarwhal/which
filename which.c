/*
 * Copyright (c) 2020 Calvin Buckley <calvin@cmpct.info>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* For getopt and PATH_MAX */
#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 2
#endif
/* For strdup (don't define on AIX, readlink64at linkage failure) */
#if !defined(_XOPEN_SOURCE) && !defined(_AIX)
# define _XOPEN_SOURCE 700
#endif

#include <limits.h>
#include <paths.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Some systems (like AIX) don't define this. */
#ifndef _PATH_DEFPATH
# define _PATH_DEFPATH "/usr/bin:/bin"
#endif

/*
 * No names were resolved. Used when there was an error calling the program,
 * or we tried and found none.
 */
#define WHICH_NONE 2
/*
 * We found some, but not all names.
 */
#define WHICH_NOT_ALL 1

static char *argv0;

static void
usage(void)
{
  fprintf(stderr, "%s [-aps] [--] PROGRAM [...]\n", argv0);
  exit(WHICH_NONE);
}

/**
 * Creates a full path from a directory and file, adding a slash between them
 * if necessary.
 */
static void
create_full_path(char *path, const size_t size, const char *dir,
                 const char *file)
{
  char *slash = dir[strlen(dir) - 1] == '/' ? "" : "/";
  snprintf(path, size, "%s%s%s", dir, slash, file);
}

/**
 * Checks if the program exists at the path, and (optionally) prints the path
 * if found.
 */
static bool
try_print(const char *path, const char *name, const bool prefix_name,
          const bool quiet)
{
  /* Check for executability, not just existence. */
  bool this_success = access(path, X_OK) == 0;
  if (this_success && !quiet) {
    if (prefix_name) {
      printf("%s: %s\n", name, path);
    } else {
      printf("%s\n", path);
    }
  }
  return this_success;
}

/**
 * Iterates through path entries, (optionally) printing the path if found.
 */
static bool
try_find (char *name, const char *items, const char *original_path,
          const size_t path_size, const bool try_all, const bool prefix_name,
          const bool quiet)
{
  bool success = false;
  const char *path_maybe;
  char *name_maybe, new_path[PATH_MAX];
  /* 
   * Emulate GNU which where if the path has directory components, check only
   * those instead of $PATH.
   */
  if (strchr(name, '/') != NULL) {
    /* Copy original path, since we'll mutate name. */
    strncpy(new_path, name, PATH_MAX - 1);
    /* Mutate name, changing the last '/' to a NUL and setting each maybe. */
    path_maybe = name;
    name_maybe = strrchr(name, '/');
    *name_maybe++ = '\0';
    /* Because we now know name itself is a path.. */
    success = try_print(new_path, name_maybe, prefix_name, quiet);
  } else {
    path_maybe = original_path;
    name_maybe = name;
    for (const char *path = items;
         path < (items + path_size);
         path += strlen(path) + 1) {
      create_full_path(new_path, PATH_MAX, path, name);
      success |= try_print (new_path, name_maybe, prefix_name, quiet);
      if (success && !try_all) {
        return true;
      }
    }
  }
  if (!success && !quiet) {
    fprintf(stderr, "%s: no %s in (%s)\n", argv0, name_maybe, path_maybe);
  }
  return success;
}

/**
 * Converts $PATH colons into NULs, so it can be used as a packed array,
 * terminated by a double NUL.
 */
static bool
create_path_items(const char *original_path, const char **items, size_t *size)
{
  /*
   * We turn each : into a NULL, and append an additional NULL to terminate.
   * 
   * We don't need to escape a colon per SUS:
   * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html#tag_08_03
   */
  size_t original_path_size = strlen(original_path);
  size_t new_path_size = original_path_size + 1;
  char *new_path = (char*)malloc(new_path_size);
  if (new_path == NULL) {
    return false;
  }
  /* memcpy is OK because we're going to append an additional NUL anyways */
  memcpy(new_path, original_path, original_path_size);
  new_path[original_path_size] = '\0';
  char *colon = new_path;
  while ((colon = strchr(colon, ':')) != NULL) {
    *colon++ = '\0';
  }
  
  *items = (const char*)new_path;
  *size = new_path_size;
  return true;
}

int
main (int argc, char **argv)
{
  bool try_all = false, prefix_name = false, quiet = false;
  int errors = 0, attempts = 0, ch;
  argv0 = argv[0];
  while ((ch = getopt(argc, argv, "asp")) != -1) {
		switch (ch) {
		case 'a':
      try_all = true;
			break;
    case 'p':
      prefix_name = true;
      break;
    case 's':
      quiet = true;
      break;
		default:
			usage();
		}
	}
  if (argc == optind) { /* no args */
    usage();    
  }
  
  const char *original_path = getenv("PATH");
  if (original_path == NULL || original_path[0] == '\0') {
    original_path = _PATH_DEFPATH;
  }
  
  const char *items = NULL;
  size_t size = -1;
  if (!create_path_items (original_path, &items, &size)) {
    fprintf(stderr, "%s: error allocating path entries\n", argv0);
  }
  for (int i = optind; i < argc; i++) {
    if (!try_find (argv [i], items, original_path, size, try_all, prefix_name,
                   quiet)) {
      errors++;
    }
    attempts++;
  }
  
  free((void*)items);
  if (errors == attempts)
    return WHICH_NONE;
  else if (errors)
    return WHICH_NOT_ALL;
  else return 0;
}

