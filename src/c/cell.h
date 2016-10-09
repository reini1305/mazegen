#ifndef CELL_H
#define CELL_H

#include <pebble.h>

typedef enum {
  N = 0x1,
  S = 0x2,
  W = 0x4,
  E = 0x8
} WALL;

typedef struct cell{
  bool visited;
  bool correct_path;
  uint8_t walls;
}cell;

#endif
