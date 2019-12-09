#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dict.h"

struct dict_item {
  // Each word is at most 100 bytes.
  char word[100];
  // The length of each word.
  size_t len;
};

struct dict_t {
  // The path to the underlying file
  char *path;
  // The file descriptor of the opened file. Set by dictionary_open_map.
  int fd;
  // How many items the mmaped file should store (this will determine the size).
  // There are ~500k words in /usr/share/dict/words.
  size_t num_items;
  // A pointer to the first item.
  struct dict_item *base;
};

// Construct a new dict_t struct.
// data_file is where to write the data,
// num_items is how many items this data file should store.
struct dict_t *dictionary_new(char *data_file, size_t num_items) {
  struct dict_t *dict;
  int fd = open(data_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    perror("open failure");
    return NULL;
  }
  void *ptr = mmap(NULL, num_items * sizeof(struct dict_item), PROT_READ,
                    MAP_SHARED, fd, 0);
  if (dict == MAP_FAILED) {
    perror("mapping failure");
    return NULL;
  }
  // getting segfaults here
  dict = ptr;
  dict->path = data_file;
  dict->num_items = num_items;
  dict->fd = fd;
  return dict;
}

// Computes the size of the underlying file based on the # of items and the size
// of a dict_item.
size_t dictionary_len(struct dict_t *dict) {
  return dict->num_items * sizeof(struct dict_item);
}

// This is a private helper function. It should:
// Open the underlying path (dict->path), ftruncate it to the appropriate length
// (dictionary_len), then mmap it.
int dictionary_open_map(struct dict_t *dict) {
  int fd = open(dict->path, O_RDWR);
  if (fd == -1) {
    perror("open failure");
    return EXIT_FAILURE;
  }
  if (ftruncate(fd, dict->num_items) == -1) {
    perror("truncate failure");
    return EXIT_FAILURE;
  }
  void *base =
      mmap(NULL, dict->num_items, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (base == MAP_FAILED) {
    perror("mapping failure");
    return EXIT_FAILURE;
  }
  dict->fd = fd;
  dict->base = base;
  return 0; // or return fd?
}

// The rest of the functions should be whatever is left from the header that
// hasn't been defined yet.
// Good luck!

int dictionary_generate(struct dict_t *dict, char *input) {
  struct dict_item *temp;
  temp = dict->base;
  char str[100] = { 0 };
  FILE *fp;
  fp = fopen(dict->path, "r");
  while (fgets(str, 100, fp)) {
    strcpy(temp->word, str);
    temp->len = strlen(str);
    temp += sizeof(struct dict_item);
  }
  fclose(fp);
  return 0;
}

char *dictionary_exists(struct dict_t *dict, char *word) {
  struct dict_item *temp;
  temp = dict->base;
  for (int i = 0; i < dict->num_items; i++) {
    if (strcmp(temp->word, word) == 0) {
      return word;
    }
    temp += sizeof(struct dict_item);
  }
  return NULL;
}

int dictionary_load(struct dict_t *dict) {
  // open from dict->path?
}

void dictionary_close(struct dict_t *dict) {
  munmap(dict->base, dict->num_items * sizeof(struct dict_item));
}

int dictionary_larger_than(struct dict_t *dict, size_t n) {
  struct dict_item *temp;
  temp = dict->base;
  int count = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (strlen(temp->word) > n) {
      count++;
    }
    temp += sizeof(struct dict_item);
  }
  return count;
}

int dictionary_smaller_than(struct dict_t *dict, size_t n) {
  struct dict_item *temp;
  temp = dict->base;
  int count = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (strlen(temp->word) < n) {
      count++;
    }
  }
  return count;
}

int dictionary_equal_to(struct dict_t *dict, size_t n) {
  struct dict_item *temp;
  temp = dict->base;
  int count = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (strlen(temp->word) == n) {
      count++;
    }
  }
  return count;
}
