# datalogger
Simple data logger for Adafruit Feather m0 &amp; 9 DOF &amp; ESP8266
Log data from accelerometer&gyro at 10 Hz sample rate. Pull data to telnet client. To use:
- connect to WiFi AP logger
- telnet 192.168.4.1
- to start logging "b <comment>"
- to end logging "s"
- to view battery level "v"
- to store from telnet session to file run "telnet 192.168.4.1 | tee -a myfile.log"
- tested with termux on androiod
