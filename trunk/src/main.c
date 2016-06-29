#include <pebble.h>
#include <time.h>

#define SCREEN_WIDTH		144

#define WEATHER_TEMPERATURE 		0
#define WEATHER_CONDITIONS 			1
#define CONFIG_USE24				2
#define CONFIG_POSITION				3
#define CONFIG_VIBRATE				4
#define CONFIG_UPDATETIME			5
#define CONFIG_GRIDX				6
#define CONFIG_GRIDY				7

#define USE_POSITION				8	// c -- js
#define CONFIG_USEDUST				9
#define USE_USEDUST					10	// c -- js
#define DUST_CODE					11

bool bluetoothconnected = true;
bool use24h = false;				// 24 시간제
bool userposition = true;		// 위치 정보 사용
bool usevibrate = false;			// 진동사용
//bool usedust = false;			// 미세먼지 사용?
bool usedust = true;
int weather_update_time = 3;			// 날씨 업데이트 시간
int gridx = 0;
int gridy = 0;
int dustcode = -1;

Window* window;
BitmapLayer* background;
//GBitmap* background_bmp;
Layer* w_layer;
Layer* b_layer;

int resources[11] = 
{
    RESOURCE_ID_NUM_0,
    RESOURCE_ID_NUM_1, 
    RESOURCE_ID_NUM_2,
    RESOURCE_ID_NUM_3,
    RESOURCE_ID_NUM_4,
    RESOURCE_ID_NUM_5,
    RESOURCE_ID_NUM_6,
    RESOURCE_ID_NUM_7,
    RESOURCE_ID_NUM_8,
    RESOURCE_ID_NUM_9,  // 9
    RESOURCE_ID_COLON   // :
};

int resindex[17] = 
{
	// 작은 0 ~ 1 (날짜 표시용)
	RESOURCE_ID_S0,
	RESOURCE_ID_S1,
	RESOURCE_ID_S2,
	RESOURCE_ID_S3,
	RESOURCE_ID_S4,
	RESOURCE_ID_S5,
	RESOURCE_ID_S6,
	RESOURCE_ID_S7,
	RESOURCE_ID_S8,
	RESOURCE_ID_S9,
	// 일 ~ 토
	RESOURCE_ID_SUN,
	RESOURCE_ID_MON,   //  
	RESOURCE_ID_TUE,
	RESOURCE_ID_WED,
	RESOURCE_ID_THU,
	RESOURCE_ID_FRI,
	RESOURCE_ID_SAT
};


int resweather[5] = 
{
	RESOURCE_ID_SUNNY,
	RESOURCE_ID_CLOUDSUN,
	RESOURCE_ID_CLOUD,
	RESOURCE_ID_RAIN,
	RESOURCE_ID_SNOW
};

int dustindex[5] = 
{
	RESOURCE_ID_DUST_GOOD,
	RESOURCE_ID_DUST_NORMAL,
	RESOURCE_ID_DUST_LITTLEBAD,
	RESOURCE_ID_DUST_BAD,
	RESOURCE_ID_DUST_VERYBAD
};

void update_time(struct tm* t);
void updateday(struct tm* t);

//////////////////////////////////////////////////////////////////////////////

