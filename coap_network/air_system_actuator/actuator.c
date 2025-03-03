#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/etimer.h"
#include "../cJSON-master/cJSON.h"
#include "leds.h"

#define SERVER_EP "coap://[fd00::1]:5683"

#define REGISTRATION_ATTEMPTS 5
#define REGISTRATION_DELAY 10 // in seconds

static char* service_url_danger = "/res-danger";
static char* service_url_reg = "/registrationActuator";
extern coap_resource_t res_tresh;
extern coap_resource_t res_shutdown;

static int registration_attempts = 0;
static int registered = 0;
static int danger_level = 0; // 0 = Buona, 1 = Moderata, 2 = Scarsa
static int shutdown = 0;

PROCESS(coap_client_process, "CoAP Client Process");
AUTOSTART_PROCESSES(&coap_client_process);

static coap_observee_t *obs_danger = NULL;

void response_handler_danger(coap_message_t *response) {
    if(response == NULL) {
        printf("❌ Nessuna risposta ricevuta dal server.\n");
        return;
    }

    const uint8_t *chunk;
    int len = coap_get_payload(response, &chunk);
    char danger_str[len + 1];
    strncpy(danger_str, (char *)chunk, len);
    danger_str[len] = '\0';
    danger_level = atoi(danger_str);

    printf("📡 Livello di Pericolo Ricevuto: %d\n", danger_level);

    // Attivare l'attuatore in base al livello di pericolo
    if (danger_level == 0) {
        printf("🟢 Qualità dell'aria BUONA. Nessuna azione necessaria.\n");
        leds_off(LEDS_RED);
        leds_on(LEDS_GREEN);
    } else if (danger_level == 1) {
        printf("🟡 Qualità dell'aria MODERATA. Monitoraggio attivo.\n");
        leds_on(LEDS_YELLOW);
        leds_off(LEDS_GREEN);
    } else if (danger_level == 2) {
        printf("🔴 QUALITÀ DELL'ARIA SCARSA! ATTIVAZIONE SISTEMA!\n");
        leds_on(LEDS_RED);
        leds_off(LEDS_GREEN);
    } else {
        printf("❌ Errore nella classificazione della qualità dell'aria.\n");
    }
}

void handle_notification_danger(struct coap_observee_s *observee, void *notification, coap_notification_flag_t flag) {
    coap_message_t *msg = (coap_message_t *)notification;
    if (msg) {
        printf("📡 Notifica ricevuta sul livello di pericolo dell'aria.\n");
        response_handler_danger(msg);
        process_poll(&coap_client_process); // Poll per aggiornare lo stato
    } else {
        printf("❌ Nessuna notifica ricevuta.\n");
    }
}

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

    while (registration_attempts < REGISTRATION_ATTEMPTS && !registered) {
        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(request, service_url_reg);
        coap_set_payload(request, (uint8_t *)"actuator", strlen("actuator"));

        printf("📡 Invio richiesta di registrazione...\n");
        COAP_BLOCKING_REQUEST(&server_ep, request, registration_handler);

        if (!registered) {
            registration_attempts++;
            etimer_set(&registration_timer, CLOCK_SECOND * REGISTRATION_DELAY);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&registration_timer));
        }
    }

    if (registered) {
        printf("✅ Attuatore registrato con successo.\n");

        static coap_endpoint_t server_ep_danger;
        char addr_danger[100] = "coap://[";
        strcat(addr_danger, "fd00::1");
        strcat(addr_danger, "]:5683");

        coap_endpoint_parse(addr_danger, strlen(addr_danger), &server_ep_danger);

