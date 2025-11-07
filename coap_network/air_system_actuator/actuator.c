#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/etimer.h"
#include "leds.h"
#include "../cJSON-master/cJSON.h"
#include "os/dev/button-hal.h"

#if PLATFORM_SUPPORTS_BUTTON_HAL
#include "dev/button-hal.h"
#else
#include "dev/button-sensor.h"
#endif

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL  LOG_LEVEL_DBG

#define SERVER_EP "coap://[fd00::1]:5683"
#define REGISTRATION_ATTEMPTS 5
#define REGISTRATION_DELAY 10 // in seconds

static char* service_url_reg = "/registrationActuator";
static char* service_url_danger = "danger";
static int registration_attempts = 0;
static int registered = 0;

int shutdown = 0;
int danger_threshold = 0;

extern coap_resource_t res_threshold;
extern coap_resource_t res_shutdown;

static char ipv6_danger[50];
static coap_observee_t *obs_danger = NULL;

PROCESS(coap_client_process, "CoAP Client Process");
AUTOSTART_PROCESSES(&coap_client_process);

void response_handler_danger(coap_message_t *response) {
  if(response == NULL) return;

  const uint8_t *chunk; int len = coap_get_payload(response, &chunk);
  char buf[len+1]; memcpy(buf, chunk, len); buf[len]='\0';

  int danger = atoi(buf);   // 0-1-2
  printf("Livello di pericolo: %d\n", danger);

  /* esempio di reazione: */
  switch(danger) {
    case 0: leds_single_off(LEDS_YELLOW); leds_single_off(LEDS_RED); leds_single_on(LEDS_GREEN);  break;
    case 1: leds_single_off(LEDS_GREEN); leds_single_off(LEDS_RED); leds_single_on(LEDS_YELLOW); break;
    case 2: leds_single_off(LEDS_GREEN); leds_single_off(LEDS_YELLOW); leds_single_on(LEDS_RED);  break;
  }
}


void handle_notification_danger(struct coap_observee_s *observee, void *notification, coap_notification_flag_t flag) {
  coap_message_t *msg = (coap_message_t *)notification;
  if (msg) {
    printf("Received danger notification\n");
    response_handler_danger(msg);
    process_poll(&coap_client_process);
  } else {
    printf("No danger notification received\n");
  }
}

void registration_handler(coap_message_t *response) {
  if (response == NULL) {
    printf("No response received.\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  char payload[len + 1];
  strncpy(payload, (char *)chunk, len);
  payload[len] = '\0';
  printf("Registration payload: %s\n", payload);

  cJSON *json = cJSON_Parse(payload);
  if (json == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      fprintf(stderr, "JSON parsing error: %s\n", error_ptr);
    }
    return;
  }

  // 🔹 Cerca solo il campo "e" (endpoint del sensore)
  cJSON *ipv6_item = cJSON_GetObjectItemCaseSensitive(json, "e");
  if (cJSON_IsString(ipv6_item)) {
    snprintf(ipv6_danger, sizeof(ipv6_danger), "%s", ipv6_item->valuestring);
    printf("📡 IPv6 ricevuto dal server: %s\n", ipv6_danger);
    registered = 1;
  } else {
    printf("⚠️ Nessun campo \"e\" trovato nel JSON!\n");
  }
  cJSON_Delete(json);
}

PROCESS_THREAD(coap_client_process, ev, data) {
  PROCESS_BEGIN();

  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static struct etimer registration_timer;

  leds_single_on(LEDS_RED);

  PROCESS_WAIT_EVENT_UNTIL(ev == button_hal_press_event);

  leds_single_off(LEDS_RED);

  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

  while (registration_attempts < REGISTRATION_ATTEMPTS && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, service_url_reg);
    coap_set_payload(request, (uint8_t *)"actuator", strlen("actuator"));
    printf("Sending registration request...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, registration_handler);

    if (!registered) {
      registration_attempts++;
      etimer_set(&registration_timer, CLOCK_SECOND * REGISTRATION_DELAY);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&registration_timer));
    }
  }

  if (registered) {
    static coap_endpoint_t server_ep_danger;
    char addr_danger[100] = "coap://[";
    strcat(addr_danger, ipv6_danger);
    strcat(addr_danger, "]:5683");

    coap_endpoint_parse(addr_danger, strlen(addr_danger), &server_ep_danger);
    printf("Sending observation request to %s\n", addr_danger);

    obs_danger = coap_obs_request_registration(&server_ep_danger, service_url_danger, handle_notification_danger, NULL);

    coap_activate_resource(&res_threshold, "threshold");
    coap_activate_resource(&res_shutdown, "shutdown");

    while (1) {
      PROCESS_WAIT_EVENT();
      if (danger_threshold == -1 || shutdown == 1) {
        printf("Shutdown activated, exiting process.\n");
        if (obs_danger != NULL) {
          coap_obs_remove_observee(obs_danger);
        }
        process_exit(&coap_client_process);
        PROCESS_EXIT();
      }
    }
  } else {
    printf("Failed to register after %d attempts\n", REGISTRATION_ATTEMPTS);
  }


  PROCESS_END();
}
