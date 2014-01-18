#include <pebble.h>
#include "lightbox.h"
	
#define BG_COLOR_DEFAULT GColorBlack
#define FG_COLOR_DEFAULT GColorWhite
#define MAX_LIGHTS 4
#define MAX_STARTUP_SEQUENCES 6
#define CURVE AnimationCurveEaseInOut
#define ANIMATION_DELAY 250
#define ANIMATION_DURATION 750
	
GColor bg_color;
GColor fg_color;

static Layer *light[MAX_LIGHTS];
static Layer *light_test;
static Window *window;
Layer *root_layer;
static BitmapLayer *mask_layer;
static GBitmap *mask;

static PropertyAnimation *light_animation[MAX_LIGHTS];

static GRect location[MAX_LIGHTS];
static GRect from_location[MAX_LIGHTS];

static int my_round(float x)
{
        if((x-(int)x) >= 0.5){return (int)x + 1;}
        else{return (int)x;}
}

static void run_animations()
{
	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		from_location[i] = layer_get_frame(light[i]);
		if(animation_is_scheduled((Animation*)light_animation[i]))
		{
			animation_unschedule((Animation*)light_animation[i]);
		}
		light_animation[i] = property_animation_create_layer_frame(light[i], &from_location[i], &location[i]);
		animation_set_curve((Animation*)light_animation[i], CURVE);
		animation_set_delay((Animation*)light_animation[i], ANIMATION_DELAY + (i * (ANIMATION_DELAY + ANIMATION_DURATION))); //animations fire sequentially
		animation_set_duration((Animation*)light_animation[i], ANIMATION_DURATION);
		animation_schedule((Animation*)light_animation[i]);
	}
}

static void set_locations(struct tm *tick_time)
{
	int hour = (tick_time->tm_hour)%12;
	int minute = my_round((tick_time->tm_min) / 5.0f) % 12;

	int j = rand() % MAX_LIGHTS;
	
	location[j] = ITS;
	j++;
	j = j % MAX_LIGHTS;

	if(minute == 0)
	{
		location[j] = O;
		j++;
		j = j % MAX_LIGHTS;
		
		location[j] = CLOCK;
		j++;
		j = j % MAX_LIGHTS;
	}
	else if(minute == 1 || minute == 11)
	{
		location[j] = FIVE_M;
		j++;
		j = j % MAX_LIGHTS;

		if(minute == 1)
		{
			location[j] = PAST;
		}
		else
		{
			location[j] = TO;
			hour++;
			hour = hour % 12;
		}
		j++;
		j = j % MAX_LIGHTS;
	}
	else if(minute == 2 || minute == 10)
	{
		location[j] = TEN_M;
		j++;
		j = j % MAX_LIGHTS;
		if(minute == 1)
		{
			location[j] = PAST;
		}
		else
		{
			location[j] = TO;
			hour++;
			hour = hour % 12;
		}
		j++;
		j = j % MAX_LIGHTS;
	}
	else if(minute == 3 || minute == 9)
	{
		location[j] = QUARTER;
		j++;
		j = j % MAX_LIGHTS;

		if(minute == 1)
		{
			location[j] = PAST;
		}
		else
		{
			location[j] = TO;
			hour++;
			hour = hour % 12;
		}
		j++;
		j = j % MAX_LIGHTS;
	}
	else if(minute == 4 || minute == 8)
	{
		location[j] = TWENTY;
		j++;
		j = j % MAX_LIGHTS;

		if(minute == 1)
		{
			location[j] = PAST;
		}
		else
		{
			location[j] = TO;
			hour++;
			hour = hour % 12;
		}
		j++;
		j = j % MAX_LIGHTS;
	}
	else if(minute == 5 || minute == 7)
	{
		location[j] = TWENTYFIVE;
		j++;
		j = j % MAX_LIGHTS;

		if(minute == 1)
		{
			location[j] = PAST;
		}
		else
		{
			location[j] = TO;
			hour++;
			hour = hour % 12;
		}
		j++;
		j = j % MAX_LIGHTS;
	}
	else //minute == 6
	{
		location[j] = HALF;
		j++;
		j = j % MAX_LIGHTS;
	
		location[j] = PAST;
		j++;
		j = j % MAX_LIGHTS;
	}
	switch(hour)
	{
		case 1: location[j] = ONE; break;
		case 2: location[j] = TWO; break;
		case 3: location[j] = THREE; break;
		case 4: location[j] = FOUR;	break;
		case 5: location[j] = FIVE_H; break;
		case 6: location[j] = SIX; break;
		case 7: location[j] = SEVEN; break;
		case 8:	location[j] = EIGHT; break;
		case 9:	location[j] = NINE; break;
		case 10: location[j] = TEN_H; break;
		case 11: location[j] = ELEVEN; break;
		case 0: location[j] = TWELVE; break;
	}
}

static void set_startup_locations()
{
	switch(rand() % MAX_STARTUP_SEQUENCES)
	{
		case 0:
			//Four corners
			layer_set_frame(light[0], GRect(-144,-168,144,168));
			layer_set_frame(light[1], GRect(-144,168,144,168));
			layer_set_frame(light[2], GRect(144,168,144,168));
			layer_set_frame(light[3], GRect(144,-168,144,168));
			break;
		default:
			//Center point
			layer_set_frame(light[0], GRect(72,84,0,0));
			layer_set_frame(light[1], GRect(73,84,0,0));
			layer_set_frame(light[2], GRect(73,85,0,0));
			layer_set_frame(light[3], GRect(72,85,0,0));
			break;
	}
}

static void light_draw(Layer *layer, GContext *ctx)
{
	graphics_context_set_fill_color(ctx, fg_color);
	graphics_context_set_stroke_color(ctx, fg_color);
	graphics_fill_rect(ctx, GRect(0,0,144,168), 0, GCornerNone);//layer_get_frame(layer));
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
	if(((tick_time->tm_min)+2)%5 == 0)
	{
		set_locations(tick_time);
		run_animations();
	}
}

void handle_init(void)
{
	bg_color = BG_COLOR_DEFAULT;
	fg_color = FG_COLOR_DEFAULT;
	
	window = window_create();
	window_set_background_color(window, bg_color);
	window_stack_push(window, true);
	root_layer = window_get_root_layer(window);

	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		light[i] = layer_create(GRect(0,0,0,0));
		layer_set_clips(light[i], true);
		layer_set_update_proc(light[i], light_draw);
		layer_add_child(root_layer, light[i]);
	}
	

	mask = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK_BLACK);
	mask_layer = bitmap_layer_create(layer_get_frame(root_layer));
	bitmap_layer_set_bitmap(mask_layer, mask);
	bitmap_layer_set_compositing_mode(mask_layer, GCompOpClear);
	layer_add_child(root_layer, bitmap_layer_get_layer(mask_layer));
	
	time_t now = time(NULL);
	struct tm *time = localtime(&now);
	srand(now);

	set_startup_locations(time);
	set_locations(time);
	run_animations();

	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);        
}

void handle_deinit(void) 
{
	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		layer_destroy(light[i]);
//		property_animation_destroy(light_animation[i]);
	}
	bitmap_layer_destroy(mask_layer);
	window_destroy(window);
}

int main(void) 
{
	handle_init();
	app_event_loop();
	handle_deinit();
}