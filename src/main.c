#include <pebble.h>
#include <lightbox.h>
#include <settings.h>
	
#define BG_COLOR GColorBlack
#define FG_COLOR GColorWhite
#define MAX_LIGHTS 4
#define MAX_STARTUP_SEQUENCES 6
#define CURVE AnimationCurveEaseInOut

typedef enum { 
	Simultaneous = 0,
	Sequential = 1, 
	Staggered = 2 
}Mode;
	
static Mode mode;

static Layer *light[MAX_LIGHTS];
static Window *window;
Layer *root_layer;
static BitmapLayer *mask_layer;
static GBitmap *mask;
static InverterLayer *inverter;

static PropertyAnimation *light_animation[MAX_LIGHTS];

static GRect location[MAX_LIGHTS];

static int my_round(float x)
{
        if((x-(int)x) >= 0.5){return (int)x + 1;}
        else{return (int)x;}
}

static void run_animations()
{
	int delay;
	int duration;
	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		switch(mode)
		{
			case Simultaneous:
				delay = 0;
				duration = animation_duration;
				break;
			case Sequential:
				delay = i * (animation_duration/4);
				duration = (animation_duration/4);
				break;
			case Staggered:
				delay = i * (animation_duration/8);
				duration = 5 * (animation_duration/8);
				break;
			default:
				delay = 0;
				duration = 0;
				break;
		}

		//set up animations. If the animation is already running then the target location will have been 
		//updated anyway and the animation will divert to the new location part-way through.
		//This allows tidy handling of a minute tick update during the startup animation.
		if(!animation_is_scheduled((Animation*)light_animation[i])) 
		{
			light_animation[i] = property_animation_create_layer_frame(light[i], NULL, &location[i]);
			animation_set_curve((Animation*)light_animation[i], CURVE);
			animation_set_delay((Animation*)light_animation[i], delay);
			animation_set_duration((Animation*)light_animation[i], duration);
			animation_schedule((Animation*)light_animation[i]);
		}
	}
}

static void set_next_locations(struct tm *tick_time)
{
	int hour = (tick_time->tm_hour)%12;
	int minute = my_round((tick_time->tm_min) / 5.0f) % 12;

	location[0] = ITS;

	switch(minute)
	{
		case 0:
			location[1] = O;
			location[2] = CLOCK;
			if(tick_time->tm_min > 30) //ticking over 2 mins early for fuzzy time
			{
				hour++;					//so need to increment the hour early too.
				hour = hour % 12;
			}
			break;
		case 1: location[1] = FIVE_M; break;
		case 2: location[1] = TEN_M; break;
		case 3: location[1] = QUARTER; break;
		case 4: location[1] = TWENTY; break;
		case 5: location[1] = TWENTYFIVE; break;
		case 6: location[1] = HALF; break;
		case 7: location[1] = TWENTYFIVE; break;
		case 8: location[1] = TWENTY; break;
		case 9: location[1] = QUARTER; break;
		case 10: location[1] = TEN_M; break;
		case 11: location[1] = FIVE_M; break;
	}
	
	if(minute > 0 && minute <= 6)
	{
		location[2] = PAST;
	}
	else if(minute > 6)
	{
		location[2] = TO;
		hour++;
		hour = hour % 12;
	}
	
	switch(hour)
	{
		case 1: location[3] = ONE; break;
		case 2: location[3] = TWO; break;
		case 3: location[3] = THREE; break;
		case 4: location[3] = FOUR;	break;
		case 5: location[3] = FIVE_H; break;
		case 6: location[3] = SIX; break;
		case 7: location[3] = SEVEN; break;
		case 8:	location[3] = EIGHT; break;
		case 9:	location[3] = NINE; break;
		case 10: location[3] = TEN_H; break;
		case 11: location[3] = ELEVEN; break;
		case 0: location[3] = TWELVE; break;
	}
}

static void light_draw(Layer *layer, GContext *ctx)
{
	graphics_context_set_fill_color(ctx, FG_COLOR);
	graphics_context_set_stroke_color(ctx, FG_COLOR);
	graphics_fill_rect(ctx, GRect(0,0,144,168), 0, GCornerNone);
}