void custom_vibrate(int i)
{
	if( i == 0 )
	{
		vibes_short_pulse();    
	}
	else
	{
		static const uint32_t const segments[] = { 200, 100, 400, 100, 200, 100, 400 };
		
		VibePattern pat = {
		  .durations = segments,
		  .num_segments = ARRAY_LENGTH(segments),
		};
		vibes_enqueue_custom_pattern(pat);		
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// WEATHER


bool firstweatherupdate = false;
int weather_code = -1;				// 날씨 (-1은 아직 없다!)
int weather_temperature = -255;		// 온도 (-100은 아직 없다!)

typedef struct 
{
	BitmapLayer* layer;
	GBitmap* bmp;
}_SPR;


#define MAXTEMP_SPR     5
// icon + 숫자 + (dot)아이콘
_SPR	 temperature_spr[MAXTEMP_SPR];


char *translate_error(AppMessageResult result) {
  	switch (result) {
		case APP_MSG_OK: return "APP_MSG_OK";
		case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
		case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
		case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
		case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
		case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
		case APP_MSG_BUSY: return "APP_MSG_BUSY";
		case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
		case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
		case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
		case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
		case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
		case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
		case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
		default: return "UNKNOWN ERROR";
  }
}

int getspritewidth(int resourceid)
{
	GBitmap* bmp = gbitmap_create_with_resource(resourceid);
    GRect bitmap_bounds = gbitmap_get_bounds(bmp);
    int width = bitmap_bounds.size.w;	
    gbitmap_destroy(bmp);
    return width;
}

int rendersprite(int x, int y, _SPR *spr, int resourceid)
{
    GBitmap* bmp = gbitmap_create_with_resource(resourceid);

    GRect frame;
    frame.origin.x = x;
    frame.origin.y = y;
    GRect bitmap_bounds = gbitmap_get_bounds(bmp);
    frame.size = bitmap_bounds.size;
    BitmapLayer* layer = bitmap_layer_create(frame);

    bitmap_layer_set_bitmap(layer, bmp);
    layer_add_child(w_layer, bitmap_layer_get_layer(layer));

    bitmap_layer_set_compositing_mode(layer, GCompOpSet);
    layer_add_child(w_layer, bitmap_layer_get_layer(layer));

    spr->bmp = bmp; 
    spr->layer = layer; 

    return frame.size.w;
}

void releasesprite(_SPR *spr, int n)
{
   for(int i=0; i<n; i++)
   {
        if(spr[i].bmp != NULL)  
        {
            gbitmap_destroy(spr[i].bmp);
            layer_remove_from_parent(bitmap_layer_get_layer(spr[i].layer));
            bitmap_layer_destroy(spr[i].layer);
            spr[i].bmp = NULL;
        }            
    }    
}

// 날씨 정보 표시
void displayweather()
{
	static int displaycode = -1;
	static int displaytemperature = -255;

	if( weather_code == displaycode && weather_temperature == displaytemperature )
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Skip displayweather");
		return;
	}

    releasesprite(temperature_spr, MAXTEMP_SPR);
    //releaseweathersprite();

	// 요일 정보가 있으면 렌더
	if( weather_code != -1 || weather_temperature != -255 )
	{
		int index = 0;
		int ypos = 45;
		// 날씨 ICON			

		// 0 : 맑음 , 1 : 일부 흐림, 2 : 흐림 , 3 ; 비 , 4 : 눈
		//weather_code = 3;
		if( weather_code != -1 )
			rendersprite(12, ypos-5, &temperature_spr[index++], resweather[weather_code]);

		if( weather_temperature != -255 )
		{
			//weather_temperature = -32;				// test
			int xpos = 80;
			// 온도 (100도는 안넘겠지??)
			bool minus = weather_temperature >= 0 ? false : true;

			int weather10 = abs(weather_temperature) / 10;
			int weather1 = abs(weather_temperature) % 10;
			
			if(minus == true) 
				xpos += rendersprite(xpos, ypos, &temperature_spr[index++], RESOURCE_ID_MINUS);
			
			if( weather10 > 0 ) 
				xpos += rendersprite(xpos, ypos, &temperature_spr[index++], resindex[weather10]);
			
			xpos += rendersprite(xpos, ypos, &temperature_spr[index++], resindex[weather1]);
			xpos += rendersprite(xpos, ypos, &temperature_spr[index++], RESOURCE_ID_DOT);			
		}

		displaycode = weather_code;
		displaytemperature = weather_temperature;
	}
}

// 폰에서 날씨 정보 얻어옴
void getweather() 
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Getting new weather info.");

    // Begin dictionary
    DictionaryIterator *iter;
    
    AppMessageResult res = app_message_outbox_begin(&iter);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgBegin: %s", translate_error(res));

    if (iter == NULL) 
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "NULL iter.");
        dict_write_end(iter);
        return;
    }

    // Add a key-value pair
    dict_write_uint8(iter, USE_POSITION, userposition ? 1 : 0);
	dict_write_uint8(iter, CONFIG_GRIDX, gridx);
	dict_write_uint8(iter, CONFIG_GRIDY, gridy);
	dict_write_uint8(iter, USE_USEDUST, usedust ? 1 : 0);

/*
 * 자바스크립쪽은 이렇게 받
[03:06:01] javascript> AppMessage received in JS! - Getting weather...
[03:06:01] javascript> --> {"0":0,"1":10,"WEATHER_TEMPERATURE":0,"CONDITIONS":10} 
*/    
    
    dict_write_end(iter);

    // Send the message!
    res = app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgSend: %s", translate_error(res));
}


