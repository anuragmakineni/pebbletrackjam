#include <pebble.h>
#include <math.h> 

static Window *s_main_window;
static TextLayer *s_output_layer;
static TextLayer *s_calibrate_layer;
static int i = 0;
static double average = 0;
static int end = 20;
static int steps = -1;
static int threshold = 1200;
static int bpm = 0;
static int32_t timeCurrent;
static int32_t timePrevious;
static int32_t timeElapsed;
static bool buttonPressed = false;
static int countTime = 0;
static const int timeLimit = 5;
static int countAvg = 0;
static int averageValue = 0;
static int averagedPace = 0;
static bool downPressed = false;
static time_t secStart; 
static uint16_t msStart;
static double averageNew = 0;
static int32_t timePased;
static int averageInt;

double my_sqrt( double num )
{
  i++;
	double a, p, e = 0.001, b;

	a = num;
	p = a * a;
	while( p - num >= e )
	{
		b = ( a + ( num / a ) ) / 2;
		a = b;
		p = a * a;
	}
	return a;
}

static void sendSPM(int stepRate)
{
    Tuplet spm_tuple =  TupletInteger(0, stepRate); 
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter == NULL) {
       return;
    }
    dict_write_tuplet(iter, &spm_tuple);
    dict_write_end(iter);
    app_message_outbox_send();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  buttonPressed = true;
  time_ms(&secStart, &msStart);
  text_layer_set_text(s_calibrate_layer, "Averaging.");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  downPressed = !downPressed;
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}


static void data_handler(AccelData *data, uint32_t num_samples) {

  
  double x = data[0].x * 1.0;
  double y = data[0].y * 1.0;
  double z = data[0].z * 1.0;
  double magnitude = my_sqrt(x*x + y*y + z*z);
  int mag = magnitude;
  
//calibrate magnitude
  if (i == 1)
    {
    average = magnitude;
    steps = -1;
    text_layer_set_text(s_calibrate_layer, "Calib. Keep Still.");
  }
    
  if(i < end)
  {
    average =  average + magnitude;
    steps = -1;
    text_layer_set_text(s_calibrate_layer, "Calib. Keep Still.");
  }
  else if (i == end)
    {
    average = average / (i * 1.0);
    steps = 0;
    text_layer_set_text(s_calibrate_layer, "Ready.");
  }
   
int avg = average;

int delta = abs(avg - mag);
  

  
  if (delta > threshold)
  {
    steps++;
  
    
    if (steps > 1)
    {
      text_layer_set_text(s_calibrate_layer, "TrackJAM.");
      timePrevious = timeCurrent;  
    }
    
    time_t secCurrent; 
    uint16_t msCurrent;
    time_ms(&secCurrent, &msCurrent);
    
    timeCurrent = (int32_t)1000*(int32_t)secCurrent + (int32_t)msCurrent;
    
    if (steps <= 1)
    {
      timeElapsed = -1;
      bpm = -1;
    }
    else
    {
      timeElapsed = timeCurrent - timePrevious;
      double hz = (1000.0 / (timeElapsed * 1.0));
      
      
      bpm = hz * 60.0;
      
    }
    
    //averaging after button press
    if (buttonPressed == true)
    {
      countTime = countTime + 1;
      text_layer_set_text(s_calibrate_layer, "Averaging.");
      if (countTime >= timeLimit){
        
      time_t secNow; 
      uint16_t msNow;
      time_ms(&secNow, &msNow);
    
      timePased = ((int32_t)1000*(int32_t)secNow + (int32_t)msNow) - ((int32_t)1000*(int32_t)secStart + (int32_t)msStart);
        
      averageNew = (timeLimit / (timePased / 1000.0)) * 60;
      
      averageInt = averageNew;
      
      
        
        text_layer_set_text(s_calibrate_layer, "TrackJAM.");
        averageValue = averageValue / timeLimit;
        sendSPM(averageNew);
        averagedPace = averageNew;
        averageNew = 0;
        buttonPressed = false;
        countTime = 0;
        countAvg = 0;
      }
      else{
        text_layer_set_text(s_output_layer, "Averaging pace...");
        averageValue = averageValue + bpm;
      }
    }
      
  }


  
    // Long lived buffer
  static char s_buffer[128];
  
  

  // Compose string of all data
//   snprintf(s_buffer, sizeof(s_buffer), 
//     "N X,Y,Z\n0 %d,%d,%d\n1 %d,%d,%d\n2 %d\n3 %d,%d,%d,%d,%d",
//     data[0].x, data[0].y, data[0].z, mag, avg,threshold,steps,bpm,averageValue,buttonPressed,countTime,averagedPace
//   );
   snprintf(s_buffer, sizeof(s_buffer),
             "STEPS: %d\nSPM: %d\nAVG: %d",
            steps, bpm, averagedPace
            );
  
//     snprintf(s_buffer, sizeof(s_buffer), 
//     "N X,Y,Z\n0 %d,%d,%d,%d",
//     data[0].x, data[0].y, data[0].z, mag);
  
  //Show the data
  text_layer_set_text(s_output_layer, s_buffer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create output TextLayer
  s_output_layer = text_layer_create(GRect(2, 0, window_bounds.size.w - 4, window_bounds.size.h));
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
  ResHandle rh = resource_get_handle(RESOURCE_ID_FONT_MADRID_34);
  text_layer_set_font(s_output_layer, fonts_load_custom_font(rh));
  text_layer_set_text(s_output_layer, "No data yet.");
  text_layer_set_overflow_mode(s_output_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
  
  s_calibrate_layer = text_layer_create(GRect(2, window_bounds.size.h - 30, window_bounds.size.w - 4, 30));
  text_layer_set_text_alignment(s_calibrate_layer, GTextAlignmentCenter);
  ResHandle rh2 = resource_get_handle(RESOURCE_ID_FONT_MADRID_18);
  text_layer_set_font(s_calibrate_layer, fonts_load_custom_font(rh2));
  text_layer_set_overflow_mode(s_calibrate_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_calibrate_layer));
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_output_layer);
}





static void init() {
  // Create main Window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

    // Subscribe to the accelerometer data service
    int num_samples = 1;
    accel_data_service_subscribe(num_samples, data_handler);

    // Choose update rate
    accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  
    //initSendingService
    app_message_open(124, 256);
  
    window_set_click_config_provider(s_main_window, click_config_provider);
  
}

static void deinit() {
  // Destroy main Window
  window_destroy(s_main_window);
  accel_data_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}