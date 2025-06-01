#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "../cJSON-master/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../global_variable/global_variables.h"
#include "machine_learning/functionsML.h"

/* Variabili locali */
static int new_pred = 0;
static float new_co = 0.0;
static float new_air_quality = 0.0;
static float new_tvoc = 0.0;
static int sample_id = 0;
static char last_payload[128]; // buffer da riusare per GET e notify

extern AirSample air_sample;

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

/* Risorsa osservabile */
EVENT_RESOURCE(res_monitoring_air,
         "title=\"Air Monitoring\";rt=\"application/json\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);

/* Handler GET (usa ultimo JSON salvato) */
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                             uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  coap_set_header_content_format(response, APPLICATION_JSON);
  coap_set_payload(response, last_payload, strlen(last_payload));
}

/* Notifica automatica */
static void res_event_handler(void) {
  printf("Sending notification (monitoring)\n");

  /* Leggi i dati correnti */
  new_co = air_sample.co;
  new_air_quality = air_sample.air_quality;
  new_tvoc = air_sample.tvoc;

  /* Predizione */
  float features[3] = {new_co, new_air_quality, new_tvoc};
  new_pred = predict_class(features, 3);
  if (new_pred < 0) new_pred = -new_pred;

  /* Costruzione JSON */
  cJSON *root = cJSON_CreateObject();
  cJSON *ss_array = cJSON_CreateArray();
  cJSON_AddItemToArray(ss_array, cJSON_CreateNumber(new_co));
  cJSON_AddItemToArray(ss_array, cJSON_CreateNumber(new_air_quality));
  cJSON_AddItemToArray(ss_array, cJSON_CreateNumber(new_tvoc));
  cJSON_AddItemToArray(ss_array, cJSON_CreateNumber(new_pred));
  cJSON_AddItemToObject(root, "ss", ss_array);
  cJSON_AddNumberToObject(root, "t", sample_id++);

  char *json_payload = cJSON_PrintUnformatted(root);
  if (json_payload != NULL) {
    strncpy(last_payload, json_payload, sizeof(last_payload) - 1);
    last_payload[sizeof(last_payload) - 1] = '\0';
    free(json_payload);
  } else {
    snprintf(last_payload, sizeof(last_payload), "{\"ss\":[0,0,0,0],\"id\":%d}", sample_id++);
  }

  cJSON_Delete(root);

  coap_notify_observers(&res_monitoring_air);
}