// 응답 도착
void inbox_received_callback(DictionaryIterator *iterator, void *context) 
{
	bool forceupdate = false;
	int prev_weather_code = weather_code;
	
    Tuple *t = dict_read_first(iterator);
    while(t != NULL) 
    {
        switch(t->key) 
        {
			// 온도 (도)
            case WEATHER_TEMPERATURE:
			{
				weather_temperature = (int)t->value->int32;
				APP_LOG(APP_LOG_LEVEL_DEBUG, "PEBBLE WEATHER_TEMPERATURE : %d", weather_temperature);
				
				firstweatherupdate = true;
				persist_write_int(WEATHER_TEMPERATURE, weather_temperature);				
				//persist_write_bool(WEATHER_FIRSTUPDATE, firstweatherupdate);
			}
            break;

			// 날씨 코드
            case WEATHER_CONDITIONS:
			{
				// 0 : 맑음 , 1 : 일부 흐림, 2 : 흐림 , 3 ; 비 , 4 : 눈
				weather_code = (int)t->value->int32;
				APP_LOG(APP_LOG_LEVEL_DEBUG, "PEBBLE WEATHER_Donditions : %d", weather_code);
				
				firstweatherupdate = true;				
				persist_write_int(WEATHER_CONDITIONS, weather_code);
				//persist_write_bool(WEATHER_FIRSTUPDATE, firstweatherupdate);
			}
            break;
            
            case DUST_CODE :
            {
				dustcode = (int)t->value->int32;
				APP_LOG(APP_LOG_LEVEL_DEBUG, "PEBBLE DUST_CODE : %d", dustcode);
				persist_write_int(DUST_CODE, dustcode);								
			}
            break;
            
            // web 설정
            // 24시간제
            case CONFIG_USE24 :
            {
				APP_LOG(APP_LOG_LEVEL_ERROR, "USE 24h");
				int _use24 = (int)t->value->int32;
				use24h = _use24 == 0 ? false : true;
				persist_write_bool(CONFIG_USE24, use24h);
			}
            break;
            
            // 위치 정보 사용
    		case CONFIG_POSITION :
				APP_LOG(APP_LOG_LEVEL_ERROR, "CONFIG_POSITION");
				int _userposition = (int)t->value->int32;
				userposition = _userposition == 0 ? false : true;				
				persist_write_bool(CONFIG_POSITION, userposition);
			break;

			// 진동사용
			case CONFIG_VIBRATE :
				APP_LOG(APP_LOG_LEVEL_ERROR, "CONFIG_VIBRATE");
				int _usevibrate = (int)t->value->int32;
				usevibrate = _usevibrate == 0 ? false : true;
				persist_write_bool(CONFIG_VIBRATE, usevibrate);
			break;
			
			// 날씨 업데이트 주기
			case CONFIG_UPDATETIME :
				APP_LOG(APP_LOG_LEVEL_ERROR, "CONFIG_UPDATETIME");
				weather_update_time = (int)t->value->int32;
				persist_write_int(CONFIG_UPDATETIME, weather_update_time);
            break;
			
			case CONFIG_GRIDX :
				APP_LOG(APP_LOG_LEVEL_ERROR, "CONFIG_GRIDX");
				gridx = (int)t->value->int32;
				persist_write_int(CONFIG_GRIDX, gridx);
            break;
			
			case CONFIG_GRIDY :
				APP_LOG(APP_LOG_LEVEL_ERROR, "CONFIG_GRIDY");
				gridy = (int)t->value->int32;
				persist_write_int(CONFIG_GRIDY, gridy);
            break;
            
			// 미세먼지 정보 사용
    		case CONFIG_USEDUST :
				APP_LOG(APP_LOG_LEVEL_ERROR, "CONFIG_USEDUST");
				int _usedust = (int)t->value->int32;
				usedust = _usedust == 0 ? false : true;				
				persist_write_bool(CONFIG_USEDUST, usedust);
				forceupdate = true;				
			break;
            
            default:
				APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
            break;
        }
		t = dict_read_next(iterator);
    }
    
    if( forceupdate )
		firstweatherupdate = false;

	// 지역정보 없으면 위치 기반으로 작동
	if( gridx == 0 || gridy == 0 )
		userposition = true;
    
    // 날씨 렌더
    displayweather();

    time_t now = time(NULL);
    struct tm* _time = localtime(&now);
    update_time(_time);
	updateday(_time);
	
    if(usevibrate)
    {
		if( prev_weather_code == -1 )
		{
			custom_vibrate(0);	
		}
		else
		{
			if( ( (prev_weather_code == 0 || prev_weather_code == 1 ||prev_weather_code == 2) && (weather_code ==3||weather_code ==4) ) ||
			((prev_weather_code == 3 || prev_weather_code == 4) && (weather_code == 0 || weather_code == 1 || weather_code == 2)) )
			custom_vibrate(1);
				else
			custom_vibrate(0);	
		}
	}
}

