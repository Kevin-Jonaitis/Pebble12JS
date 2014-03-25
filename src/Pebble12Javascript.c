//#include "time.h"
//#include <pebble.h>
#include "pebble.h"
#include "Pebble12Javascript.h"
//#include <stddef.h>
//#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *top_layer;
const int JS_READY = 0;
//const int ACCESS_TOKEN_KEY = 1;
//const int REFRESH_TOKEN_KEY = 2;
const int NO_ACCESS_TOKEN = 3; // Tells the javascript file it failed to find a access token
const int VERIFICATION_CODE = 4;
const int GOT_TOKENS = 5;
const int ACCESS_TOKEN = 6;
const int REFRESH_TOKEN = 7;
const int TIMES = 8; //Free/busy times

const int INITAL_ARRAY_SIZE = 48; //We shouldn't have more than 48 events in a 24 hour period, becuase that's quite a bit.


//Utility method for splitting strings.
char *
strtok(s, delim)
	register char *s;
	register const char *delim;
{
	register char *spanp;
	register int c, sc;
	char *tok;
	static char *last;


	if (s == NULL && (s = last) == NULL)
		return (NULL);

	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		last = NULL;
		return (NULL);
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				last = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

typedef struct {
  long startTime;
  long endTime;
} Event;

typedef struct {
  Event *array;
  size_t used;
  size_t size;
  
} Array;

void initArray(Array *a) {
  a->array = (Event *)malloc(INITAL_ARRAY_SIZE * sizeof(Event));
  a->used = 0;
  a->size = INITAL_ARRAY_SIZE;
}

void insertArray(Array *a, Event element) {
  if (a->used == a->size) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "WE COMPLETELY FILLED UP THE ARRAY THIS IS A PROBLEM!"); 
  } else {
  a->array[a->used++] = element;
  }
}

void freeArray(Array *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}

Array events;


void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent a message!");

 }

 void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Out dropped: %i - %s", reason, translate_error(reason));
   // outgoing message failed
 }

