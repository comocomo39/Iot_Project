#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "coap-engine.h"
#include "random.h"
#include "net/ipv6/uip-ds6.h"
#include "leds.h"
#include "coap-blocking-api.h"
#include "global_variable/global_variables.h"
#include "../cJSON-master/cJSON.h"
#include "os/dev/button-hal.h"


#define TIME_SAMPLE 10  // Campionamento ogni 10 secondi
#define SERVER_EP "coap://[fd00::1]:5683"
#define GOOD_ACK 65
#define MAX_REGISTRATION_RETRY 3

static int registration_retry_count = 0;
static int registered = 0;
static int shutdown = 0;

extern coap_resource_t res_danger;
extern coap_resource_t res_monitoring_air;
extern coap_resource_t res_shutdown;
extern coap_resource_t res_dangerCount;

AirSample air_sample;

PROCESS(air_quality_sensor_process, "Air Quality Sensor Process");
AUTOSTART_PROCESSES(&air_quality_sensor_process);

static struct etimer monitoring_timer;

// Funzione per raccogliere un nuovo campione di dati
void write_sample(void) {
    uint16_t bucket = random_rand() % 100;   // 0-99
    if(bucket < 40) {                // SAFE
        air_sample.co          = 50  + (random_rand() % 100);   // 50-149
        air_sample.tvoc        = 5   + (random_rand() % 24);    // 5-28
        air_sample.air_quality = 25  + (random_rand() % 10);    // 25-34
    } else if(bucket < 80) {         // WARNING
        air_sample.co          = 140 + (random_rand() % 60);    // 140-199
        air_sample.tvoc        = 35  + (random_rand() % 40);    // 35-74
        air_sample.air_quality = 35  + (random_rand() % 8);     // 35-42
    } else {                         // DANGER
        air_sample.co          = 195 + (random_rand() % 60);    // 195-254
        air_sample.tvoc        = 80  + (random_rand() % 40);    // 80-119
        air_sample.air_quality = 42  + (random_rand() % 8);     // 42-49
    }
}


// Funzione di gestione della risposta dal server di registrazione
void client_chunk_handler(coap_message_t *response) {
    if (response == NULL) {
        printf("Timeout della richiesta di registrazione\n");
        return;
    }

    const uint8_t *chunk;
    int len = coap_get_payload(response, &chunk);
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';

    printf("Risposta dal server: %s\n", payload);

    if (response->code == GOOD_ACK) {
        printf("Registrazione completata\n");
        registered = 1;
    } else {
        printf("Registrazione fallita\n");
    }
}

PROCESS_THREAD(air_quality_sensor_process, ev, data) {
    static coap_endpoint_t server_ep;
    static coap_message_t request[1];


    PROCESS_BEGIN();

    leds_single_on(LEDS_RED);

    PROCESS_WAIT_EVENT_UNTIL(ev == button_hal_press_event);

    leds_single_off(LEDS_RED);

    // Tentativo di registrazione con il server CoAP
    coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);
    while (registration_retry_count < MAX_REGISTRATION_RETRY && registered == 0) {
        leds_on(LEDS_RED);
        leds_single_on(LEDS_YELLOW);

        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(request, "/registrationSensor");

        // Creazione del payload JSON per la registrazione
        cJSON *root = cJSON_CreateObject();
        if (root == NULL) {
            printf("Errore nella creazione del JSON\n");
            PROCESS_EXIT();
        }
        cJSON_AddStringToObject(root, "s", "air_sample");

        cJSON *features = cJSON_CreateArray();
        cJSON_AddItemToArray(features, cJSON_CreateString("co"));
        cJSON_AddItemToArray(features, cJSON_CreateString("air"));
        cJSON_AddItemToArray(features, cJSON_CreateString("tvoc"));
        cJSON_AddItemToObject(root, "ss", features);
        cJSON_AddNumberToObject(root, "t", TIME_SAMPLE);

        char *payload = cJSON_PrintUnformatted(root);
        if (payload == NULL) {
            printf("Errore nella stampa del JSON\n");
            cJSON_Delete(root);
            PROCESS_EXIT();
        }
        coap_set_payload(request, (uint8_t *)payload, strlen(payload));

        printf("📡 Invio richiesta di registrazione...\n");
        COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);



        if (registered == 0) {
            printf("🔄 Ritentativo registrazione (%d/%d)\n", registration_retry_count, MAX_REGISTRATION_RETRY);
            registration_retry_count++;
            etimer_set(&monitoring_timer, CLOCK_SECOND * 5);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&monitoring_timer));
        }
    }

    // Se il sensore è registrato, avvia le risorse CoAP
    if (registered == 1) {
        leds_off(LEDS_RED);
        leds_single_off(LEDS_YELLOW);
        write_sample();

        printf("Attivazione server CoAP\n");
        coap_activate_resource(&res_danger, "danger");
        coap_activate_resource(&res_dangerCount, "dangerCount");
        coap_activate_resource(&res_monitoring_air, "monitoring");
        coap_activate_resource(&res_shutdown, "shutdown");

        // Impostiamo il timer per il campionamento e la predizione
        etimer_set(&monitoring_timer, CLOCK_SECOND * TIME_SAMPLE);
        
        while (1) {
            PROCESS_YIELD();

            if (air_sample.co == -1 || shutdown == 1) {
                shutdown = 1;
                printf("Sistema in spegnimento...\n");
                process_exit(&air_quality_sensor_process);
                PROCESS_EXIT();
            }

            if (etimer_expired(&monitoring_timer)) {
                write_sample();                // aggiorna dati
                res_danger.trigger();          // 🔁 notifica pericolo
                res_monitoring_air.trigger();  // 🔁 notifica stato
                etimer_reset(&monitoring_timer);
            }
        }


    } else {
        printf("Numero massimo di tentativi di registrazione raggiunto\n");
    }

    PROCESS_END();
}