void inbox_dropped_callback(AppMessageResult reason, void *context) 
{
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!: %s", translate_error(reason));
}

void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) 
{
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!: %s", translate_error(reason));
}

void outbox_sent_callback(DictionaryIterator *iterator, void *context) 
{
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void initweather()
{
    APP_LOG(APP_LOG_LEVEL_INFO, "initweather !!");

	for(int i=0; i<4; i++)
	{
		temperature_spr[i].layer = NULL;
		temperature_spr[i].bmp = NULL;
	}
	
    firstweatherupdate = false;
	weather_code = -1;
	weather_temperature = -255;
}

void updateweather(struct tm* t)
{
	displayweather();
	
//	for(int i=0; i<24; i++)
//	APP_LOG(APP_LOG_LEVEL_INFO, "updateweather : %d", i % weather_update_time);

//    if( t->tm_min == 0 && (t->tm_hour == 0 || t->tm_hour == 3 || t->tm_hour == 6 || t->tm_hour == 9 || 
//		t->tm_hour == 12 || t->tm_hour == 15 || t->tm_hour == 18 || t->tm_hour == 21 ) )

	if( t->tm_min == 0 ) 
    {
		if( t->tm_hour % weather_update_time == 0 )
			getweather();
    }
    
    if( firstweatherupdate == false )
    {
		// 최초 1회 업데이트
		APP_LOG(APP_LOG_LEVEL_DEBUG, "first update");
		getweather();
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_DATE    7
_SPR    date_spr[MAX_DATE];

void initdate()
{
    for(int i=0; i<MAX_DATE; i++)
    {
        date_spr[i].layer = NULL;
        date_spr[i].bmp = NULL;
    }
}

void updateday(struct tm* t)
{

#if 0
struct tm {
   int tm_sec;         /* seconds,  range 0 to 59          */
   int tm_min;         /* minutes, range 0 to 59           */
   int tm_hour;        /* hours, range 0 to 23             */
   int tm_mday;        /* day of the month, range 1 to 31  */
   int tm_mon;         /* month, range 0 to 11             */
   int tm_year;        /* The number of years since 1900   */
   int tm_wday;        /* day of the week, range 0 to 6    */
   int tm_yday;        /* day in the year, range 0 to 365  */
   int tm_isdst;       /* daylight saving time             */   
};
#endif

    releasesprite(date_spr, MAX_DATE);

    int month = t->tm_mon+1;
    int day = t->tm_mday;
    int weekday = t->tm_wday;
    APP_LOG(APP_LOG_LEVEL_INFO, "update date %d : %d : %d", month, day, weekday);

	int countindex = 0;
	int gap = 2;	
	
	if( usedust )
	{
		int xpos = 7;
		int ypos = 8;
		
		int spritewidth = 0;
		if(month < 10)
		{
			spritewidth += getspritewidth(resindex[month]) + gap;
		}
		else
		{
			int month2 = month % 10;
			spritewidth += getspritewidth(resindex[1]) + gap;
			spritewidth += getspritewidth(resindex[month2]) + gap;
		}
		
		spritewidth += getspritewidth(RESOURCE_ID_SLASH) + gap;

		if( day >= 10 )
		{
			int d1 = day / 10;
			int d2 = day % 10;
			spritewidth += getspritewidth(resindex[d1]) + gap;	//	일 1
			spritewidth += getspritewidth(resindex[d2]) + gap;	//	일 2
		}
		else
		{
			//spritewidth += getspritewidth(resindex[0]) + gap;	//	0
			spritewidth += getspritewidth(resindex[day]) + gap;	//	일
		}
		
		spritewidth += 8;
		spritewidth += getspritewidth(resindex[weekday+10]);	// 요일		

		if(dustcode > -1)
		{
			spritewidth += 8;
			spritewidth += getspritewidth(dustindex[dustcode]);
		}
		
		xpos = (SCREEN_WIDTH - spritewidth) / 2; 
		
		/////////////////////////////////////////////////////////////////////////////
		
		// Month 첫자리
		// Month 두번째 자리
		if(month < 10)
		{
			//xpos = 12;
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[month]) + gap;
		}
		else
		{
			int month2 = month % 10;
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[1]) + gap;
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[month2]) + gap;
		}
		
		// 슬래시
		xpos += rendersprite(xpos, ypos, &date_spr[countindex++], RESOURCE_ID_SLASH) + gap;
		
		//  일 1
		//  일 2
		if( day >= 10 )
		{
			int d1 = day / 10;
			int d2 = day % 10;
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[d1]) + gap;	//	일 1
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[d2]) + gap;	//	일 2
		}
		else
		{
			//xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[0]) + gap;	//	0
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[day]) + gap;	//	일
		}

		// 요일
		//xpos = 86;
		xpos += 8;
		xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[weekday+10]);	// 요일		
		
		if(dustcode > -1)
		{
			// 먼지
			//xpos = 110;
			xpos += 8;
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], dustindex[dustcode]);
		}
	}
	else
	{
		int xpos = 20;
		int ypos = 8;

		//month = 12;  // for test
		//day = 28;  // for test
		
		if( month >= 10 ) xpos -= 6;
		if( day >= 10 ) xpos -= 10;    

		// Month
		if(month < 10)
		{
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[month]) + gap;
		}
		else
		{
			int month2 = month % 10;
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[1]) + gap;
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[month2]) + gap;
		}

		xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[11]) + gap;
		xpos += 2;
		
		if( day >= 10 )
		{
			int d1 = day / 10;
			int d2 = day % 10;
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[d1]) + gap;	//	일 1
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[d2]) + gap;	//	일 2
		}
		else
			xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[day]) + gap;	//	일

		xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[10]) + 14;	// '일'
		xpos += rendersprite(xpos, ypos, &date_spr[countindex++], resindex[weekday+10]);	// 요일		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BATTERY_POS_Y	83
