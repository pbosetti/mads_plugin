# How to modify the configuration MQTT inside the App Edge
1. Pass the Edge online
   - On-boarding DHCP (put the flash drive with the file for on-boarding) 
   - Change the cable for the one connected to the network
   - Verify Asset Manager : it should be showing "On-boarded" and "On-line"
2. Go to AAp Management
   - Sinumerik edge MQTT client
   - In the gear sign (Parameters), go to the file of configuration and change it
   - the IP is 10.100.15.55 
   - Press the button play >
   - Go to the JOB window (Configure) - It should be "Configured OK"
3. Pass to local (the inverse of the stage 1)
4. Certificate of the broker in the app MQTT
