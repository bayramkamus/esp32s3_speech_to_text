#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "driver/i2s.h"

#define SAMPLE_RATE     16000
#define SAMPLE_BITS     I2S_BITS_PER_SAMPLE_16BIT
#define CHANNEL_FORMAT  I2S_CHANNEL_FMT_ONLY_LEFT
#define RECORD_TIME     2
#define BUFFER_SIZE     (SAMPLE_RATE * RECORD_TIME)

#define I2S_WS  42
#define I2S_SD  2
#define I2S_SCK 41

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#define SERVER_URL "http://<Flask_IP>:5000/upload"

static const char* TAG = "ESP32_SPEECH";

void create_wav_header(uint8_t* header, size_t pcm_size, int sample_rate) {
    int byte_rate = sample_rate * 2;
    int block_align = 2;
    memcpy(header, "RIFF", 4);
    uint32_t chunk_size = pcm_size + 36;
    memcpy(header + 4, &chunk_size, 4);
    memcpy(header + 8, "WAVEfmt ", 8);
    uint32_t subchunk1_size = 16;
    memcpy(header + 16, &subchunk1_size, 4);
    header[20] = 1; header[21] = 0;
    header[22] = 1; header[23] = 0;
    memcpy(header + 24, &sample_rate, 4);
    memcpy(header + 28, &byte_rate, 4);
    header[32] = block_align; header[33] = 0;
    header[34] = 16; header[35] = 0;
    memcpy(header + 36, "data", 4);
    memcpy(header + 40, &pcm_size, 4);
}

void wifi_init_sta() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();

    ESP_LOGI(TAG, "Connecting to Wi-Fi...");

    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    for (int i = 0; i < 10; i++) {
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
            ESP_LOGI(TAG, "✅ IP: " IPSTR, IP2STR(&ip_info.ip));
            return;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGE(TAG, "❌ Failed to get IP address..");
}

void record_and_send_audio() {
    ESP_LOGI(TAG, "🎙️ Starting audio recording...");
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = SAMPLE_BITS,
        .channel_format = CHANNEL_FORMAT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .intr_alloc_flags = 0
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);

    uint8_t* pcm_data = malloc(BUFFER_SIZE * 2);
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, pcm_data, BUFFER_SIZE * 2, &bytes_read, portMAX_DELAY);
    ESP_LOGI(TAG, "🎧 bytes_read: %d", bytes_read);

    uint8_t* wav_data = malloc(bytes_read + 44);
    create_wav_header(wav_data, bytes_read, SAMPLE_RATE);
    memcpy(wav_data + 44, pcm_data, bytes_read);
    free(pcm_data);

    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .method = HTTP_METHOD_POST,
        .buffer_size = 2048,
        .timeout_ms = 15000,
        .transport_type = HTTP_TRANSPORT_OVER_TCP
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "audio/wav");
    esp_http_client_set_post_field(client, (char*)wav_data, bytes_read + 44);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "📤 Audio successfully sent to server.");
        char response[64] = {0};
        int len = esp_http_client_read_response(client, response, sizeof(response) - 1);
        response[len] = '\0';
        ESP_LOGI(TAG, "🛰️ Server response: \"%s\"", response);
    } else {
        ESP_LOGE(TAG, "🚫 HTTP send error: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(wav_data);
    i2s_driver_uninstall(I2S_NUM_0);
}

esp_err_t start_record_handler(httpd_req_t *req) {
    ESP_LOGI("HTTP", "🔔 /start-record isteği alındı");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    record_and_send_audio();
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t cors_options_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "*");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t record_uri = {
            .uri = "/start-record",
            .method = HTTP_GET,
            .handler = start_record_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &record_uri);

        httpd_uri_t cors_uri = {
            .uri = "/*",
            .method = HTTP_OPTIONS,
            .handler = cors_options_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &cors_uri);
    }
    return server;
}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI("INIT", "NVS hazır");
    wifi_init_sta();
    start_webserver();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}