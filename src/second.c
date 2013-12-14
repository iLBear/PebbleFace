#include "second.h"

#include "pebble.h"
#include "QTPlus.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"

Layer *simple_bg_layer;

Layer *date_layer;
TextLayer *day_label;
char day_buffer[6];
TextLayer *num_label;
char num_buffer[4];
TextLayer *sec_label;
char sec_buffer[4];

static GPath *minute_arrow;
static GPath *hour_arrow;
static GPath *tick_paths[NUM_CLOCK_TICKS];
Layer *hands_layer;
Window *window;
int32_t mode;	//watch-mode which changes when shoke the watch
GPoint center;
TextLayer *text_time_layer;


static void bg_update_proc(Layer *layer, GContext *ctx) {
	
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
	
	//graphics_context_set_fill_color(ctx, GColorWhite);
	//for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    //gpath_draw_filled(ctx, tick_paths[i]);
	//}
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	center = grect_center_point(&bounds);
	const int16_t secondHandLength = bounds.size.w / 2;
	GPoint secondHand;
	
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	
	int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
	secondHand.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.y;
	secondHand.x = (int16_t)(sin_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.x;
	
	// second hand
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_draw_line(ctx, secondHand, center);
	
	// minute/hour hand
	if(mode != 0){
		graphics_context_set_fill_color(ctx, GColorWhite);
		graphics_context_set_stroke_color(ctx, GColorBlack);
	}
	
	//minute hand
	if(mode == 1){
		gpath_rotate_to(minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
		gpath_draw_filled(ctx, minute_arrow);
		gpath_draw_outline(ctx, minute_arrow);
	}
	
	//hour hand
	if(mode == 2){
		gpath_rotate_to(hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
		gpath_draw_filled(ctx, hour_arrow);
		gpath_draw_outline(ctx, hour_arrow);
	}
	
	// dot in the middle
	//graphics_context_set_fill_color(ctx, GColorBlack);
	//graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
	
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	
	if(mode != 0){
	//weekday
	strftime(day_buffer, sizeof(day_buffer), "%a", t);
	text_layer_set_text(day_label, day_buffer);
	
	//day
	strftime(num_buffer, sizeof(num_buffer), "%d", t);
	text_layer_set_text(num_label, num_buffer);
	}
}

//144*168 px
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	
	layer_mark_dirty(window_get_root_layer(window));	//reflesh face
	
	const int RAD = 10;
	int32_t angle = TRIG_MAX_ANGLE * ((tick_time->tm_sec+30)%60) / 60;
	
	
	GPoint secondHand;
	secondHand.y = (int16_t)(-cos_lookup(angle) * RAD / TRIG_MAX_RATIO) + center.y;
	secondHand.x = (int16_t)(sin_lookup(angle) * RAD / TRIG_MAX_RATIO) + center.x;
	
	//second
	strftime(sec_buffer, sizeof(sec_buffer), "%S", tick_time);	//get second
	text_layer_set_text(sec_label, sec_buffer);
	
	text_layer_destroy(sec_label);
	if(numFollowHand){
		sec_label = text_layer_create(GRect(secondHand.x-7, secondHand.y-12, 14, 18));	//around the center of the face
	}else{
		sec_label = text_layer_create(GRect(rand()%124+10 , rand()%148, 14, 18));
	}
	text_layer_set_text(sec_label, sec_buffer);
	text_layer_set_background_color(sec_label, GColorBlack);
	text_layer_set_text_color(sec_label, GColorWhite);
	GFont norm18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
	text_layer_set_font(sec_label, norm18);
	layer_add_child(date_layer, text_layer_get_layer(sec_label));
	
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	// init layers
	simple_bg_layer = layer_create(bounds);
	layer_set_update_proc(simple_bg_layer, bg_update_proc);
	layer_add_child(window_layer, simple_bg_layer);
	
	// init date layer -> a plain parent layer to create a date update proc
	date_layer = layer_create(bounds);
	layer_set_update_proc(date_layer, date_update_proc);
	layer_add_child(window_layer, date_layer);
	
	// init day
	day_label = text_layer_create(GRect(110, 149, 27, 20));
	text_layer_set_text(day_label, day_buffer);
	text_layer_set_background_color(day_label, GColorBlack);
	text_layer_set_text_color(day_label, GColorWhite);
	GFont norm18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
	text_layer_set_font(day_label, norm18);
	
	layer_add_child(date_layer, text_layer_get_layer(day_label));
	
	// init num
	num_label = text_layer_create(GRect(127, 149, 18, 20));
	text_layer_set_text(num_label, num_buffer);
	text_layer_set_background_color(num_label, GColorBlack);
	text_layer_set_text_color(num_label, GColorWhite);
	GFont bold18 = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
	text_layer_set_font(num_label, bold18);
	layer_add_child(date_layer, text_layer_get_layer(num_label));
	
	// init sec
	sec_label = text_layer_create(GRect(10, 10, 27, 20));
	text_layer_set_text(sec_label, sec_buffer);
	text_layer_set_background_color(sec_label, GColorBlack);
	text_layer_set_text_color(sec_label, GColorWhite);
	text_layer_set_font(sec_label, norm18);
	layer_add_child(date_layer, text_layer_get_layer(sec_label));
	
	// init hands
	hands_layer = layer_create(bounds);
	layer_set_update_proc(hands_layer, hands_update_proc);
	layer_add_child(window_layer, hands_layer);
	
}

static void window_unload(Window *window) {
	layer_destroy(simple_bg_layer);
	layer_destroy(date_layer);
	text_layer_destroy(day_label);
	text_layer_destroy(num_label);
	layer_destroy(hands_layer);
	text_layer_destroy(sec_label);
}

static void init(void) {
	mode = 0;
	srand(time(NULL));
	numFollowHand = true;
	
	qtp_conf = QTP_K_SHOW_TIME | QTP_K_SHOW_WEATHER | QTP_K_AUTOHIDE | QTP_K_INVERT;
	
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	
	day_buffer[0] = '\0';
	num_buffer[0] = '\0';
	sec_buffer[0] = '\0';
	
	// init hand paths
	minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
	hour_arrow = gpath_create(&HOUR_HAND_POINTS);
	
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	const GPoint center = grect_center_point(&bounds);
	gpath_move_to(minute_arrow, center);
	gpath_move_to(hour_arrow, center);
	
	// init clock face paths
	for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
		tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
	}
	
	// Push the window onto the stack
	const bool animated = true;
	window_stack_push(window, animated);
	
	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void deinit(void) {
	gpath_destroy(minute_arrow);
	gpath_destroy(hour_arrow);
	
	for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
		gpath_destroy(tick_paths[i]);
	}
	qtp_app_deinit();
	tick_timer_service_unsubscribe();
	window_destroy(window);
}

int main(void) {
	init();
	qtp_setup();
	app_event_loop();
	deinit();
}
