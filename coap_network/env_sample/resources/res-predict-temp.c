#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "machine_learning/temperature_prediction.h"
#include "global_variable/global_variables.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_SAMPLES 10
#define TIME_PREDICT 10

extern Sample samples[10];
static float temp_history[10];
static float hum_history[10];
static float dust_history[10];
static float next_temperature;

extern int sample_count;

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_predict_temp,
         "title=\"Temperature Prediction\";rt=\"Text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);

static void predict_temperature(void){
  if(sample_count < MAX_SAMPLES) return;

  for(int i = 0; i < MAX_SAMPLES; i++){
  temp_history[i] = samples[i].temperature;
  hum_history[i]  = samples[i].humidity;
  dust_history[i] = samples[i].dust_density;
}

  next_temperature = predict_next_temperature_from_values(
                         temp_history, hum_history, dust_history);
  printf("Predizione Temperatura: %.2f°C\n", next_temperature);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {
    predict_temperature();

    /* res-predict-temp.c */
    int length = snprintf((char *)buffer, preferred_size, "%.2f", next_temperature);

    coap_set_payload(response, (uint8_t *)buffer, length);
}

static void res_event_handler(void) {
    predict_temperature();
    coap_notify_observers(&res_predict_temp);
}
