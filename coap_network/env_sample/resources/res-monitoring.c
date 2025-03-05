#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "../cJSON-master/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "machine_learning/temperature_prediction.h"
#include "../global_variable/global_variables.h"

#define SAMPLE_COUNT 10
#define TIME_SAMPLE 10  // Campionamento ogni 10 secondi

static int count = 0;
extern Sample samples[SAMPLE_COUNT]; // Array per memorizzare i campioni
static float new_temperature;
static float new_humidity;
static float new_dust_density;

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_monitoring_temp,
         "title=\"Environmental Monitoring\";rt=\"Text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);



// Handler per la richiesta GET
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {
    
    if (count >= SAMPLE_COUNT) {
            count = 0; // Resetta il buffer se pieno
        }

    new_temperature = samples[count].temperature;
    new_humidity = samples[count].humidity;
    new_dust_density = samples[count].dust_density;
    // Crea un JSON con gli ultimi dati raccolti
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();
    cJSON_AddItemToArray(data, cJSON_CreateNumber(samples[count - 1].temperature));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(samples[count - 1].humidity));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(samples[count - 1].dust_density));
    cJSON_AddItemToObject(root, "env_data", data);

    char *json = cJSON_Print(root);
    int length = snprintf((char *)buffer, preferred_size, "%s", json);
    coap_set_payload(response, (uint8_t *)buffer, length);

    count++;
}

// Notifica gli osservatori ogni 10 secondi
static void res_event_handler(void) {
    coap_notify_observers(&res_monitoring_temp);
}