void in_received_handler(DictionaryIterator *iter, void *context) {
  char *accessToken;
  char *refreshToken;
  //Check for fields you expect to receive
  Tuple *js_ready  = dict_find(iter,JS_READY);
  //	int addOrRemove = (int) addOrRem->value->data[0];
  Tuple *code = dict_find(iter,VERIFICATION_CODE);
  Tuple *gotTokens = dict_find(iter,GOT_TOKENS);
  Tuple *getAccessToken = dict_find(iter,ACCESS_TOKEN);
  Tuple *getRefreshToken = dict_find(iter,REFRESH_TOKEN);
  Tuple *times = dict_find(iter,TIMES);
  
  if(js_ready) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"JAVASCRIPT IS LOADED");
    determine_token_status();
  }
  if(code) {
    text_layer_set_text(top_layer, "Please open the configuration window and enter the following code when prompted:");
    char *verification_code = code->value->cstring; //DO I NEED TO FREE THIS MEMORY LATER????
    text_layer_set_text(text_layer, verification_code);
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    //layer_mark_dirty(text_layer_get_layer(text_layer));
    layer_mark_dirty(window_get_root_layer(window));
  }
  if(gotTokens) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"WE GOT THE TOKENS clearing Loading... screen");
    text_layer_set_text(text_layer,"Fetching calendar data...");
    layer_remove_from_parent(text_layer_get_layer(top_layer));
    layer_mark_dirty(window_get_root_layer(window));
    //    layer_mark_dirty(text_layer_get_layer(text_layer));                
  }
  
  if(getAccessToken) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"WRITING TOKENS TO STORAGE");
    APP_LOG(APP_LOG_LEVEL_DEBUG,getAccessToken->value->cstring);
    
    accessToken = getAccessToken->value->cstring;
    int return_value = persist_write_string(ACCESS_TOKEN,accessToken);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"PERSIST WRITE RETURN VALUE: %d",return_value);

    
  }
  
  if(getRefreshToken) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"GOT REFRESH TOKEN, IT IS BELOW");
    APP_LOG(APP_LOG_LEVEL_DEBUG,getRefreshToken->value->cstring);
    
    refreshToken = getRefreshToken->value->cstring;
    persist_write_string(REFRESH_TOKEN,refreshToken);
    
  }
  if(times) {
    freeArray(&events);
    initArray(&events);

    int length = times->length;
    for(int i = 0; i < length; i ++) {
      int tupleIndex = times->value->data[i];
      APP_LOG(APP_LOG_LEVEL_DEBUG,"STRING VALUE %d",(int) times->value->data[0]);
      Tuple *timeTuple = dict_find(iter,tupleIndex);
      char *timeString = timeTuple->value->cstring;
      APP_LOG(APP_LOG_LEVEL_DEBUG,"A timeString %s", timeString);

      char times_string[22];
      char *time_part;
      strcpy(times_string,timeString);
      time_part = strtok(times_string,".");
      APP_LOG(APP_LOG_LEVEL_DEBUG,"START TIME: %s",time_part);
      long startTime = atol(time_part);
      time_part = strtok(NULL,".");
      APP_LOG(APP_LOG_LEVEL_DEBUG,"END TIME: %s",time_part);
      long endTime = atol(time_part);
      Event e = {startTime,endTime};
      insertArray(&events,e);
      
    }
  
    time_t now;
    //    time_t t;
    time(&now);

    long seconds = (long) now;
    time_t test = (time_t) events.array[0].endTime;
    // struct tm tmlol = {0};
    //strptime("12344656","%s",&tmlol);
    //t = mktime(&tmlol);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"TEST TIME: %lu", (long) test);

    APP_LOG(APP_LOG_LEVEL_DEBUG,"NOW NOW TIME: %lu", seconds);
    long difference = (long) difftime(test,now);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"DIFFERENCE IN MINUTES FOR START TIME: %lu", difference);



    //  long test = (long) nownowTime;
    //    APP_LOG(APP_LOG_LEVEL_DEBUG,"NOW NOW TIME: %lu", test);

    

    
    

    /* //    freeArray(&events); // Clear all old events */
    /* //initArray(&events); // Create a new version of the array */
    /*  char times_string[256]; */
    /* layer_remove_from_parent(text_layer_get_layer(text_layer)); */
    /* //char * times_string = malloc(strlen(times->value->cstring)); */
    /* strcpy(times_string,times->value->cstring); */
    /* APP_LOG(APP_LOG_LEVEL_DEBUG,"HERE ARE THE TIMES OF %s",times_string); */

    /* char *time_part; */
    /* char * end_time; */
    /* time_part = strtok(times_string,"."); */
    /* //APP_LOG(APP_LOG_LEVEL_DEBUG,"TESTING: %s",time); */
    /* while(time_part != NULL) { */
    /*   //Times are stored in pairs of 2 */
    /*   APP_LOG(APP_LOG_LEVEL_DEBUG,"Start time: %s\n",time_part); */
    /*   long startTime = atol(time_part); */
    /*   time_part = strtok(NULL,"."); */
    /*   APP_LOG(APP_LOG_LEVEL_DEBUG,"END TIME: %s\n",time_part); */

    /*   if(time_part == NULL){ //We reached the last period */
    /* 	break; */
    /*   } */
      
    /*        time_part = strtok(NULL,"."); */
  
    /* 	   //strptime("1234","%s",&endTime); */
    /* 	   //long endTime = strtol(time,&end_time,10); */
      
    /*         time_t startTime_t = (time_t) startTime; */

    /*         time_t now; */
    /*         time(&now); */
    /*         APP_LOG(APP_LOG_LEVEL_DEBUG,"START: %lu\n",startTime); */
    /*         APP_LOG(APP_LOG_LEVEL_DEBUG,"NOW: %lu\n",now); */


    /*   //      double seconds = difftime(now,startTime_t); */
    /*   //      double minutes =  seconds/60; */
    /*   //      int min_int = (int) minutes; */

    /*   //      APP_LOG(APP_LOG_LEVEL_DEBUG,"%f minutes have passed since the start of the event",minutes); */
    /*   //      APP_LOG(APP_LOG_LEVEL_DEBUG,"%d minutes have passed since the start of the event",min_int); */

    /*   //  Event e = {startTime,endTime}; */
    /*   //insertArray(&events,e); */
    /* //  //      Event e = {timeStart,timeEnd); */
    /* // //insertArray(&events,e); */
    /* ////do something with the time */
    /*  } */
    /* //free(times_string); */
  }
}


