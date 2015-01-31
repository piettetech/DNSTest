/*
 *  DNSTEST.ino
 *
 *  Written October 2014
 *  Scott Piette (Piette Technologies, LTD)
 *  Copyright (c) 2014, 2015 Piette Technologies, LTD
 *
 *  Updated January 2015
 *      Added display of RSSI when WiFi is on
 *      Fixed bCloudConnect flag when WiFi is disconnected or off
 *      Enabled use of connecting to cloud after WiFi is on
 *      Implemented way to fix DNS if CC3000 is bad
 *
 *  License:  GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 *  Application:  DNSTEST.ino
 *
 *  This application is designed to test the network settings on the Spark
 *  It prompts the user for options allowing the user to control the network connection
 *
 *  Options are -
 *          [0] Print IP/DNS settings
 *          [1] WiFi Enable / Disable
 *          [2] WiFi Clear Credentials
 *          [3] WiFi Set Credentials
 *          [4] WiFi Connect / Disconnect
 *          [5] Set WiFi to DHCP
 *          [6] Set WiFi to static IP/DNS
 *          [7] Connect / Disconnect Cloud
 *          [8] Test DNS lookup
 *          [9] Fix DNS
 *
 *
 *   NOTE:  When you use Reset IP it will force the IP addresses set below into the Spark
 *          You should change the USER settings below to reflect your specific environment
 */

#include <application.h>

SYSTEM_MODE(MANUAL)

// The following define will enable additional options
// not generally needed but I wanted to keep them in there for future use
//#define EXPERT_MODE

// User defined information  ## THESE ARE FOR YOU TO CHANGE ##
#define USER_WIFI_SSID         "Your_SSID"
#define USER_WIFI_PASSWORD     "Your_SSID_PASSWORD"
#define USER_IPADDRESS         192, 168, 1, 104
#define USER_NETMASK           255, 255, 255, 0
#define USER_GATEWAY           192, 168, 1, 1
#define USER_DNS               192, 168, 1, 1

// Global variables
bool bWiFiEnable;
bool bWiFiConnect;
bool bCloudConnect;
bool bGoodDNS;
uint8_t badDNS[] = { 0, 0, 83, 76};   // the TI chip bad DNS
char _buf[20];
char _strMacAddress[18];

