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

#define LOG_MODULE "TVOC Sensor"
#define LOG_LEVEL LOG_LEVEL_APP

#define SERVER_EP "coap://[fd00::1]:5683"
#define TIME_SAMPLE 5
#define MAX_REGISTRATION_RETRY 3
#define GOOD_ACK 65

extern coap_resource_t res_monitoring_tvoc;
extern coap_resource_t res_shutdown;

static int registration_retry_count = 0;
static int registered = 0;
static int shutdown = 0;
static float tvoc_value = 0;

PROCESS(tvoc_sensor_process, "TVOC Sensor Process");
AUTOSTART_PROCESSES(&tvoc_sensor_process);

static struct etimer monitoring_timer;

void write_sample() {
    tvoc_value = random_rand() % 500;
    printf("SAMPLE TVOC: %f ppb\n", tvoc_value);
}

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
    
    if (response->code == GOOD_ACK) {
        printf("Registration successful\n");
        registered = 1;
    } else {
        printf("Registration failed\n");
    }
}

PROCESS_THREAD(tvoc_sensor_process, ev, data) {
    static coap_endpoint_t server_ep;
    static coap_message_t request[1];
    int pressed = 0;

    PROCESS_BEGIN();

    random_init(0);
    while (ev != button_hal_press_event || pressed == 0) {
        pressed = 1;
        PROCESS_YIELD();

        coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);
        while (registration_retry_count < MAX_REGISTRATION_RETRY && registered == 0) {
            leds_on(LEDS_RED);
            coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
            coap_set_header_uri_path(request, "/registrationSensor");

            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "s", "TVOC");
            cJSON *string_array = cJSON_CreateArray();
            cJSON_AddItemToArray(string_array, cJSON_CreateString("tvoc"));
            cJSON_AddItemToObject(root, "ss", string_array);
            cJSON_AddNumberToObject(root, "t", TIME_SAMPLE);

            char *payload = cJSON_PrintUnformatted(root);
            coap_set_payload(request, (uint8_t *)payload, strlen(payload));

            COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

            if (registered == 1) {
                leds_off(LEDS_RED);
                write_sample();
                coap_activate_resource(&res_monitoring_tvoc, "monitoring");
                coap_activate_resource(&res_shutdown, "shutdown");

                etimer_set(&monitoring_timer, CLOCK_SECOND * 10);

                while (1) {
                    PROCESS_WAIT_EVENT();
                    if (shutdown == 1) {
                        process_exit(&tvoc_sensor_process);
                        PROCESS_EXIT();
                    }
                    if (etimer_expired(&monitoring_timer)) {
                        res_monitoring_tvoc.trigger();
                        write_sample();
                        etimer_reset(&monitoring_timer);
                    }
                }
            }
        }
    }

    PROCESS_END();
}
