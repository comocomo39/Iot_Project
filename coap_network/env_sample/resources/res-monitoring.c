#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "../cJSON-master/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../global_variable/global_variables.h"

#define SAMPLE_COUNT 10
#define TIME_SAMPLE 10  // Campionamento ogni 10 secondi

static int count = 0;
extern Sample samples[SAMPLE_COUNT]; // Array per memorizzare i campioni

static struct etimer monitoring_timer; // Timer per il campionamento

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

// Funzione per raccogliere nuovi campioni
static void update_samples() {
    if (count >= SAMPLE_COUNT) {
        count = 0; // Resetta il buffer se pieno
    }

    // Genera nuovi dati casuali per test (sostituire con lettura da sensori reali)
    samples[count].temperature = (random_rand() % 30) + 15;
    samples[count].humidity = (random_rand() % 100);
    samples[count].dust_density = (random_rand() % 500);

    printf("🟢 Nuovo campione [%d]: Temp=%.2f°C, Hum=%.2f%%, Dust=%d\n",
           count, samples[count].temperature, samples[count].humidity, samples[count].dust_density);

    count++;
}

// Handler per la richiesta GET
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {
    update_samples();

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
}

// Notifica gli osservatori ogni 10 secondi
static void res_event_handler(void) {
    coap_notify_observers(&res_monitoring_temp);
}

// Processo per il monitoraggio ambientale
PROCESS(monitoring_process, "Environmental Monitoring Process");
AUTOSTART_PROCESSES(&monitoring_process);

PROCESS_THREAD(monitoring_process, ev, data) {
    PROCESS_BEGIN();
    
    etimer_set(&monitoring_timer, CLOCK_SECOND * TIME_SAMPLE);
    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&monitoring_timer));

        update_samples();
        res_event_handler();
        etimer_reset(&monitoring_timer);
    }

    PROCESS_END();
}