void in_dropped_handler(AppMessageResult reason, void *context) {
   // incoming message dropped
 }


void determine_token_status() {
	DictionaryIterator *iter;
	char access_key[128];
	char refresh_key[128];
	app_message_outbox_begin(&iter);

	if(persist_exists(ACCESS_TOKEN)) {

                APP_LOG(APP_LOG_LEVEL_DEBUG, "We have an access token! Here it is below...");
		persist_read_string(ACCESS_TOKEN,access_key,128);
		dict_write_cstring(iter,ACCESS_TOKEN,access_key); 		

		//ALSO PASS BACK THE REFRESH TOKEN.
		persist_read_string(REFRESH_TOKEN,refresh_key,128);
		dict_write_cstring(iter,REFRESH_TOKEN,refresh_key);

		text_layer_set_text(text_layer,"Fetching calendar data...");
		layer_mark_dirty(text_layer_get_layer(text_layer));              

        } else {
                APP_LOG(APP_LOG_LEVEL_DEBUG, "No access token. Asking js for token...");
		Tuplet value = TupletInteger(NO_ACCESS_TOKEN,1);
                dict_write_tuplet(iter, &value);
		app_message_outbox_send();
		text_layer_set_text(text_layer,"Retrieving access token....");
		layer_mark_dirty(text_layer_get_layer(text_layer));                                                                                          
        }

	//Send the data
	app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent Token data to JS");

	//COULD CAUSE PROBLEMS IF TEXT LAYER IS NOT LOADED YET, POSSIBLE FIXES??
//        layer_mark_dirty(text_layer_get_layer(text_layer));

}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  //text_layer = text_layer_create(bounds); //set the text layer to the whole screen
  text_layer = text_layer_create((GRect) { .origin = { 0,bounds.size.h/2 }, .size = { bounds.size.w, bounds.size.h/2 } });
  text_layer_set_text(text_layer, "Loading...");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));


  //Top layer, won't display anything for now, but will display when we retrieve the token
  top_layer = text_layer_create((GRect) { .origin = { 0,0 }, .size = { bounds.size.w, bounds.size.h/2 } });
  text_layer_set_text_alignment(top_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(top_layer));

}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  //DEBUG ONLY, COMMENT OUT FINAL COMMIT
  //  persist_delete(ACCESS_TOKEN);
  //  persist_delete(REFRESH_TOKEN);

  window = window_create();
  window_set_fullscreen(window,true);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

   app_message_register_inbox_received(in_received_handler);
   app_message_register_inbox_dropped(in_dropped_handler);
   app_message_register_outbox_sent(out_sent_handler);
   app_message_register_outbox_failed(out_failed_handler);

   const uint32_t inbound_size = 512;
   const uint32_t outbound_size = 512;
   app_message_open(inbound_size, outbound_size);

   const bool animated = true;
   window_stack_push(window, animated);
}

static void deinit(void) {
  freeArray(&events); //Empty the array
  layer_destroy(text_layer_get_layer(top_layer)); //We are done with this, don't need it anymore                                                    
  layer_destroy(text_layer_get_layer(text_layer)); //We are done with this, don't need it anymore                                                    
  window_destroy(window);

}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}

//Utility method
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
