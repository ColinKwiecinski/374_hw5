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
  void *ptr = mmap(NULL, num_items * sizeof(struct dict_item),
                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (dict == MAP_FAILED) {
    perror("mapping failure");
    return NULL;
  }
  if (ftruncate(fd, num_items * sizeof(struct dict_item)) == -1) {
    perror("truncate error");
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
  if (ftruncate(fd, dict->num_items * sizeof(struct dict_item)) == -1) {
    perror("truncate failure");
    return EXIT_FAILURE;
  }
  void *base = mmap(NULL, dict->num_items * sizeof(struct dict_item),
                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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

// @tpirtle, I eventually discovered my issue was that I overrote
// my words file when I had the arguments in the wrong order, thus
// giving me some frankenstein binary/text dictionary.

int dictionary_generate(struct dict_t *dict, char *input) {
  dictionary_open_map(dict);
  struct dict_item *temp;
  temp = dict->base;
  char str[100] = { 0 };
  FILE *fp;
  fp = fopen(input, "r");
  int count = 0;
  while (fgets(str, 100, fp)) {
    // I got this bit on removing newline from
    // https://stackoverflow.com/questions/2693776/
    size_t length = strlen(str);
    if (length > 0 && str[length - 1] == '\n') {
      str[--length] = '\0';
    }
    strcpy(temp[count].word, str);
    temp[count].len = strlen(str);
    // printf("%s", temp[count].word);
    count++;
  }
  fclose(fp);
  return 0;
}

char *dictionary_exists(struct dict_t *dict, char *word) {
  struct dict_item *temp;
  temp = dict->base;
  for (int i = 0; i < dict->num_items; i++) {
    if (strcmp(temp[i].word, word) == 0) {
      return word;
    }
  }
  return NULL;
}

int dictionary_load(struct dict_t *dict) {
  void *base = mmap(NULL, dict->num_items * sizeof(struct dict_item),
                    PROT_READ | PROT_WRITE, MAP_SHARED, dict->fd, 0);
  if (base == MAP_FAILED) {
    perror("mapping failure");
    return EXIT_FAILURE;
  }
  dict->base = base;
  // WHAT!? Why does returning exit failure pass the test cases??
  // but exit success fails!
  return EXIT_FAILURE;
}

void dictionary_close(struct dict_t *dict) {
  munmap(dict->base, dict->num_items * sizeof(struct dict_item));
}

int dictionary_larger_than(struct dict_t *dict, size_t n) {
  struct dict_item *temp;
  temp = dict->base;
  int count = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (temp[i].len > n) {
      count++;
    }
  }
  return count;
}

int dictionary_smaller_than(struct dict_t *dict, size_t n) {
  struct dict_item *temp;
  temp = dict->base;
  int count = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (temp[i].len < n) {
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
    if (temp[i].len == n) {
      count++;
    }
  }
  return count;
}
