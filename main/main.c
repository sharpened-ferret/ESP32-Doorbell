#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "discord.h"
#include "discord/session.h"
#include "discord/message.h"
#include "estr.h"
#include "time.h"

/* WiFi config settings */
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PWD CONFIG_WIFI_PASSWORD
#define MAXIMUM_RETRY CONFIG_MAXIMUM_RETRY
#define DOORBELL_MESSAGE CONFIG_DOORBELL_MESSAGE
#define CONNECTION_MSG_ENABLED CONFIG_CONNECTION_MESSAGE_ENABLED
#define CONNECTION_MSG CONFIG_CONNECTION_MESSAGE
#define CHANNEL_ID CONFIG_CHANNEL_ID
#define BELL_PIN_NUM CONFIG_BELL_PIN_NUMBER
#define BELL_TIMEOUT CONFIG_BELL_TIMEOUT

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static xQueueHandle gpio_evt_queue = NULL;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

 
static bool BOT_CONNECTED = false;
//static bool RESTART_FLAG = false;
static time_t LAST_RING = 0;
static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PWD,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PWD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PWD);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}


static discord_handle_t bot;

static void bot_discord_event_handler(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_event_data_t* data = (discord_event_data_t*) event_data;

    switch (event_id) {
        case DISCORD_EVENT_CONNECTED: {
                discord_session_t* session = (discord_session_t*) data->ptr;
                ESP_LOGI("BOT", "Bot %s#%s connected", session->user->username, session->user->discriminator);
                BOT_CONNECTED = true;
                if (CONNECTION_MSG_ENABLED) {
                    discord_message_t connectionMsg = {
                        .content = CONNECTION_MSG,
                        .channel_id = CHANNEL_ID
                    };
                    discord_message_send(bot, &connectionMsg, NULL);
                }
                
            }
            break;

        case DISCORD_EVENT_DISCONNECTED: {
                BOT_CONNECTED = false;
                ESP_LOGI("BOT", "Bot Disconnected");
            }
            break;
    }
}

static void doorbell_messaging_service(void* arg) {
    uint32_t gpio_num;
    for (;;) {
        if(xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY)) {
            if (BOT_CONNECTED) {
                discord_message_t bellMsg = {
                    .content = DOORBELL_MESSAGE,
                    .channel_id = CHANNEL_ID
                };
                discord_message_send(bot, &bellMsg, NULL);
            }
        }
    }
}


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    time_t current_time = time(NULL);
    if ((current_time - LAST_RING) > BELL_TIMEOUT) {
        uint32_t gpio_num = (uint32_t) arg;
        LAST_RING = current_time;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    }
}

void app_main(void)
{
     //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();


    printf("Hello world!\n");


    discord_config_t cfg = {
        .intents = DISCORD_INTENT_GUILD_MESSAGES
    };

    bot = discord_create(&cfg);
    discord_register_events(bot, DISCORD_EVENT_ANY, bot_discord_event_handler, NULL);
    discord_login(bot);
    printf("Bot created!\n");


    
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(doorbell_messaging_service, "gpio_doorbell_trigger", 2048, NULL, 10, NULL);

    gpio_install_isr_service(0);

    gpio_set_intr_type(BELL_PIN_NUM, GPIO_INTR_POSEDGE);
    gpio_isr_handler_add(BELL_PIN_NUM, gpio_isr_handler, (void*) BELL_PIN_NUM);

    // for (int i = 10; i >= 0; i--) {
    //         printf("Restarting in %d seconds...\n", i);
    //         vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     }
    // printf("Restarting now.\n");
    // fflush(stdout);
    // esp_restart();
}


