#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "../cJSON-master/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../global_variable/global_variables.h"
#include "model/functionsML.h"

#define SAMPLE_COUNT 10
#define TIME_SAMPLE 10  // Campionamento ogni 10 secondi

static int count = 0;
extern AirSample air_samples[SAMPLE_COUNT]; // Array di campioni della qualità dell'aria

static struct etimer monitoring_timer; // Timer per il campionamento

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_monitoring_air,
         "title=\"Air Quality Monitoring\";rt=\"Text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);

// Funzione per raccogliere nuovi campioni
static void update_air_samples() {
    if (count >= SAMPLE_COUNT) {
        count = 0; // Resetta il buffer se pieno
    }

    // Genera nuovi dati casuali per test (sostituire con lettura da sensori reali)
    air_samples[count].co = random_rand() % 100;         // Simulazione CO
    air_samples[count].air_quality = random_rand() % 100; // Simulazione MQ135
    air_samples[count].tvoc = random_rand() % 500;      // Simulazione TVOC

    printf("🟢 Nuovo campione qualità aria [%d]: CO=%d, AirQuality=%d, TVOC=%d\n",
           count, air_samples[count].co, air_samples[count].air_quality, air_samples[count].tvoc);

    count++;
}

// Handler per la richiesta GET
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset) {
    update_air_samples();

    // Crea un JSON con gli ultimi dati raccolti
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();
    cJSON_AddItemToArray(data, cJSON_CreateNumber(air_samples[count - 1].co));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(air_samples[count - 1].air_quality));
    cJSON_AddItemToArray(data, cJSON_CreateNumber(air_samples[count - 1].tvoc));
    cJSON_AddItemToObject(root, "air_data", data);

    char *json = cJSON_Print(root);
    int length = snprintf((char *)buffer, preferred_size, "%s", json);
    coap_set_payload(response, (uint8_t *)buffer, length);
}

// Notifica gli osservatori ogni 10 secondi
static void res_event_handler(void) {
    coap_notify_observers(&res_monitoring_air);
}

// Processo per il monitoraggio della qualità dell'aria
PROCESS(monitoring_process, "Air Quality Monitoring Process");
AUTOSTART_PROCESSES(&monitoring_process);

PROCESS_THREAD(monitoring_process, ev, data) {
    PROCESS_BEGIN();
    
    etimer_set(&monitoring_timer, CLOCK_SECOND * TIME_SAMPLE);
    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&monitoring_timer));

        update_air_samples();
        res_event_handler();
        etimer_reset(&monitoring_timer);
    }

    PROCESS_END();
}
