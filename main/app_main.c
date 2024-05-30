#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "driver/mcpwm.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>
#include "servo.h"
#include <soc/gpio_num.h>

//khai báo mqtt
#define BROKER_URL "mqtts://d461e7a5dbd3402da2b7fccff53666ad.s1.eu.hivemq.cloud"
#define TOPIC_SET_ANGLE "/smarthome/servo"

//khai báo username, password của mqtt
static const char *username = "ce232";
static const char *password = "Tuong12345";

static const char *TAG = "servo_example";

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
#endif
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");

// Khai báo hàm handle_user_input để tránh lỗi "implicit declaration"
// Hàm này từ trong hàm process_webclient_data để tránh lỗi và đồng thời thực hiện chức năng điều khiển servo
// Nếu nhận được msg từ mqtt = "open" thì cập nhật góc quay 0 -> 90 độ và "close" 90 -> 0 độ
void handle_user_input(const char* command) {
    int angle = 0;
    if (strcmp(command, "open") == 0) {
        angle = 90;
         for (int i = 0; i <= SERVO_MAX_DEGREE; i++) {
            servo_per_degree_init(i);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    } else if (strcmp(command, "close") == 0) {
        angle = 0;
        for (int i = SERVO_MAX_DEGREE; i >= 0; i--) {
            servo_per_degree_init(i);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    } else {
        ESP_LOGW(TAG, "Invalid command: %s", command);
        return;
    }
    // Set the target angle for the servo
    targetAngle = angle;
    ESP_LOGI(TAG, "Servo angle updated: %d\n", angle); // In ra góc đã được cập nhật
}

// Hàm xử lý dữ liệu từ webclient
void process_webclient_data(char *data) {
    // Kiểm tra xem dữ liệu có tồn tại không
    if (data == NULL) {
        printf("Error: Empty data received from webclient.\n");
        return;
    }

    // Kiểm tra độ dài của dữ liệu
    int data_length = strlen(data);
    if (data_length == 0) {
        printf("Error: Empty data received from webclient.\n");
        return;
    }

    // Gọi hàm xử lý lệnh từ webclient
    handle_user_input(data);
}


// Hàm handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, TOPIC_SET_ANGLE, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s, DATA=%.*s", event->topic_len, event->topic, event->data_len, event->data);
            
            // Copy dữ liệu nhận từ mqtt được để xử lý
            char *data = (char *)malloc(event->data_len + 1);
            if (data == NULL) {
                printf("Error: Could not allocate memory for data\n");
                return;
            }
            strncpy(data, event->data, event->data_len);
            data[event->data_len] = '\0';
            printf("Received data: %s\r\n", data);
            // Gọi hàm xử lý dữ liệu
            process_webclient_data(data);
            free(data);

            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                         strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

// kết nối mqtt, đăng kí topic
static void mqtt_app_start(void) {
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = BROKER_URL,
            .verification.certificate = (const char *)mqtt_eclipseprojects_io_pem_start
        },
        .credentials = {
            .username = username,
            .authentication.password = password
        }
    };

    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    }

void app_main(void) {
    ESP_LOGI(TAG, "[APP] Khởi động..");
    ESP_LOGI(TAG, "[APP] Bộ nhớ trống: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] Phiên bản IDF: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Khởi tạo GPIO cho servo
    esp_rom_gpio_pad_select_gpio(GPIO_OUTPUT_IO_0);
    gpio_set_direction(GPIO_OUTPUT_IO_0, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_OUTPUT_IO_0, 0);

    // Khởi tạo LEDC cho servo
    servo_init();

    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
}