#define MAX_BATTERY 10
#define BATTERY_X	14
#define BATTERY_Y	5

bool ischarging = false;
BatteryChargeState current_battery;

//_SPR    battery_spr[MAX_BATTERY];
GColor colortable[MAX_BATTERY];

void initbattery()
{
/*	
    for(int i=0; i<MAX_BATTERY; i++)
    {
        battery_spr[i].layer = NULL;
        battery_spr[i].bmp = NULL;
    }
*/  
	colortable[0] = GColorFromRGB(170, 0, 0);
	colortable[1] = GColorFromRGB(170, 0, 85);
	colortable[2] = GColorFromRGB(170, 0, 170);	
	colortable[3] = GColorFromRGB(170, 0, 255);	
	colortable[4] = GColorFromRGB(85, 85, 170);	
	colortable[5] = GColorFromRGB(85, 85, 255);	
	colortable[6] = GColorFromRGB(0, 85, 255);
	colortable[7] = GColorFromRGB(85, 170, 255);
	colortable[8] = GColorFromRGB(0, 170, 255);
	colortable[9] = GColorFromRGB(0, 255, 255);
}

void updatebatteryrect(Layer *layer, GContext *ctx)
{
	int n_segments = (current_battery.charge_percent)/10;
	//if( ischarging ) n_segments = 10;
	int x = 2;
	for(int j=0; j<n_segments; j++)
	{
		int xpos = x + (j * BATTERY_X);
		// 테두리
		GRect crect0 = GRect(xpos , 0, BATTERY_X-2, BATTERY_Y);
		graphics_context_set_fill_color( ctx, GColorFromRGB(150, 150, 150) );
		graphics_fill_rect(ctx, crect0, 0, GCornersAll); 

		// 안쪽
		//GColor color = ischarging == true ? colortable[9] : colortable[j];
		GColor color = ischarging == true ? colortable[9] : colortable[7];
		GRect crect = GRect(xpos+1 , 0+1, BATTERY_X-2-2, BATTERY_Y-2);
		graphics_context_set_fill_color( ctx, color );
		graphics_fill_rect(ctx, crect, 0, GCornersAll); 
	}		
}

