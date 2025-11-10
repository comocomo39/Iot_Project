#include "contiki.h"
#include "coap-engine.h"
#include "random.h"
#include "contiki-net.h"
#include <stdio.h>
#include "sys/log.h"
#include "coap-blocking-api.h"
#include "os/dev/button-hal.h"
#include "leds.h"
#include "../cJSON-master/cJSON.h"
#include <stdlib.h>
#include <string.h>
#include "global_variable/global_variables.h"
#include "os/dev/button-hal.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

#define SERVER_EP "coap://[fd00::1]:5683"
#define TIME_SAMPLE 2
#define MAX_REGISTRATION_RETRY 3
#define GOOD_ACK 65
#define MAX_SAMPLES 10  // Manteniamo gli ultimi 10 campioni per la serie temporale

extern coap_resource_t res_predict_temp;
extern coap_resource_t res_monitoring_temp;
extern coap_resource_t res_shutdown;

static int registration_retry_count = 0;
static int registered = 0;
static int shutdown = 0;

int sample_count = 0; // Numero di campioni salvati
static int id_counter = 0; // ID unico per ogni sample
Sample samples[MAX_SAMPLES];

PROCESS(temperature_sensor_process, "Temperature Sensor Process");
AUTOSTART_PROCESSES(&temperature_sensor_process);

static struct etimer prediction_timer;
static struct etimer monitoring_timer;

// Funzione per raccogliere un nuovo campione di dati
#define T_MIN   28
#define T_MAX   35
#define H_MIN   30
#define H_MAX   80
#define D_MIN    0
#define D_MAX   100

// stato “persistente” tra una chiamata e l’altra
static int last_temp = 30;
static int last_hum  = 45;
static int last_dust = 20;

void write_samples() {
    // piccolo passo casuale in [-1, +1]
    int dt = (random_rand() % 3) - 1;
    int dh = (random_rand() % 3) - 1;
    int dd = (random_rand() % 3) - 1;

    // aggiorno con clamp sul min/max
    last_temp = last_temp + dt;
    if (last_temp < T_MIN) last_temp = T_MIN;
    if (last_temp > T_MAX) last_temp = T_MAX;

    last_hum = last_hum + dh;
    if (last_hum < H_MIN) last_hum = H_MIN;
    if (last_hum > H_MAX) last_hum = H_MAX;

    last_dust = last_dust + dd;
    if (last_dust < D_MIN) last_dust = D_MIN;
    if (last_dust > D_MAX) last_dust = D_MAX;

    // inserisco nel buffer (stesso FIFO di prima)
    if (sample_count < MAX_SAMPLES) {
        samples[sample_count].timeid      = id_counter++;
        samples[sample_count].temperature = last_temp;
        samples[sample_count].humidity    = last_hum;
        samples[sample_count].dust_density= last_dust;
        sample_count++;
    } else {
        for (int i = 0; i < MAX_SAMPLES - 1; i++) {
            samples[i] = samples[i + 1];
        }
        samples[MAX_SAMPLES - 1].timeid       = id_counter++;
        samples[MAX_SAMPLES - 1].temperature  = last_temp;
        samples[MAX_SAMPLES - 1].humidity     = last_hum;
        samples[MAX_SAMPLES - 1].dust_density = last_dust;
    }
}


// Funzione per gestire la registrazione del sensore al server CoAP
void client_chunk_handler(coap_message_t *response) {
    const uint8_t *chunk;
    if (response == NULL) {
        LOG_ERR("Request timed out\n");
        return;
    }
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

PROCESS_THREAD(temperature_sensor_process, ev, data) {
    static coap_endpoint_t server_ep;
    static coap_message_t request[1];

    PROCESS_BEGIN();

    random_init(0); // Inizializza generatore di numeri casuali

    leds_single_on(LEDS_RED);

    PROCESS_WAIT_EVENT_UNTIL(ev == button_hal_press_event);

    leds_single_off(LEDS_RED);

    // Tentativo di registrazione con il server
    coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);
    while (registration_retry_count < MAX_REGISTRATION_RETRY && registered == 0) {
        leds_on(LEDS_RED);
        leds_single_on(LEDS_YELLOW);

        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(request, "/registrationSensor");

        // Creazione del payload JSON
        cJSON *root = cJSON_CreateObject();
        if (root == NULL) {
            LOG_ERR("Errore nella creazione del JSON\n");
            PROCESS_EXIT();
        }
        cJSON_AddStringToObject(root, "s", "env_sample");

        cJSON *features = cJSON_CreateArray();
        cJSON_AddItemToArray(features, cJSON_CreateString("temp"));
        cJSON_AddItemToArray(features, cJSON_CreateString("hum"));
        cJSON_AddItemToArray(features, cJSON_CreateString("dust_d"));
        cJSON_AddItemToObject(root, "ss", features);
        cJSON_AddNumberToObject(root, "t", TIME_SAMPLE);

        char *payload = cJSON_PrintUnformatted(root);
        if (payload == NULL) {
            LOG_ERR("Errore nella stampa del JSON\n");
            cJSON_Delete(root);
            PROCESS_EXIT();
        }
        coap_set_payload(request, (uint8_t *)payload, strlen(payload));

        printf("📡 Invio richiesta di registrazione...\n");
        COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

        if (registered == 0) {
            LOG_INFO("Ritento la registrazione (%d/%d)\n", registration_retry_count, MAX_REGISTRATION_RETRY);
            registration_retry_count++;
            etimer_set(&prediction_timer, CLOCK_SECOND * 5);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&prediction_timer));
        }
    }

    // Se il sensore è registrato, avvia le risorse CoAP
    if (registered == 1) {
        leds_off(LEDS_RED);
        leds_single_off(LEDS_YELLOW);

        printf("✅ Attivazione server CoAP\n");
        coap_activate_resource(&res_predict_temp, "predict-temp");
        coap_activate_resource(&res_monitoring_temp, "monitoring");
        coap_activate_resource(&res_shutdown, "shutdown");

        // Impostiamo i timer per il campionamento e la predizione
        etimer_set(&prediction_timer, CLOCK_SECOND * (TIME_SAMPLE));
        etimer_set(&monitoring_timer, CLOCK_SECOND * (TIME_SAMPLE));

        while (1) {
            PROCESS_YIELD();

            if(samples[0].humidity == -1 || shutdown == 1) {
              shutdown = 1;
              printf("Sistema in spegnimento…\n");
              process_exit(&temperature_sensor_process);
              PROCESS_EXIT();
            }

            if (etimer_expired(&monitoring_timer)) {
                write_samples();                    // <--- aggiungi qui
                res_monitoring_temp.trigger();      // notifica valore attuale
                etimer_reset(&monitoring_timer);
            }
            if (etimer_expired(&prediction_timer)) {
                              // <--- aggiungi qui
                res_predict_temp.trigger();
                
                etimer_reset(&prediction_timer);
            }
        }
    } else {
        printf("Numero massimo di tentativi di registrazione raggiunto\n");
    }

    PROCESS_END();
}
