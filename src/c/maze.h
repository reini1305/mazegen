#ifndef MAZE_H
#define MAZE_H

#include "cell.h"

#define MAZE_SIZE_X 12
#define MAZE_SIZE_Y 10
#define CELL_SIZE   12
#define END_X       11
#define END_Y       9

void maze_generate(cell* maze);
void maze_solve(cell* maze);
unsigned int maze_ind2sub(uint8_t x, uint8_t y);
void maze_sub2ind(unsigned int ind, int* x, int* y);

#endif
