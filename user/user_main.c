#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"

#include "client_manager.h"
#include "parser.h"
#include "response_parser.h"
#include "uart.h"


os_event_t    user_procTaskQueue[1];
static void user_procTask(os_event_t *events);

LOCAL os_timer_t broadcast_timer;

struct espconn server_conn;
clientListHead *client_list;
esp_tcp server_tcp;
parser *command_parser;
rparser *response_parser;

char update_buffer[64];
char response_buffer[512];
char uart_out_buffer[1024];

void send_broadcast_packet() {
    struct ip_info station_ip;
    int stat = wifi_station_get_connect_status();
    if (stat != STATION_GOT_IP) {
        return;
    }
    bool success = wifi_get_ip_info(STATION_IF, &station_ip);
    if (!wifi_get_ip_info(STATION_IF, &station_ip))
        return;

    // calculate broadcast address
    uint32 broadcastIp = station_ip.netmask.addr ^ 0xFFFFFFFF;
    broadcastIp = station_ip.ip.addr | broadcastIp;

    esp_udp udp_conf;
    udp_conf.remote_port = BROADCAST_PORT;
    udp_conf.local_port = 0;
    os_memcpy(udp_conf.local_ip, &station_ip.ip.addr, 4);
    os_memcpy(udp_conf.remote_ip, &broadcastIp, 4);

    struct espconn conn;
    os_memset(&conn, 0, sizeof(struct espconn));
    conn.type = ESPCONN_UDP;
    conn.state = ESPCONN_NONE;
    conn.proto.udp = &udp_conf;
    if (espconn_create(&conn) != ESPCONN_OK) {
        return;
    }

    os_sprintf(update_buffer, "{ \"id\": %d, \"type\": \"" NODE_TYPE "\" }", system_get_chip_id());

    espconn_send(&conn, update_buffer, os_strlen(update_buffer));
    espconn_delete(&conn);
}

void broadcast_timer_fn(void *arg) {
    send_broadcast_packet();
}

void server_recv_cb(void *arg, char *pdata, unsigned short len) {
    struct espconn *pConn = (struct espconn *)arg;

    int parsedInt;
    char parsedData[512];

    if (len == 0) {
        return;
    }

    // feed message to parser
    parser_feed(command_parser, pdata, len);

    // process it
    parser_result pres = parser_process(command_parser, &parsedInt, parsedData, 512);
    if (pres == PR_SET_COMMAND) {
        pres = parser_process(command_parser, &parsedInt, parsedData, 512);
        if (pres == PR_KEY) {
            if (!os_strcmp("wifi_ssid", parsedData)) {
                char m[128];
                char errmsg[] = "not yet implemented";
                os_sprintf(m, "-%d:%s", strlen(errmsg), errmsg);
                espconn_send(pConn, m, strlen(m));
            }
            else if (!os_strcmp("wifi_pass", parsedData)) {
                char m[128];
                char errmsg[] = "not yet implemented";
                os_sprintf(m, "-%d:%s", strlen(errmsg), errmsg);
                espconn_send(pConn, m, strlen(m));
            }
            else {
                cm_set_active_connection(client_list, pConn);
                os_printf("s%d:%s", os_strlen(parsedData), parsedData);
                pres = parser_process(command_parser, &parsedInt, parsedData, 512);
                if (pres == PR_VALUE_INT) {
                    os_printf("i%de", parsedInt);
                }
                else if (pres == PR_VALUE_STRING) {
                    os_printf("%d:%s", os_strlen(parsedData), parsedData);
                }
                else {
                    // error parsing, cancel command by sending crap value
                    os_printf("x");
                }
            }
        }
    }
    else if (pres == PR_GET_COMMAND) {
        // Message contained 'get' command
        pres = parser_process(command_parser, &parsedInt, parsedData, 512);
        if (pres == PR_KEY) {
            // send command to uart
            cm_set_active_connection(client_list, pConn);
            os_printf("g%d:%s", os_strlen(parsedData), parsedData);
        }
    }

    parser_reset(command_parser);
}

void server_sent_cb(void *arg) {
    struct espconn *pConn = (struct espconn *)arg;
}

void client_connect_cb(void *arg) {
    struct espconn *pConn = (struct espconn *)arg;
    cm_add_connection(client_list, pConn);
}

void client_reconnect_cb(void *arg, sint8 err) {
    struct espconn *pConn = (struct espconn *)arg;
}

void client_disconnect_cb(void *arg) {
    struct espconn *pConn = (struct espconn *)arg;
    bool res = cm_remove_connection(client_list, pConn);
    if (res) {
    }
    else {
        // error, this shouldn't happen
    }
}