static void init_layers()
{
	switch(rand() % MAX_STARTUP_SEQUENCES)
	{
		case 0:
			//Four corners
			light[0] = layer_create(GRect(-144,168,144,168));
			light[1] = layer_create(GRect(144,-168,144,168));
			light[2] = layer_create(GRect(-144,-168,144,168));
			light[3] = layer_create(GRect(144,168,144,168));
			mode = Sequential;
			break;
		case 1:
			//Center point
			light[0] = layer_create(GRect(72,84,0,0));
			light[1] = layer_create(GRect(73,84,0,0));
			light[2] = layer_create(GRect(73,85,0,0));
			light[3] = layer_create(GRect(72,85,0,0));
			mode = Staggered;
			break;
		case 2:
			//Full screen
			light[0] = layer_create(GRect(0,0,144,168));
			light[1] = layer_create(GRect(0,0,144,168));
			light[2] = layer_create(GRect(0,0,144,168));
			light[3] = layer_create(GRect(0,0,144,168));
			mode = Simultaneous;
			break;
		case 3:
			//Card deal
			light[0] = layer_create(GRect(65,168,14,16));
			light[1] = layer_create(GRect(65,168,14,16));
			light[2] = layer_create(GRect(65,168,14,16));
			light[3] = layer_create(GRect(65,168,14,16));
			mode = Sequential;
			break;
		case 4:
			//Light up
			for(int i = 0; i < MAX_LIGHTS; i++)
			{
				GPoint centre = grect_center_point(&location[i]);
				light[i] = layer_create(GRect(centre.x,centre.y,0,0));
			}
			mode = Sequential;
			break;
		case 5:
			//Drop in
			for(int i = 0; i < MAX_LIGHTS; i++)
			{
				GRect rect = location[i];
				rect.origin.y = rect.origin.y - 168;
				light[i] = layer_create(rect);
			}
			mode = Sequential;
			break;
	}
	
	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		layer_set_clips(light[i], true);
		layer_set_update_proc(light[i], light_draw);
		layer_add_child(root_layer, light[i]);
	}
	
	run_animations();
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
	set_next_locations(tick_time);
	if((tick_time->tm_min + 2) % 5 == 0) //display updates at 3, 8, 13, 18, 23, 28, 33, 38, 43, 48, 53, & 58
	{
		mode = Staggered;
		run_animations();
	}
	if(vibe && tick_time->tm_min == 0)
	{
		vibes_double_pulse();
	}
}

static void set_battery_locations(int charge_level)
{
	location[3] = BATTERY;
	location[2] = AT;
	switch(charge_level/10)
	{
		case 1: location[1] = TEN_H; break;
		case 2: location[1] = TWENTY; break;
		case 3: location[1] = THIRTY; break;
		case 4: location[1] = FORTY; break;
		case 5: location[1] = FIFTY; break;
		case 6: location[1] = SIXTY; break;
		case 7: location[1] = SEVENTY; break;
		case 8: location[1] = EIGHTY; break;
		case 9: location[1] = NINETY; break;
		case 10: location[1] = HUNDRED; break;
		default: location[1] = FIVE_H; break;
	}
	location[0] = PERCENT;
}

void handle_tap(AccelAxisType axis, int32_t direction) 
{
	GRect battery_rect = BATTERY;
	GRect light_rect = layer_get_frame(light[3]);
	//if currently displaying battery level, switch to time display
	if(grect_equal(&location[3], &battery_rect) && grect_equal(&light_rect, &battery_rect))
	{
		time_t now = time(NULL);
	    struct tm *time = localtime(&now);
		set_next_locations(time);
		mode = Simultaneous;
		run_animations();
	}
	else	//switch to battery display
	{
		BatteryChargeState charge_state;
		charge_state = battery_state_service_peek();
		set_battery_locations(charge_state.charge_percent);
		mode = Simultaneous;
		run_animations();
	}
}

static void update_settings()
{
	layer_set_hidden(inverter_layer_get_layer(inverter), !invert_colors);
}

void handle_init(void)
{
	init_settings();
	
	window = window_create();
	window_set_background_color(window, BG_COLOR);
	window_stack_push(window, true);
	root_layer = window_get_root_layer(window);

	time_t now = time(NULL);
    struct tm *time = localtime(&now);
	srand(now);
	
	set_next_locations(time);
	init_layers();
	
	mask = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK_BLACK);
	mask_layer = bitmap_layer_create(layer_get_frame(root_layer));
	bitmap_layer_set_bitmap(mask_layer, mask);
	bitmap_layer_set_compositing_mode(mask_layer, GCompOpClear);
	layer_add_child(root_layer, bitmap_layer_get_layer(mask_layer));

	inverter = inverter_layer_create(GRect(0,0,144,168));
	layer_add_child(root_layer, inverter_layer_get_layer(inverter));
	layer_set_hidden(inverter_layer_get_layer(inverter), !invert_colors); //hide layer to not invert colors
	
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);        
	accel_tap_service_subscribe(handle_tap);
}

void handle_deinit(void) 
{
	animation_unschedule_all();
	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		layer_destroy(light[i]);
		property_animation_destroy(light_animation[i]);
	}
	bitmap_layer_destroy(mask_layer);
	inverter_layer_destroy(inverter);
	window_destroy(window);
	save_settings();
}

int main(void) 
{
	handle_init();
	app_event_loop();
	handle_deinit();
}