void updatebattery()
{
/*	
    releasesprite(battery_spr, MAX_BATTERY);

   int n_segments = (current_battery.charge_percent+4)/10;
    if( ischarging )
        n_segments = 10;

   int x = 2;
   for(int j=0; j<n_segments; j++)
   {
        int resourceid = RESOURCE_ID_BATTERY;
        if( ischarging )
            resourceid = RESOURCE_ID_CHARGE;
        else
            resourceid = RESOURCE_ID_BATTERY;

        rendersprite(x + (j*14), BATTERY_POS_Y, &battery_spr[j], resourceid);
    }
*/
}

void battery_handler(BatteryChargeState battery_charge_state) 
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "battery charge state changed to %d", battery_charge_state.charge_percent);
    current_battery = battery_charge_state;

    ischarging = false;
    if (current_battery.is_plugged) 
    {
        if (current_battery.is_charging) 
        {
            ischarging = true;
        }
    }

    //updatebattery();
    layer_mark_dirty(b_layer);
}


//////////////////////////////////////////////////////////////////////	TIME

#define TIMESPR_NUM		6
_SPR	time_spr[TIMESPR_NUM];	// 1 2 : 1 2 pm (6개)

void inittime()
{
	for(int i=0; i<TIMESPR_NUM; i++)
	{
		time_spr[i].bmp = NULL;
		time_spr[i].layer = NULL;
	}
}

int xpos[TIMESPR_NUM] = { -2, 31, 77, 110, 66 };

// 시간표시
void update_time(struct tm* t) 
{
    APP_LOG(APP_LOG_LEVEL_INFO, "update time %d:%d %d", t->tm_hour, t->tm_min, t->tm_sec);

    releasesprite(time_spr, TIMESPR_NUM);

	//int ypos = 88;
	int ypos = 95;
	// use 24h
	unsigned short hour = 0;
	if( use24h == true )
	{
		hour = t->tm_hour;
	}
	else
	{
		hour = (t->tm_hour > 12 ? t->tm_hour - 12 : t->tm_hour);
		if(hour == 0) hour = 12;
	}

	if( !(use24h == false && hour < 10 ))
		rendersprite(xpos[0], ypos, &time_spr[0], resources[hour/10]);	// 시 앞자리
	rendersprite(xpos[1], ypos, &time_spr[1], resources[hour % 10]);		// 시 뒷자리
	rendersprite(xpos[2], ypos, &time_spr[2], resources[t->tm_min / 10]);	// 분 앞자리
	rendersprite(xpos[3], ypos, &time_spr[3], resources[t->tm_min % 10]);	// 분 뒷자리
	rendersprite(xpos[4], ypos, &time_spr[4], resources[10]);				// :

	if( use24h == false && t->tm_hour >= 12 )
		rendersprite(1, 92, &time_spr[5], RESOURCE_ID_PM);			// PM

/*		
	bool bluetooth = connection_service_peek_pebblekit_connection();	// BlueTooth connection
	bool app = connection_service_peek_pebble_app_connection();			// connect to Pebble App
	bluetoothconnected = (bluetooth == true && app == true) ? true : false;
	updatebluetooth();		
	*/
}

// 분단위의 업데이트
void callback_minute_tick(struct tm* t, TimeUnits delta_t) 
{
    update_time(t);
    updateday(t);
    updatebattery();
    updateweather(t);
}

void initbackground()
{
#if 0	
	// back ground bitmap layer 생성
    background = bitmap_layer_create(layer_get_frame(w_layer));
	// png pack ground 로딩
    background_bmp = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);

	// background layer에 로딩한 이미지 붙임
    bitmap_layer_set_bitmap(background, background_bmp);
	// main window의 자식으로 추가
    layer_add_child(w_layer, bitmap_layer_get_layer(background));	
    
#else

#endif    
}

void draw_layer(Layer *layer, GContext *ctx)
{
	graphics_context_set_fill_color(ctx, GColorFromRGB(0, 0, 170));
	GRect rect_bounds = GRect(0, 0, 144, 168);

	int corner_radius = 0;
	graphics_fill_rect(ctx, rect_bounds, corner_radius, GCornersAll);
	
	GRect crect = GRect(48, 0, 48, 168);
	graphics_context_set_fill_color(ctx, GColorFromRGB(255, 0, 0));
	graphics_fill_rect(ctx, crect, 0, GCornersAll);
	
	// battery
	//updatebatteryrect(ctx);
}

////////////////////////////////////////////////////////////////////////////////// BlueTooth

// Blue Tooth Connection
_SPR	 bluetooth_spr[1];

void initbluetooth()
{
	bluetooth_spr[0].bmp = NULL;
	bluetooth_spr[0].layer = NULL;
}

