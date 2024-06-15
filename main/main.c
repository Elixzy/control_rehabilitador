#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
//librerias del display
#include "st7789.h"
#include "fontx.h"
//librerias de wifi
#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
//librerias de pines y ADC
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
//pines de entrada para los push buttons
#define BOTON_PIN_1 GPIO_NUM_15
#define BOTON_PIN_2 GPIO_NUM_2
#define BOTON_PIN_3 GPIO_NUM_5
#define BUTTON_PIN_4 GPIO_NUM_19
//canal de wifi
#define ESP_CHANNEL 1
//etiqueta para logs en consola
#define TAG "esp32"
//variables de operacion globales
uint8_t count=0;
uint8_t peer_mac[6]={0xe0,0x5a,0x1b,0xd2,0x48,0xe8};//direccion mac del receptor
uint8_t ascii[24];
uint16_t color=0x001f;
uint16_t color1=0x001f;
bool block=true;
bool D4=false;
char data_string[24];
char int_string[24];
int adc_raw;

//funcion que procesa los datos que recibe del otro esp32
void recv_cb(const esp_now_recv_info_t * esp_now_info,
		const uint8_t *data, int data_len){
	sprintf(int_string,"%s", data);
	//se agrega el dato recibido a una variable global de tipo string
}
//funcion que se ejecuta al enviar un dato
void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status){
	if(status==ESP_NOW_SEND_SUCCESS){
		ESP_LOGI(TAG, "ESP_NOW_SEND_SUCCESS");
	}
	else{
		ESP_LOGW(TAG, "ESP_NOW_SEND_FAIL");
	}
	//avisa por consola si el dato se envió correctamente
}
//funcion para inicializar el wifi
void init_wifi(void){
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	esp_netif_init();
	esp_event_loop_create_default();
	nvs_flash_init();
	esp_wifi_init(&wifi_init_config);
	esp_wifi_set_mode(WIFI_MODE_STA);//modo station
	esp_wifi_start();
	ESP_LOGI(TAG, "wifi init complete");
}
//funcion para inicializar el protocolo esp-now
void init_esp_now(void){
	esp_now_init();
	esp_now_register_recv_cb(recv_cb);//se agregan las funciones que se ejecutaran al recibir o enviar datos
	esp_now_register_send_cb(send_cb);
	ESP_LOGI(TAG, "esp now init complete");
}
//funcion para configurar la direccion mac del receptor
void register_peer(uint8_t *peer_addr){
	esp_now_peer_info_t esp_now_peer_info={};
	memcpy(esp_now_peer_info.peer_addr, peer_mac, 6);
	esp_now_peer_info.channel=ESP_CHANNEL;
	esp_now_peer_info.ifidx=ESP_IF_WIFI_STA;
	esp_now_add_peer(&esp_now_peer_info);
	ESP_LOGI(TAG, "register peer init complete");
}
//funcion para crear una carpeta en la memoria flash del esp32 y acceder a archivos de fuentes de letras para el display
void SPIFFS_Directory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}
//funcion para encviar datos al otro esp32
void send_data(const uint8_t *data, size_t len) {
    // Enviar datos al dispositivo receptor
    esp_now_send(NULL, data, len);
}
//tarea que se ejecuta en paralelo que se encarga de las tareas del wifi
void wifi_task(){
	//se inicializa el ADC con un un rango de 9 bits 0-511
	adc_oneshot_unit_handle_t adc2_handle;
	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ADC_UNIT_1,};
	adc_oneshot_new_unit(&init_config1, &adc2_handle);
	adc_oneshot_chan_cfg_t config = {
		.bitwidth = ADC_BITWIDTH_9,
		.atten = ADC_ATTEN_DB_11,};
	adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_7, &config);
	//se llaman a las funciones correspondientes a la inicializacion del wifi y esp-now
	init_wifi();
	init_esp_now();
	register_peer(peer_mac);
	char *message;
		while(1){
			//pin de seleccion de opciones
			if(!gpio_get_level(BOTON_PIN_1)){
				if(block){
				count++;
				if(count==6){count=0;}
				}
				block=false;
			}
			//pin de ENVIAR
			else if(!gpio_get_level(BOTON_PIN_3)){
				if(block){
					color=0xffff;
					switch(count){
					  case 0:
						  message="D0";
						  D4=false;
						  send_data((const uint8_t *)message, strlen(message));
						  break;
					  case 1:
						  message="D1";
						  D4=false;
						  send_data((const uint8_t *)message, strlen(message));
						  break;
					  case 2:
						  message="D2";
						  D4=false;
						  send_data((const uint8_t *)message, strlen(message));
						  break;
					  case 3:
						  message="D3";
						  D4=false;
						  send_data((const uint8_t *)message, strlen(message));
						  break;
					  case 4:
						  message="D4";
						  D4=true;
						  send_data((const uint8_t *)message, strlen(message));
						  break;
					  case 5:
						  message="D5";
						  D4=true;
						  send_data((const uint8_t *)message, strlen(message));
						  break;
					}
				}
				block=false;
			}
			//pin de DETENER
			else if(!gpio_get_level(BUTTON_PIN_4)){
				if(block){
				color1=0xffff;
				message="STOP";
				D4=false;
				send_data((const uint8_t *)message, strlen(message));
				}
				block=false;
			}

			else{
				block=true;
				color=0x001f;
				color1=0x001f;
			}
			//se evian constantemente datos del ADC
			if(D4){
				adc_oneshot_read(adc2_handle, ADC_CHANNEL_7, &adc_raw);
				adc_raw=adc_raw+10;
				sprintf(data_string,"%d", adc_raw);
				send_data((const uint8_t *)data_string, strlen(data_string));
			}
			delayMS(20);
		}
}
//funcion principal
void app_main(void){
	//se inicializa la carpeta dentro de la memoria flash
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 12,
		.format_if_mount_failed =true};
	esp_vfs_spiffs_register(&conf);
	SPIFFS_Directory("/spiffs/");
	//se inicializa la fuente que usaremos para mostrar textos en la pantalla
	FontxFile fx24G[2];
	InitFontx(fx24G,"/spiffs/ILGH24XB.FNT","");
	//se inicializa la comunicacion SPI a 40Mhz
	TFT_t dev;
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO,
			CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	//se inicializa el display
	lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
	lcdFillScreen(&dev, BLUE);
	lcdDrawFinish(&dev);
	delayMS(1000);
	//se configuran los pines como entrada y se activan la resistencias internas de PULL-UP
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<BOTON_PIN_1) |
    						(1ULL<<BOTON_PIN_2) |
							(1ULL<<BOTON_PIN_3) |
							(1ULL<<BUTTON_PIN_4);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE,
    gpio_config(&io_conf);
    //se inicia una tarea en paralelo llamada wifi task
    xTaskCreate(wifi_task, "wifi task", 1024*6, NULL, 2, NULL);

	while (true){
		//interfaz gráfica con el display
		lcdFillScreen(&dev, BLACK);
		lcdDrawRoundRect(&dev, 4, 4+count*(30), 110, 32+count*(30), 5, GREEN);
		lcdDrawRoundRect(&dev, 115, 4, 235, 182, 5, BLUE);
		lcdDrawFillRect(&dev, 4, 204, 110, 232, color);
		lcdDrawFillRect(&dev, 115, 204, 235, 232, color1);
		strcpy((char *)ascii, "ANGULO");
		lcdDrawString(&dev, fx24G, 140, 30, ascii, RED);
		strcpy((char *)ascii, int_string);
		lcdDrawString(&dev, fx24G, 130, 60, ascii, CYAN);
		strcpy((char *)ascii, "RUTINA 1");
		lcdDrawString(&dev, fx24G, 10, 30, ascii, RED);
		strcpy((char *)ascii, "RUTINA 2");
		lcdDrawString(&dev, fx24G, 10, 60, ascii, RED);
		strcpy((char *)ascii, "RUTINA 3");
		lcdDrawString(&dev, fx24G, 10, 90, ascii, RED);
		strcpy((char *)ascii, "RUTINA 4");
		lcdDrawString(&dev, fx24G, 10, 120, ascii, RED);
		strcpy((char *)ascii, "MANUAL X");
		lcdDrawString(&dev, fx24G, 10, 150, ascii, RED);
		strcpy((char *)ascii, "MANUAL Y");
		lcdDrawString(&dev, fx24G, 10, 180, ascii, RED);
		strcpy((char *)ascii, "ENVIAR");
		lcdDrawString(&dev, fx24G, 20, 230, ascii, BLACK);
		strcpy((char *)ascii, "DETENER");
		lcdDrawString(&dev, fx24G, 130, 230, ascii, BLACK);
		lcdDrawFinish(&dev);
    	delayMS(20);
    }
}