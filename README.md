# DNSTest
DNS Test Application for the Spark Core
Used to help diagnose the TI CC3000 DNS problem using DHCP on WiFi.

dnstest-simple.bin - this firmware has limited options for viewing and enabling the CC3000, WiFi, and Cloud
dnstest-expert.bin - this firmware has more options for controlling the CC3000

Download the .bin file, use DFU-UTIL or Spark flash to push this program to your spark core

The typical behavior is the calls to TCPConnect will fail when using a hostname, but work when using a IP address.

In this case you need to look at what the core set its DNS server to during the setup of DHCP.  I have a few cores that fail to correctly setup DNS and they always get set to 76.83.0.0.

I have written this utility to help diagnose and control the WiFi connection.  CAUTION - Use carefully





All debugging is done via the serial port.  When you launch the app it will start the Spark in manual WiFi mode.  You will be prompted with the following options -

    > Press any key to begin
    > Press any key to begin
    
    > +---------------------------------+
    > |        DNS Test App v2.0        |
    > |    Scott Piette Jan 14, 2015    |
    > +-------------+-------------------+
    > | CC30000     | Off               |
    > |-------------+-------------------|
    > | WiFi        | Not Connected     |
    > |-------------+-------------------|
    > | Spark Cloud | Not Connected     |
    > +-------------+-------------------+
    
    > Enter
    >    [0] Print interface status
    >    [1] CC3000 Enable
    >    [7] Cloud Connect
    >    --> 

You have the option of printing the IP settings, turning the CC3000 on or off or connect to the cloud.  The connect to the cloud will call `Spark.connect()` and that call is fairly automated, it will enable the CC3000, WiFi, connect to the router and finally the spark cloud.

After you have enabled the CC3000 you will see more options -

    CC3000 - enabling
    
    +---------------------------------+
    |        DNS Test App v2.0        |
    |    Scott Piette Jan 14, 2015    |
    +-------------+-------------------+
    | CC30000     | On                |
    |   Mac Addr  | 08:00:28:72:38:26 |
    |-------------+-------------------|
    | WiFi        | Not Connected     |
    |-------------+-------------------|
    | Spark Cloud | Not Connected     |
    +-------------+-------------------+
    
    Enter
       [0] Print interface status
       [1] CC3000 Disable
       [2] WiFi Connect
       [3] WiFi Clear Credentials
       [4] WiFi Set Credentials
       [5] Set WiFi to DHCP
       [6] Set WiFi to static IP/DNS
       [7] Cloud Connect
       --> 


The list of options above is what you will see in expert mode.  You can try connecting to your router, clearing or setting your credentials (remember to put your wifi router settings in the code).   You can also set DHCP or Static IP settings (you need to set your static IP settings in the code) and finally you still have the option of connecting to the cloud from here.  Note your MAC address is now available.

Once you have connected to a WiFi network and have an IP address you have a one more option -

    WiFi - connecting
    Waiting for WiFi to connect +++++
    
    +---------------------------------+
    |        DNS Test App v2.0        |
    |    Scott Piette Jan 14, 2015    |
    +-------------+-------------------+
    | CC30000     | On                |
    |   Mac Addr  | 08:00:28:72:38:26 |
    |-------------+-------------------|
    | WiFi        | Connected         |
    |   SSID      | PietteFiOS        |
    |   RSSI      | -51               |
    |   Local IP  | 192.168.1.105     |
    |   Subnet    | 255.255.255.0     |
    |   Gateway   | 192.168.1.1       |
    |   DNS IP    | 192.168.1.1       |
    |-------------+-------------------|
    | Spark Cloud | Not Connected     |
    +-------------+-------------------+
    
    Enter
       [0] Print interface status
       [1] CC3000 Disable
       [2] WiFi Disconnect
       [3] WiFi Clear Credentials
       [4] WiFi Set Credentials
       [5] Set WiFi to DHCP
       [6] Set WiFi to static IP/DNS
       [7] Cloud Connect
       [8] Test DNS lookup
       --> 

Once you have established a connection to your WiFi router you can now test DNS.  Option [8] will attempt a connection to data.sparkfun.com using DNS to resolve the IP address.  It will try this several times.  If you are unable to get an connection look at your DNS IP.  If it is set to 76.83.0.0 then you have the problem.  If the program detects you have 76.83.0.0 as your DNS IP then you will have another option to try and correct this. 

If your DNS is set to 76.83.0.0 then you should try the latest CC3000 following these instructions from @Dave

    If you use the spark-cli, and place your core in dfu-mode ( https://github.com/spark/spark-cli )
    spark flash --usb cc3000