void updatebluetooth()
{
    APP_LOG(APP_LOG_LEVEL_INFO, "update bluetooth");
    releasesprite(bluetooth_spr, 1);
    
    if(bluetoothconnected == false)
		rendersprite(20, 92, &bluetooth_spr[0], RESOURCE_ID_BLUETOOTH);
}

void bluetooth_handler(bool enable)
{
	//bool bluetooth = connection_service_peek_pebblekit_connection();	// BlueTooth connection
	//bool app = connection_service_peek_pebble_app_connection();			// connect to Pebble App
	//bluetoothconnected = (bluetooth == true && app == true) ? true : false;
	bluetoothconnected = enable;
	updatebluetooth();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void init_watch(void) 
{
    window = window_create();
    window_stack_push(window, true);
    w_layer = window_get_root_layer(window);
	
	GRect bounds = GRect(0, BATTERY_POS_Y, 144, BATTERY_POS_Y + BATTERY_Y);
	b_layer = layer_create(bounds);
	layer_add_child(w_layer, b_layer);
	
    layer_set_update_proc(w_layer, draw_layer);
    layer_set_update_proc(b_layer, updatebatteryrect);
    
	initbluetooth();
	initbackground();
    inittime();
    initweather();    
    initbattery();
    initdate();

	// BlueTooth
    connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_handler
	});

	// 분단위 update callback등록 (callback_minute_tick 함수 call)
    tick_timer_service_subscribe(MINUTE_UNIT, callback_minute_tick);

    //battery_handler(battery_state_service_peek());
    battery_state_service_subscribe(battery_handler);

    // App message Register callbacks
    app_message_register_inbox_received(inbox_received_callback);   // App 메시지 도착
    app_message_register_inbox_dropped(inbox_dropped_callback);     // App 메시지 누락
    app_message_register_outbox_failed(outbox_failed_callback);     // App 메시지 실패
    app_message_register_outbox_sent(outbox_sent_callback);         // watch --> phone send 보냄
    
    // Open AppMessage
    //AppMessageResult res = app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
    AppMessageResult res = app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMsgOpen: %s", translate_error(res));
    
    updatebluetooth();
    
	// 시작시 현재시간 표시
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    update_time(t);
    updateday(t);
    
	battery_handler(battery_state_service_peek());
    updatebattery();

	if (persist_exists(WEATHER_CONDITIONS))
		weather_code = persist_read_int(WEATHER_CONDITIONS);
		
	if (persist_exists(WEATHER_TEMPERATURE))
		weather_temperature = persist_read_int(WEATHER_TEMPERATURE);
		
	if( weather_code != -1 && weather_temperature != -255 )
		firstweatherupdate = true;

	if (persist_exists(CONFIG_USE24))
		use24h = persist_read_bool(CONFIG_USE24);

	if (persist_exists(CONFIG_POSITION))
		userposition = persist_read_bool(CONFIG_POSITION);
		
	if (persist_exists(CONFIG_VIBRATE))
		usevibrate = persist_read_bool(CONFIG_VIBRATE);
		
	if (persist_exists(CONFIG_UPDATETIME))
		weather_update_time = persist_read_int(CONFIG_UPDATETIME);

	if (persist_exists(CONFIG_GRIDX))
		gridx = persist_read_int(CONFIG_GRIDX);

	if (persist_exists(CONFIG_GRIDY))
		gridy = persist_read_int(CONFIG_GRIDY);

	if (persist_exists(CONFIG_USEDUST))
		usedust = persist_read_bool(CONFIG_USEDUST);

	if (persist_exists(DUST_CODE))
		dustcode = persist_read_int(DUST_CODE);

//	if (persist_exists(WEATHER_FIRSTUPDATE))
//		firstweatherupdate = persist_read_bool(WEATHER_FIRSTUPDATE);

    updateweather(t);
}

void deinit_watch(void) 
{
    window_destroy(window);
    bitmap_layer_destroy(background);
    //gbitmap_destroy(background_bmp);

    // Todo. unload all sprite
    releasesprite(time_spr, TIMESPR_NUM);
    releasesprite(temperature_spr, MAXTEMP_SPR);
    //releasesprite(battery_spr, MAX_BATTERY);
    releasesprite(date_spr, MAX_DATE);
	releasesprite(bluetooth_spr, 1);
    
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
}

int main(void) 
{
    init_watch();
    app_event_loop();
    deinit_watch();
}
