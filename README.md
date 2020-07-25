# Met433
a small utility running at Raspi to receive reports from misc 433MHz weather sensors for further examination with
prometheus/grafana

# Motivation
Even if I thought there is already something out there, I just need to take, I could not find something
that fully satisfies my requirements. Maybe there are others out there that encounter the same issue
and want to contribute here.

I have been inspired a lot by Ray's ["Reverse Engineer Wireless Temperature Humidity Rain Sensors"](https://rayshobby.net/reverse-engineer-wireless-temperature-humidity-rain-sensors-part-1/) - all credits to Ray here. There is a 
brilliant forum thread for arduino (in German) explaining the MetSensor's 433Mhz interface quite
well ["Tchibo Wetterstation 433 MHz - Dekodierung mal ganz einfach"](https://forum.arduino.cc/index.php?topic=136836.0)

# Basic System
In my case I still have an old raspi (B) which is not used, but should be good enough to serve the
desired purpose. So I'll do:
* setup raspbian OS (it is a debian buster based 10.4) "RASPBERRY PI OS LITE (32-BIT)"
* add an empty file "ssh" to the boot partition to enable ssh
* setup a static IP address via /etc/dhcpcd.conf in my case:
````
	interface eth0
	static ip_address=10.0.0.10/24
	static routers=10.0.0.138
    static domain_name_servers=10.0.0.138
````

* setup WiringPi (PI OS LITE comes without wiringpi)
````
    sudo apt-get install wiringpi
````
* check if WiringPi has been setup successfully
````
    gpio -v
````
    which replies with board version and if your Raspi supports user-level GPIO access.

# Hardware

to be populated, explain circuit diagram, RXB6 superhet receiver in comparision with ....

# Software
