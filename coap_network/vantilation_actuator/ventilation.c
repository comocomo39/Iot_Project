#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/etimer.h"
#include "os/dev/leds.h"
#include "../cJSON-master/cJSON.h"
#include "global_variable/global_variables.h"

#define GOOD_ACK 65
#define SERVER_EP "coap://[fd00::1]:5683"

#define MAX_REGISTRATION_ENTRY 5
static char* service_url_temp = "/predict-temp";

static float predicted_temperature = 0;
static int registered = 0;
static int registration_retry_count = 0;
static int shutdown_flag = 0;

extern coap_resource_t res_tresh;
extern coap_resource_t res_shutdown;

PROCESS(coap_client_process, "CoAP Client Process");
AUTOSTART_PROCESSES(&coap_client_process);

static coap_observee_t *obs_temp = NULL;

// Funzione per attivare o disattivare la ventilazione in base alla temperatura
void control_ventilation(float predicted_temp, float threshold) {
    if (predicted_temp < threshold) {
        printf("🟢 Temperatura OK (%.2f°C). Ventilazione SPENTA.\n", predicted_temp);
        leds_off(LEDS_RED);
        leds_on(LEDS_GREEN);
    } else {
        printf("🔴 Temperatura ALTA (%.2f°C). ATTIVAZIONE VENTILAZIONE!\n", predicted_temp);
        leds_on(LEDS_RED);
        leds_off(LEDS_GREEN);
    }
}

// Handler per la risposta alla predizione della temperatura
void response_handler_temp(coap_message_t *response) {
    if (response == NULL) {
        printf("❌ Nessuna risposta ricevuta dal server.\n");
        return;
    }

    const uint8_t *chunk;
    int len = coap_get_payload(response, &chunk);
    char temp_str[len + 1];
    strncpy(temp_str, (char *)chunk, len);
    temp_str[len] = '\0';
    predicted_temperature = atof(temp_str);

    printf("📡 Temperatura Predetta: %.2f°C\n", predicted_temperature);

    // Controllo della ventilazione in base alla soglia
    control_ventilation(predicted_temperature, temp_tresh);
}

// Handler per la notifica della predizione della temperatura
void handle_notification_temp(struct coap_observee_s *observee, void *notification, coap_notification_flag_t flag) {
    coap_message_t *msg = (coap_message_t *)notification;
    if (msg) {
        printf("📡 Notifica ricevuta sulla predizione della temperatura.\n");
        response_handler_temp(msg);
        process_poll(&coap_client_process);
    } else {
        printf("❌ Nessuna notifica ricevuta.\n");
    }
}

// Handler per la registrazione dell'attuatore
void registration_handler(coap_message_t *response) {
    if (response == NULL) {
        printf("❌ Nessuna risposta ricevuta per la registrazione.\n");
        return;
    }

    const uint8_t *chunk;
    int len = coap_get_payload(response, &chunk);
    char payload[len + 1];
    strncpy(payload, (char *)chunk, len);
    payload[len] = '\0';

    printf("📡 Risposta dal server per la registrazione: %s\n", payload);

    if (response->code == COAP_RESPONSE_CODE(201)) {
        registered = 1;
        printf("✅ Registrazione completata.\n");
    } else {
        printf("❌ Registrazione fallita.\n");
    }
}

PROCESS_THREAD(coap_client_process, ev, data) {
    PROCESS_BEGIN();
    static coap_endpoint_t server_ep;
    static coap_message_t request[1];
    static struct etimer registration_timer;

    coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

    while (registration_retry_count < MAX_REGISTRATION_ENTRY && !registered) {
        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(request, "/registrationActuator");
        coap_set_payload(request, (uint8_t *)"ventilation", strlen("ventilation"));

        printf("📡 Invio richiesta di registrazione...\n");
        COAP_BLOCKING_REQUEST(&server_ep, request, registration_handler);

        if (!registered) {
            registration_retry_count++;
            etimer_set(&registration_timer, CLOCK_SECOND * 10);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&registration_timer));
        }
    }

    if (registered) {
        printf("✅ Ventilatore registrato con successo.\n");

        static coap_endpoint_t server_ep_temp;
        char addr_temp[100] = "coap://[fd00::1]:5683";

        coap_endpoint_parse(addr_temp, strlen(addr_temp), &server_ep_temp);

        printf("📡 Invio richiesta di osservazione su %s\n", addr_temp);
        obs_temp = coap_obs_request_registration(&server_ep_temp, service_url_temp, handle_notification_temp, NULL);

        coap_activate_resource(&res_tresh, "threshold");
        coap_activate_resource(&res_shutdown, "shutdown");

        while (1) {
            PROCESS_WAIT_EVENT();
            if (shutdown_flag == 1) {
                printf("❌ Spegnimento ventilazione...\n");
                if (obs_temp != NULL) {
                    coap_obs_remove_observee(obs_temp);
                }
                process_exit(&coap_client_process);
                PROCESS_EXIT();
            }
        }
    } else {
        printf("❌ Fallita la registrazione dopo %d tentativi\n", MAX_REGISTRATION_ENTRY);
    }

    PROCESS_END();
}