ICACHE_FLASH_ATTR
void start_server()
{
    // zero config structs
    os_memset(&server_conn, 0, sizeof(struct espconn));
    os_memset(&server_tcp, 0, sizeof(esp_tcp));

    // configure TCP server connection settings
    server_conn.type = ESPCONN_TCP;
    server_conn.state = ESPCONN_NONE;
    server_conn.proto.tcp = &server_tcp;
    server_tcp.local_port = LISTEN_PORT;

    // register callbacks
    espconn_regist_recvcb(&server_conn, server_recv_cb);
    espconn_regist_sentcb(&server_conn, server_sent_cb);
    espconn_regist_connectcb(&server_conn, client_connect_cb);
    espconn_regist_reconcb(&server_conn, client_reconnect_cb);
    espconn_regist_disconcb(&server_conn, client_disconnect_cb);

    // set the timeout
    espconn_regist_time(&server_conn, 0, 0);

    // set max clients
    espconn_tcp_set_max_con_allow(&server_conn, 4);

    // create TCP listening port
    if (espconn_accept(&server_conn) != ESPCONN_OK) {
        //os_printf("Server failed to start. Error!\n");
        return;
    }

    // set up broadcast timer
    os_timer_disarm(&broadcast_timer);
    os_timer_setfn(&broadcast_timer, &broadcast_timer_fn, 0);
    os_timer_arm(&broadcast_timer, BROADCAST_INTERVAL, 1);

    // send initial broadcast packet
    send_broadcast_packet();
}

void wifi_callback(System_Event_t *evt)
{
    switch (evt->event)
    {
        case EVENT_STAMODE_CONNECTED:
        {
            os_printf("connect to ssid %s, channel %d\n",
                        evt->event_info.connected.ssid,
                        evt->event_info.connected.channel);
            break;
        }

        case EVENT_STAMODE_DISCONNECTED:
        {
            //os_printf("disconnect from ssid %s, reason %d\n",
//                        evt->event_info.disconnected.ssid,
//                        evt->event_info.disconnected.reason);

            deep_sleep_set_option(0);
            system_deep_sleep(60 * 1000 * 1000);  // 60 seconds
            break;
        }

        case EVENT_STAMODE_GOT_IP:
        {
            os_printf("ip=" IPSTR ",mask=" IPSTR ",gw=" IPSTR "\n",
                IP2STR(&evt->event_info.got_ip.ip),
                IP2STR(&evt->event_info.got_ip.mask),
                IP2STR(&evt->event_info.got_ip.gw));
            start_server();
            break;
        }

        default:
        {
            break;
        }
    }
}

ICACHE_FLASH_ATTR
static void user_procTask(os_event_t *events)
{
    // get incoming character
    int c = uart0_rx_one_char();
    if(c == -1) {
        // not actually a character
        return;
    }

    if (rparser_parse_char(response_parser, c) == RPR_READY) {
        char *msg = rparser_get_msg(response_parser);
        clientListNode* cl = cm_get_active_connection(client_list);

        if (cl->esp_conn->state == ESPCONN_CONNECT) {
            espconn_send(cl->esp_conn, msg, os_strlen(msg));
        }
        rparser_reset(response_parser);
    }
}

ICACHE_FLASH_ATTR
void user_init(void)
{
    static struct station_config config;

    // initialize variables
    client_list = cm_create_clientList();
    command_parser = parser_create();
    response_parser = rparser_create();

    // set up baud rate
    //uart_div_modify(0, UART_CLK_FREQ / (115200));
    uart_init(BIT_RATE_57600, BIT_RATE_57600);

    // config gpio
    gpio_init();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);

    // set uart0 as default output for os_printf

    // set up wifi options
    wifi_station_set_hostname("haNode");
    wifi_set_opmode_current(STATION_MODE);

    config.bssid_set = 0;
    os_memcpy( &config.ssid, WIFI_SSID, 32 );
    os_memcpy( &config.password, WIFI_PASS, 64 );
    wifi_station_set_config(&config);

    // set up static ip for demo
    // TODO: remove this eventually
    /*
    wifi_station_dhcpc_stop();
    struct ip_info info;
    IP4_ADDR(&info.ip, 192, 168, 43, 201);
    IP4_ADDR(&info.gw, 192, 168, 43, 1);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    wifi_set_ip_info(STATION_IF, &info);
    */

    wifi_set_event_handler_cb(wifi_callback);

    system_os_task(user_procTask, 0, user_procTaskQueue, 1);
    system_os_post(0, 0, 0);
}

