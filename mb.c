#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <unistd.h> 
//#include <time.h>
#include <string.h>
#include<pthread.h>
#include <mosquitto.h>

typedef struct
{
	const char *identifier;
	int address;
	const char *label;
	uint16_t registers[4];
	int cmd;

} input_t; 

input_t inputs[] =
{
    { .identifier = "y402", .address = 1, .label = "SV 1NP SCHODY", .cmd = 3},
    { .identifier = "y401", .address = 2, .label = "SV 2NP KUCHYN LINKA", .cmd = 3 },
    { .identifier = "y403", .address = 3, .label = "SV 1NP OBYVAK KRB STR", .cmd = 3 },
    { .identifier = "y404", .address = 4, .label = "SV 1NP OBYVAK KRB KRJ", .cmd = 3 },
    { .identifier = "y405", .address = 5, .label = "SV 2NP LOZNICE KRJ", .cmd = 3 },
    { .identifier = "y406", .address = 6, .label = "SV 2NP LOZNICE STR", .cmd = 3 }
};

// variables
static int run = 1;
struct mosquitto *mosq = NULL;

// prototypes
int getModbusData(void *arg);
void mb_process(modbus_t *ctx, input_t *in);

void on_connect(struct mosquitto *mosq, void *obj, int rc);
void on_publish(struct mosquitto *mosq, void *obj, int mid);
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
void on_disconnect(struct mosquitto *mosq, void *obj, int rc);


int main(void)
{
	pthread_t id1;
	
	printf("App started...\r\n");
	
	mosquitto_lib_init();
	mosq = mosquitto_new("modbus-mqtt-gateway", true, NULL);

	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_disconnect_callback_set(mosq, on_disconnect);
	mosquitto_publish_callback_set(mosq, on_publish);
	mosquitto_message_callback_set(mosq, my_message_callback);

	int rc = mosquitto_connect(mosq, "192.168.0.20", 1883, 120);
	if (rc)
	{
		mosquitto_destroy(mosq);
		return -1;
	}
	
	// Start modbus-rtu-thread
	pthread_create(&id1, NULL, (void *)getModbusData, "RTU");
	

	while (run == 1)
	{
		mosquitto_loop(mosq, -1, 1);
	}



    printf("App close\r\n");
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return 0;
}

void mb_process(modbus_t *ctx, input_t *in)
{
	int connected;
	int rc;

	modbus_set_slave(ctx, in->address);
	connected = modbus_connect(ctx);
 
	if (connected == -1) return;
	else printf("[%04d] connected\n", in->address);
 
	//printf("ID=%s CMD=%d AD=%d\r\n", in->identifier, in->cmd, in->address);

	switch(in->cmd)
	{
		case 1: // DO Read (Coil status read)
			//rc = modbus_read_bits(ctx, 0, 16, tab_reg);
			break;
		case 3: // AO read
			rc = modbus_read_registers(ctx, 0, 4, in->registers);
			break;
		case 4: // AI read
			//rc = modbus_read_input_registers(ctx, 0, nb_data, in->iregisters);
			break;
	}
 
	if (rc == -1)
	{
		fprintf(stderr, "%s\n", modbus_strerror(errno));
		modbus_close(ctx);
		return;
	} 
 
	for (int i=0; i < rc; i++)
	{
		printf("reg[%d]=%d (0x%X)\n", i, in->registers[i], in->registers[i]);
	}
	modbus_close(ctx);

	// raise mqtt message
	// here
}

int getModbusData(void *arg)
{
	char *connType = (char*)arg;
	int slave_id = 0x0001;

	modbus_t *ctx = NULL;
	int rc;

	int inputs_cnt = ((sizeof inputs) / (sizeof inputs[0]));
	fprintf(stderr, "Modbus input modules: %d\n", inputs_cnt);

	if (!strcmp(connType,"RTU"))	// RTU
	{
		printf("getMogbusData>>> RTU\n");
		ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);
	}
	else if(!strcmp(connType, "TCP")) // TCP
	{
		printf("getMogbusData>>> TCP\n");
		//ctx = modbus_new_tcp((char*)Modbus.Client_IP, Modbus.Client_Port);
	}
	else
	{
		printf("getMogbusData>>> ASCII is not achieved\n");  // Режим не реализован
		//ctx = modbus_new_ascii(Modbus.Com_Port, Modbus.Baud_Rate, Modbus.Parity, Modbus.Data_bits, Modbus.Stop_bits);
	}

	if (ctx == NULL)
	{
		printf("Unable to allocate libmodbus context\n");
		return -1;
	}
	
	//modbus_set_debug(ctx, FALSE);
	//modbus_set_debug(ctx, TRUE);

	while (1)
	{
		// Опрос модулей
		
		for(int i=0; i < inputs_cnt; i++)
		{   
			usleep(100 * 1000);
 			mb_process(ctx, &inputs[i]);
		}
	
	}

	modbus_free(ctx);

	return 1;
}

//*****************************?????**************************************************
void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	if (rc)
	{
		printf("connect mqtt error!!\n");
		exit(1);
	}
	else
	{
		mosquitto_subscribe(mosq, NULL, "/test", 0);
		mosquitto_subscribe(mosq, NULL, "/cmd", 0);
		//mosquitto_publish(mosq, &sent_mid, "LD/TTTTTT/P/A", strlen("message"), "message", 0, false); // send test message
	}
}

void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
	printf("mqtt msg sent successfully!!\n");
}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	if (strcmp(msg->topic, "/test") == 0)
	{
		printf("test = %d\n", atoi(msg->payload));
	}
	if (strcmp(msg->topic, "/cmd") == 0)
	{
		printf("test = %s\n", (char*)msg->payload);
		if (strcmp((char*)msg->payload, "exit") == 0) 
		{
			printf("exit command recv\n");
			run = 0;
		}	
	}
	else
	{
		//do nothing
	}
	//mosquitto_disconnect(mosq); 
	//printf("%s\n",msg->topic);???????

}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
	run = 0;
}
//*****************************???*****************************************************

//https://jpa.kapsi.fi/nanopb/
//https://github.com/acg/lwpb