char *MacAddress2Str(byte *_mac, bool bReverse = false)
{
	if (bReverse)
		// if the mac is from WiFi.macAddress use this
		sprintf(_strMacAddress, "%02x:%02x:%02x:%02x:%02x:%02x",
            _mac[5], _mac[4], _mac[3], _mac[2], _mac[1], _mac[0]);
	else
        // if the mac is from nvmem_get_mac_address use this
		sprintf(_strMacAddress, "%02x:%02x:%02x:%02x:%02x:%02x",
            _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
	return _strMacAddress;
}

void printIP(byte *_addr) {
	sprintf(_strMacAddress, "%i.%i.%i.%i", _addr[3], _addr[2], _addr[1], _addr[0]);
    if (!memcmp(badDNS, _addr, sizeof(badDNS)))
    	sprintf(_buf,"%-15s * |", _strMacAddress);
    else
    	sprintf(_buf,"%-17s |", _strMacAddress);
	Serial.println(_buf);
}

void printIPAddr(IPAddress _addr) {
	sprintf(_strMacAddress, "%i.%i.%i.%i", _addr[0], _addr[1], _addr[2], _addr[3]);
	sprintf(_buf,"%-17s |", _strMacAddress);
	Serial.println(_buf);
}

/*
 *  Helper that takes an IP address and turns it into a 32 bit int
 */
uint32_t IP2U32(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  uint32_t ip = a;
  ip <<= 8;
  ip |= b;
  ip <<= 8;
  ip |= c;
  ip <<= 8;
  ip |= d;

  return ip;
}

bool setStaticIPAddress(uint32_t ip, uint32_t subnetMask, uint32_t defaultGateway, uint32_t dnsServer)
{
    // Reverse order of bytes in parameters so IP2U32 packed values can be used with the netapp_dhcp function.
    ip = (ip >> 24) | ((ip >> 8) & 0x0000FF00L) | ((ip << 8) & 0x00FF0000L) | (ip << 24);
    subnetMask = (subnetMask >> 24) | ((subnetMask >> 8) & 0x0000FF00L) | ((subnetMask << 8) & 0x00FF0000L) | (subnetMask << 24);
    defaultGateway = (defaultGateway >> 24) | ((defaultGateway >> 8) & 0x0000FF00L) | ((defaultGateway << 8) & 0x00FF0000L) | (defaultGateway << 24);
    dnsServer = (dnsServer >> 24) | ((dnsServer >> 8) & 0x0000FF00L) | ((dnsServer << 8) & 0x00FF0000L) | (dnsServer << 24);

    // Update DHCP state with specified values.
    if (netapp_dhcp(&ip, &subnetMask, &defaultGateway, &dnsServer) != 0) {
      return false;
    }

    // Reset CC3000 to use modified setting.
    wlan_stop();
    delay(200);
    wlan_start(0);
    return true;
}

/**************************************************************************/
/*!
    @brief    Set the CC3000 to use request an IP and network configuration
              using DHCP.  Note that this DHCP state will be saved in the
              CC3000's non-volatile storage and reused on next reconnect.
              This means you only need to call this once and the CC3000 will
              remember the setting forever.  To switch to use a static IP,
              call the cc3000.setStaticIPAddress function.

    @returns  False if an error occurred, true if successfully set.
*/
/**************************************************************************/
bool setDHCP()
{
    return setStaticIPAddress(0,0,0,0);
}

void setDNSIP() {
/*   Optional: Set a static IP address instead of using DHCP.
     Note that the setStaticIPAddress function will save its state
     in the CC3000's internal non-volatile memory and the details
     will be used the next time the CC3000 connects to a network.
     This means you only need to call the function once and the
     CC3000 will remember the connection details.  To switch back
     to using DHCP, call the setDHCP() function (again only needs
     to be called once).
*/
    uint32_t ipAddress = IP2U32(USER_IPADDRESS);
    uint32_t netMask = IP2U32(USER_NETMASK);
    uint32_t defaultGateway = IP2U32(USER_GATEWAY);
    uint32_t dns = IP2U32(USER_DNS);
    if (!setStaticIPAddress(ipAddress, netMask, defaultGateway, dns)) {
        Serial.println("Failed to set static IP!");
        while(1);
    }
}

#define MAX_CONNECT_RETRY 5  // Used by the test function for the number of attempts to connect to the test server
#define MAX_WIFI_RETRY 100  // Used by fixDNS for the number of times in the loop to wait for the WiFi to complete

// CAUTION, This function is aggressive and may damage the hardware
// use at your own risk, it is disabled by default
void fixDNS() {
    // start with the WiFi connected to the AP
	// check if we have a bad DNS address 76.83.0.0
	// make sure the CC3000 knows we want to use DHCP
	// disconnect from the AP
	// make sure the CC3000 knows we want to use DHCP
	// reconnect to the AP
	// go to the beginning and if we still have a bad DNS IP then do it all over again.

	// the bad DNS ip address
	uint8_t badDNS[] = { 0, 0, 83, 76};   // the TI chip bad DNS ip
	bool bGoodDNS = false;
    uint8_t _timeout;
    uint8_t _resetTimeout = 5;


    while (!bGoodDNS && _resetTimeout-- > 0) {
    	Serial.print("Attempt #");
    	Serial.println(5-_resetTimeout);
    	// start with the WiFi connected to the AP
        if (!bWiFiConnect) {
		    Serial.println("Connecting to AP");
		    WiFi.connect();
		    Serial.print("Waiting for WiFi to connect ");
		    _timeout = 0;
		    while (_timeout++ < MAX_WIFI_RETRY && !WiFi.ready()) { Serial.print("+"); Spark.process(); delay(100);}
            while (ip_config.aucDNSServer[3] == 0 && ip_config.aucDNSServer[2] == 0) { Serial.print("-"); Spark.process(); delay(100); }
		    _timeout = 0;
            while (_timeout++ < (MAX_WIFI_RETRY/2) &&  ip_config.aucDNSServer[3] == 76 && ip_config.aucDNSServer[2] == 83) { Serial.print("*"); Spark.process(); delay(100); }
            if (_timeout == MAX_WIFI_RETRY) {
                Serial.print("Failed to connect to WiFi router - credentials ");
                Serial.println(WiFi.hasCredentials() ? "known" : "unknown");
                bGoodDNS = true;  // bail!!
            }
            else Serial.println();
            bWiFiConnect = WiFi.ready();
        }

        // check if we have a bad DNS address 76.83.0.0
	    if (!memcmp(badDNS, ip_config.aucDNSServer, sizeof(badDNS))) {
            // make sure the CC3000 knows we want to use DHCP
            Serial.println("Resetting CC3000 to DHCP while connected");
            setStaticIPAddress(0,0,0,0);
            delay(500);

            // disconnect from the AP
            Serial.println("Disconnecting from AP");
            WiFi.disconnect();
            Serial.print("Waiting for WiFi to disconnect ");
            _timeout = 0;
            while (_timeout < MAX_WIFI_RETRY && WiFi.ready()) { Serial.print("-"); Spark.process(); _timeout++; delay(100); }
            while (ip_config.aucDNSServer[3] != 0 || ip_config.aucDNSServer[2] != 0) { Serial.print("-"); Spark.process(); delay(100); }
            Serial.println();
            bWiFiConnect = false;
            bCloudConnect = false;

            // make sure the CC3000 knows we want to use DHCP
            Serial.println("Resetting CC3000 to DHCP while not connected");
            setStaticIPAddress(0,0,0,0);
            delay(500);
        } else {
		    bGoodDNS = true;
            Serial.println("Success!");
        }
	}
}

void resetDNS() {
    Serial.println("Resetting DNS IP");
    setDNSIP();
}

void dumpIP() {
	Serial.print("\r\n");
    Serial.println("+---------------------------------+");
    Serial.println("|        DNS Test App v2.0        |");
    Serial.println("|    Scott Piette Jan 14, 2015    |");

    Serial.println("+-------------+-------------------+");
	Serial.print("| CC30000     | ");
	Serial.println(bWiFiEnable?"On                |":"Off               |");
    if (bWiFiEnable) {
        uint8_t mac[6];
    	Serial.print("|   Mac Addr  | ");
        if (bWiFiConnect)
            WiFi.macAddress(mac);  // use this function if the CC3000 is on and WiFi connected
        else
        	nvmem_get_mac_address(mac); // use this function only the CC3000 is on
        Serial.print(MacAddress2Str(mac, bWiFiConnect));
        Serial.println(" |");
	}
    Serial.println("|-------------+-------------------|");
	  Serial.print("| WiFi        | ");
	Serial.println(bWiFiConnect?   "Connected         |":"Not Connected     |");
    if (bWiFiConnect) {
		Serial.print("|   SSID      | ");
		sprintf(_buf,"%-17s |", WiFi.SSID());
		Serial.println(_buf);
    	Serial.print("|   RSSI      | ");
		sprintf(_buf,"%-17i |", WiFi.RSSI());
		Serial.println(_buf);
        Serial.print("|   Local IP  | ");
        printIPAddr(WiFi.localIP());
    }
	if (bWiFiConnect) {
		Serial.print("|   Subnet    | ");
		printIPAddr(WiFi.subnetMask());
		Serial.print("|   Gateway   | ");
		printIPAddr(WiFi.gatewayIP());
	    Serial.print("|   DNS IP    | ");
		printIP(ip_config.aucDNSServer);
	}
    Serial.println("|-------------+-------------------|");
	  Serial.print("| Spark Cloud | ");
	Serial.println(bCloudConnect?"Connected         |":"Not Connected     |");
    Serial.println("+-------------+-------------------+");
}

#define MAX_CONNECT_TIMEOUT 100  // How many times in while loop to wait for connection
int connectHttpServerHost(char *_host) {
    int  _ret;
    int _timeout;
    TCPClient _client;
    int8_t _c;
    uint8_t _message[] = { 'G', 'E', 'T', '\n', '\n' };

    //Serial.println("Starting client.connect");
    if(_client.connect(_host, 80)) {
	    //Serial.println("After client.connect()");
	    _timeout = 0;
	    while ( _timeout < MAX_CONNECT_TIMEOUT && !_client.connected()) {
	        //if (!_timeout) Serial.print("Waiting for connect ");
	        //else Serial.print(".");
	        delay(10);
	        ++_timeout;
	    }
	    //if (_timeout) Serial.println("");
	    //Serial.print("Sending");
	    _client.write(_message, 5);
	    _timeout = 0;
	    while (_timeout < MAX_CONNECT_TIMEOUT && !(_ret = _client.available())) {
	        //if (!_timeout) Serial.print("\r\nWaiting for response");
	        //else Serial.print(".");
	        delay(100);
	        ++_timeout;
	    }
	    //if (_ret) {
	    //    Serial.print("\r\nResponse Length = ");
	    //    Serial.println(_ret);
	    //    while ((_c = _client.read()) != -1)
		//    Serial.print((char) _c);
	    //}
	    //else
	    //    Serial.println("\r\nNo Response.");
	    _client.flush();
	    _client.stop();
	    return 1;
    }
    return 0;
}

void setup() {
    Serial.begin(9600);
    while(!Serial.available()) {
        Serial.println("Press any key to begin");
        delay(1000);
    }
    Serial.read();  // Clear any key
    dumpIP();
}

void loop() {

    byte _ans;
    int _ret;
    int _timeout = 0;

    Serial.println("\r\nEnter");
    Serial.print("   [0] Print interface status\r\n");
    Serial.print("   [1] CC3000 ");
    Serial.print(bWiFiEnable?"Disable":"Enable");
    Serial.println();
    if (bWiFiEnable) {
        Serial.print("   [2] WiFi ");
        Serial.println(bWiFiConnect?"Disconnect":"Connect");
#if defined(EXPERT_MODE)
        Serial.print("   [3] WiFi Clear Credentials\r\n");
        Serial.print("   [4] WiFi Set Credentials\r\n");
        Serial.print("   [5] Set WiFi to DHCP\r\n");
        Serial.print("   [6] Set WiFi to static IP/DNS\r\n");
    }
    Serial.print("   [7] Cloud ");
    Serial.print(bCloudConnect?"Disconnect":"Connect");
    Serial.println();
    if (bWiFiConnect) {
        Serial.print("   [8] Test DNS lookup\r\n");
    	if (!memcmp(badDNS, ip_config.aucDNSServer, sizeof(badDNS)))
            Serial.print("   [9] Fix DNS\r\n");
#else
	}
    Serial.print("   [3] Cloud ");
    Serial.print(bCloudConnect?"Disconnect":"Connect");
    Serial.println();
    if (bWiFiConnect) {
        Serial.print("   [4] Test DNS lookup\r\n");
        if (!memcmp(badDNS, ip_config.aucDNSServer, sizeof(badDNS)))
            Serial.print("   [5] Fix DNS\r\n");
#endif
    }

    Serial.print("   --> ");
    while(!Serial.available()) { if (WiFi.ready()) Spark.process(); };
    _ans = Serial.read();
    switch (_ans) {
        case '0':
            Serial.println("Print interface status");
            dumpIP();
            break;
        case '1':
            Serial.print("CC3000 ");
            Serial.print(bWiFiEnable?" Disable":" Enable");
            Serial.print("\r\n\r\nCC3000 - ");
            if (bWiFiEnable) {
                Serial.println("disabling");
            	if (bWiFiConnect) {
                    WiFi.disconnect();
                    Serial.print("Waiting for WiFi to disconnect ");
                    while (_timeout < MAX_WIFI_RETRY && WiFi.ready()) { Serial.print("-"); _timeout++; delay(100); }
                    while (ip_config.aucDNSServer[3] != 0 || ip_config.aucDNSServer[2] != 0)
                        { Serial.print("-"); delay(100); }
                    bCloudConnect = false;
                    Serial.println();
            	}
                WiFi.off();
                Serial.print("Waiting for CC3000 to turn off ");
                while (WiFi.ready()) { Serial.print("-"); Spark.process(); delay(100); }
                while (ip_config.aucDNSServer[3] != 0 || ip_config.aucDNSServer[2] != 0)
                    { Serial.print("+"); delay(100); }
                bWiFiEnable = false;
                bWiFiConnect = false;
                bCloudConnect = false;
            }
            else {
                Serial.println("enabling");
                WiFi.on();
                bWiFiEnable = true;
            }
            delay(500);
            dumpIP();
            break;
        case '2':
            Serial.print("WiFi ");
            Serial.print(bWiFiConnect?"Disconnect":"Connect");
            Serial.print("\r\n\r\nWiFi - ");
            if (bWiFiEnable) {
                if (bWiFiConnect) {
                    Serial.println("disconnecting");
                    WiFi.disconnect();
                    Serial.print("Waiting for WiFi to disconnect ");
                    while (_timeout < MAX_WIFI_RETRY && WiFi.ready()) { Serial.print("-"); _timeout++; delay(100); }
                    while (ip_config.aucDNSServer[3] != 0 || ip_config.aucDNSServer[2] != 0)
                        { Serial.print("-"); delay(100); }
                    bCloudConnect = false;
                    Serial.println();
                }
                else {
                    Serial.println("connecting");
                    WiFi.connect();
                    Serial.print("Waiting for WiFi to connect ");
                    while (_timeout < MAX_WIFI_RETRY && !WiFi.ready()) { Serial.print("+"); _timeout++; delay(100);}
                    if (_timeout == MAX_WIFI_RETRY) {
                        Serial.print("Failed to connect to WiFi router - credentials ");
                        Serial.println(WiFi.hasCredentials() ? "known" : "unknown");
                    }
                    else Serial.println();
                    _timeout = 0;
                }
                bWiFiConnect = WiFi.ready();
                delay(500);
                dumpIP();
            } else
                Serial.println("Error - CC3000 must be enabled");
            break;
#if defined(EXPERT_MODE)
        case '3':
            if (bWiFiEnable) {
                Serial.println("Clearing WiFi Credentials");
                WiFi.clearCredentials();
                dumpIP();
            } else
                Serial.println("Error - CC3000 must be enabled");
            break;
        case '4':
            if (bWiFiEnable) {
                Serial.println("Setting WiFi Credentials");
                WiFi.setCredentials(USER_WIFI_SSID, USER_WIFI_PASSWORD);
                dumpIP();
            } else
                Serial.println("Error - CC3000 must be enabled");
            break;
        case '5':
            if (bWiFiEnable) {
                Serial.println("Setting WiFi to DHCP\r\n");
                setDHCP();
                delay(500);
                dumpIP();
            } else
                Serial.println("Error - CC3000 must be enabled");
            break;
        case '6':
            if (bWiFiEnable) {
                Serial.println("Setting WiFi to static IP/DNS\r\n");
                setDNSIP();
                delay(500);
                dumpIP();
            } else
                Serial.println("Error - CC3000 must be enabled");
            break;
        case '7':
#else
        case '3':
#endif
        	Serial.print("Cloud ");
            Serial.print(bCloudConnect?"Disconnect":"Connect");
            Serial.print("\r\n\r\nCloud - ");
            if (bCloudConnect) {
                Serial.print("disconnecting -");
                Spark.disconnect();
                _timeout = 0;
                while (_timeout < MAX_WIFI_RETRY && Spark.connected()) { Serial.print("-"); _timeout++; delay(100);}
                if (_timeout == MAX_WIFI_RETRY) {
                    Serial.println("Failed to disconnect from cloud");
                }
                else Serial.println();
            } else {
            	Serial.print("connecting +");
            	Spark.connect();
            	_timeout = 0;
            	while (_timeout < MAX_WIFI_RETRY && !Spark.connected()) { Serial.print("+"); _timeout++; delay(100);}
            	if (_timeout == MAX_WIFI_RETRY) {
            		Serial.println("Failed to connect to Spark Cloud.");
            	}
                else {
                	Serial.println();
                	bWiFiEnable = true;
                	bWiFiConnect = WiFi.ready();
                }
            }
            bCloudConnect = Spark.connected();
            dumpIP();
            break;
#if defined(EXPERT_MODE)
        case '8':
#else
        case '4':
#endif
            Serial.println("Test DNS\r\n\r\nConnect to data.sparkfun.com - ");
            if (bWiFiEnable && bWiFiConnect) {
                while (_timeout < MAX_CONNECT_RETRY && !(_ret = connectHttpServerHost("data.sparkfun.com")))  {
                    delay(100);
                    ++_timeout;
                }
                if (_ret)
                    Serial.print("Connect successful! ");
                else
                    Serial.print("Connect unsuccessful. ");

                if (_timeout > 0) {
                    Serial.print("[ ");
                    Serial.print(_timeout);
                    if (_timeout > 1)
                        Serial.print("retries]");
                    else
                        Serial.print("retry]");
                }
                Serial.println();
            } else
                Serial.println("Error - CC3000 must be enabled and WiFi connected");
            break;
#if defined(EXPERT_MODE)
        case '9':
#else
        case '5':
#endif
            Serial.println("Fix DNS\r\n\r\nFixing DNS IP -");
            if (bWiFiConnect) {
                if (!memcmp(badDNS, ip_config.aucDNSServer, sizeof(badDNS))) {
                    fixDNS();
            	    dumpIP();
                }
                else
                    Serial.println("Error - CC3000 DNS must be 76.83.0.0");
            }
            else
                Serial.println("Error - CC3000 must be enabled and WiFi connected");
            break;
        default:
            Serial.println("Invalid selection.");
            break;
    }

}
