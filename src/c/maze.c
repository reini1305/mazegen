#include "maze.h"
#include "cell.h"
#include <@smallstoneapps/linked-list/linked-list.h>

static LinkedRoot* stack;
static int offsets[] = {-1,1,-MAZE_SIZE_X, MAZE_SIZE_X};

static int prv_get_random_neighbor(cell* maze, int id) {
  bool visited[] = {false,false,false,false};
  int x; int y;
  maze_sub2ind(id,&x,&y);
  while (!visited[0] || !visited[1] || !visited[2] || !visited[3]) {
    int rand_id = rand()%4;
    visited[rand_id] = true;
    if(!(rand_id==0 && x==0) && !(rand_id==1 && x==MAZE_SIZE_X-1) && !(rand_id==2 && y==0) && !(rand_id==3 && y==MAZE_SIZE_Y-1) && !maze[id+offsets[rand_id]].visited) {
      return id+offsets[rand_id];
    }
  }
  // well, we didn't find any unvisited neighbor
  return -1;
}

static void prv_crush_wall(cell* maze, int id1, int id2) {
  if(id2-id1 == 1) { // 2 is right of 1
    maze[id1].walls &= ~W;
    maze[id2].walls &= ~E;
  } else if (id1-id2 == 1) {
    maze[id1].walls &= ~E;
    maze[id2].walls &= ~W;
  } else if(id2-id1 == MAZE_SIZE_X) { // 2 is below of 1
    maze[id1].walls &= ~S;
    maze[id2].walls &= ~N;
  } else if (id1-id2 == MAZE_SIZE_X) {
    maze[id1].walls &= ~N;
    maze[id2].walls &= ~S;
  }
}

static void prv_push_to_stack(unsigned int value) {
  unsigned int *id = malloc(sizeof(unsigned int));
  *id = value;
  linked_list_append(stack,id);
}

static unsigned int prv_pop_from_stack() {
  unsigned int *id = linked_list_get(stack,0);
  unsigned int retval = *id;
  free(id);
  linked_list_remove(stack,0);
  return retval;
}

static bool prv_recursive_solve(cell* maze, int x, int y) {
    if (x == END_X && y == END_Y){
      maze[maze_ind2sub(x,y)].correct_path = true;
      return true; // If you reached the end
    }
    if (maze[maze_ind2sub(x,y)].visited) return false;
    // If you are on a wall or already were here
    maze[maze_ind2sub(x,y)].visited = true;
    if (x != 0) // Checks if not on left edge
        if ((maze[maze_ind2sub(x,y)].walls & E)==false && prv_recursive_solve(maze, x-1, y)) { // Recalls method one to the left
            maze[maze_ind2sub(x,y)].correct_path = true; // Sets that path value to true;
            return true;
        }
    if (x != MAZE_SIZE_X - 1) // Checks if not on right edge
        if ((maze[maze_ind2sub(x,y)].walls & W)==false && prv_recursive_solve(maze, x+1, y)) { // Recalls method one to the right
            maze[maze_ind2sub(x,y)].correct_path = true;
            return true;
        }
    if (y != 0)  // Checks if not on top edge
        if ((maze[maze_ind2sub(x,y)].walls & N)==false && prv_recursive_solve(maze, x, y-1)) { // Recalls method one up
            maze[maze_ind2sub(x,y)].correct_path = true;
            return true;
        }
    if (y != MAZE_SIZE_Y - 1) // Checks if not on bottom edge
        if ((maze[maze_ind2sub(x,y)].walls & S)==false && prv_recursive_solve(maze, x, y+1)) { // Recalls method one down
            maze[maze_ind2sub(x,y)].correct_path = true;
            return true;
        }
    return false;
}

void maze_generate(cell* maze){
  for(unsigned int it=0;it<MAZE_SIZE_X*MAZE_SIZE_Y;it++){
      maze[it].visited = false;
      maze[it].correct_path = false;
      maze[it].walls = N | S | E | W;
  }
  stack = linked_list_create_root();

  //start in the left upper corner
  unsigned int current_cell = 0;
  prv_push_to_stack(current_cell);
  maze[current_cell].visited = true;

  while(linked_list_count(stack)>0) {

    int next_neighbor = prv_get_random_neighbor(maze,current_cell);
    if (next_neighbor>0) { // there is still an unvisited neighbor
      // crush wall to next_neighbor
      prv_crush_wall(maze,current_cell,next_neighbor);
      // mark neighbor as visited
      prv_push_to_stack(next_neighbor);
      maze[next_neighbor].visited = true;
      current_cell = next_neighbor;
    } else {
      current_cell = prv_pop_from_stack();
    }
  }
  free(stack);
}

void maze_solve(cell* maze) {
  for(unsigned int it=0;it<MAZE_SIZE_X*MAZE_SIZE_Y;it++){
    maze[it].visited = false;
  }
  prv_recursive_solve(maze,0,0);
}

unsigned int maze_ind2sub(uint8_t x, uint8_t y){
  return x + MAZE_SIZE_X * y;
}

void maze_sub2ind(unsigned int ind, int* x, int* y){
  *x = ind % MAZE_SIZE_X;
  *y = ind / MAZE_SIZE_X;
}
