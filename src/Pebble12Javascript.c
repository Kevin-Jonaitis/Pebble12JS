#include <pebble.h>
#include "Pebble12Javascript.h"
static Window *window;
static TextLayer *text_layer;
const int JS_READY = 0;
const int ACCESS_TOKEN_KEY = 1;
const int REFRESH_TOKEN_KEY = 2;
const int NO_ACCESS_TOKEN = 3; // Tells the javascript file it failed to find a access token
const int VERIFICATION_CODE = 4;

void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered
 }

 void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
   // outgoing message failed
 }

void in_received_handler(DictionaryIterator *iter, void *context) {
	//Check for fields you expect to receive
        
	Tuple *js_ready  = dict_find(iter,JS_READY);
	//	int addOrRemove = (int) addOrRem->value->data[0];
	Tuple *code = dict_find(iter,VERIFICATION_CODE);
	if(js_ready) {
	  APP_LOG(APP_LOG_LEVEL_DEBUG,"JAVASCRIPT IS LOADED");
	  determine_token_status();
	}
	if(code) {
	  char *verification_code = code->value->cstring; //DO I NEED TO FREE THIS MEMORY LATER????
	  text_layer_set_text(text_layer, verification_code);
	  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	  layer_mark_dirty(text_layer_get_layer(text_layer));                                                                                          \
	}

}


 void in_dropped_handler(AppMessageResult reason, void *context) {
   // incoming message dropped
 }


void determine_token_status() {
	DictionaryIterator *iter;
	char access_key[64];
	app_message_outbox_begin(&iter);

	if(persist_exists(ACCESS_TOKEN_KEY)) {

                APP_LOG(APP_LOG_LEVEL_DEBUG, "We have an access token");
		persist_read_string(ACCESS_TOKEN_KEY,access_key,64);
		//Tuplet value = TupletCString(ACCESS_TOKEN_KEY,access_key);
		dict_write_cstring(iter,ACCESS_TOKEN_KEY,access_key); 		

		//ALSO PASS BACK THE REFRESH TOKEN.

                //check to see if this token still works. If it does, build window normally. Else, try the refresh_token_key

        } else {
                APP_LOG(APP_LOG_LEVEL_DEBUG, "No access token. Asking js for token...");
//              text_layer_set_text(text_layer,"Acquiring token...");
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
  text_layer = text_layer_create(bounds); //set the text layer to the whole screen
  text_layer = text_layer_create((GRect) { .origin = { 0,bounds.size.h/2 }, .size = { bounds.size.w, bounds.size.h } });
  text_layer_set_text(text_layer, "Loading...");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
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

   const uint32_t inbound_size = 64;
   const uint32_t outbound_size = 64;
   app_message_open(inbound_size, outbound_size);

   const bool animated = true;
   window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
