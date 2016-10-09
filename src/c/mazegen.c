#include <pebble.h>
#include "enamel.h"
#include <pebble-events/pebble-events.h>
#include "cell.h"
#include "maze.h"

static cell maze[MAZE_SIZE_X*MAZE_SIZE_Y];

static Window *window;
static Layer *maze_layer;
static TextLayer *time_layer;
static char time_text[] = "     ";
static bool show_solution = false;
static AppTimer *hide_solution_timer;
static EventHandle event_handle;
static bool has_been_solved = false;

#define OFFSET_BOTTOM 45

void unobstructed_change(AnimationProgress progress, void* data) {
  GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(window));
  // update layer positions
  layer_set_frame(text_layer_get_layer(time_layer),GRect(0, bounds.size.h-OFFSET_BOTTOM, bounds.size.w, OFFSET_BOTTOM));
}

static void handle_bluetooth(bool connected){
  if(enamel_get_vibrate() && !connected)
    vibes_long_pulse();
}

static void enamel_settings_received_handler(void *context){
  text_layer_set_background_color(time_layer, enamel_get_colorscheme()==1?GColorWhite:GColorBlack);
  text_layer_set_text_color(time_layer, enamel_get_colorscheme()==0?GColorWhite:GColorBlack);
  layer_mark_dirty(maze_layer);
}

static void maze_layer_draw(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, enamel_get_colorscheme()==1?GColorWhite:GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx,enamel_get_colorscheme()==0?GColorWhite:GColorBlack);
  graphics_context_set_stroke_width(ctx,2);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorGreen,GColorLightGray));

  const uint8_t cell_width = CELL_SIZE;
  for(uint8_t x=0;x<MAZE_SIZE_X;x++) {
    for(uint8_t y=0; y<MAZE_SIZE_Y;y++) {
      uint8_t walls = maze[maze_ind2sub(x,y)].walls;
      if(show_solution && maze[maze_ind2sub(x,y)].correct_path) {
        graphics_fill_rect(ctx,GRect(x*cell_width,y*cell_width,cell_width,cell_width),0,GCornerNone);
      }
      if(walls & N)
        graphics_draw_line(ctx, GPoint(x*cell_width,y*cell_width),
                                GPoint((x+1)*cell_width,y*cell_width));
      if(walls & S)
        graphics_draw_line(ctx, GPoint(x*cell_width,(y+1)*cell_width),
                                GPoint((x+1)*cell_width,(y+1)*cell_width));
      if(walls & E)
        graphics_draw_line(ctx, GPoint(x*cell_width,y*cell_width),
                                GPoint(x*cell_width,(y+1)*cell_width));
      if(walls & W)
        graphics_draw_line(ctx, GPoint((x+1)*cell_width,y*cell_width),
                                GPoint((x+1)*cell_width,(y+1)*cell_width));
    }
  }
}

static void update_time(struct tm *tick_time) {
  char *time_format;
	if (clock_is_24h_style()) {
	    time_format = "%R";
	} else {
	    time_format = "%I:%M";
	}

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }
}

static void tickHandler(struct tm *tick_time, TimeUnits units) {
  update_time(tick_time);
  maze_generate(maze);
  has_been_solved = false;
  //maze_solve(maze);
  layer_mark_dirty(maze_layer);
  layer_mark_dirty(text_layer_get_layer(time_layer));
}

void hide_solution(void* data) {
  show_solution = false;
  layer_mark_dirty(maze_layer);
  if(enamel_get_date()){
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    update_time(t);
    layer_mark_dirty(text_layer_get_layer(time_layer));
  }
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  if(!has_been_solved) {
    maze_solve(maze);
    has_been_solved=true;
  }
  show_solution = true;
  if(enamel_get_date()){
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char* time_format = "%m %d";
    strftime(time_text, sizeof(time_text), time_format, t);
    layer_mark_dirty(text_layer_get_layer(time_layer));
  }
  layer_mark_dirty(maze_layer);
  if(!app_timer_reschedule(hide_solution_timer,5000))
    hide_solution_timer = app_timer_register(5000,hide_solution,NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_unobstructed_bounds(window_layer);

  time_layer = text_layer_create(GRect(0, bounds.size.h-OFFSET_BOTTOM, bounds.size.w, OFFSET_BOTTOM));
  text_layer_set_background_color(time_layer, enamel_get_colorscheme()==1?GColorWhite:GColorBlack);
  text_layer_set_text_color(time_layer, enamel_get_colorscheme()==0?GColorWhite:GColorBlack);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_text(time_layer,time_text);

  maze_layer = layer_create(bounds);

  layer_add_child(window_layer, maze_layer);
  layer_set_update_proc(maze_layer, maze_layer_draw);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));
  // force update
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  tickHandler(t, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, tickHandler);

  bluetooth_connection_service_subscribe(handle_bluetooth);

  accel_tap_service_subscribe(&accel_tap_handler);
}

static void window_unload(Window *window) {

}

static void init(void) {

  enamel_init();
  event_handle = enamel_settings_received_subscribe(enamel_settings_received_handler,NULL);

  events_app_message_open();
  window = window_create();

  //window_set_fullscreen(window, true);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
#if PBL_API_EXISTS(unobstructed_area_service_subscribe)
  UnobstructedAreaHandlers handlers = {
    // .will_change = unobstructed_will_change,
    .change = unobstructed_change
    // .did_change = unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);
#endif
  window_stack_push(window, true);
}

static void deinit(void) {
  window_destroy(window);
  enamel_settings_received_unsubscribe(event_handle);
  enamel_deinit();
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
