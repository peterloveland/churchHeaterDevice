import network
from time import sleep, time
import machine
import urequests

ssid = 'Loveland'
password = 'idontknowthewificode'

# Function to connect to WiFi
def connect():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(ssid, password)
    while not wlan.isconnected():
        print('Waiting for connection...')
        sleep(1)
    print(wlan.ifconfig())
    send_debug_message("Church heater is online and connected")

# Function to send a message to Slack
def send_slack_message(total_time):
    slack_webhook_url = "https://hooks.slack.com/services/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"
    
    message = {
        "text": "Church heater turned on for {} hours.".format(total_time)
    }

    response = urequests.post(slack_webhook_url, json=message)
    print("Response Status Code:", response.status_code)
    response.close()

# Function to send a debug message
def send_debug_message(message):
    slack_webhook_url = "https://hooks.slack.com/services/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"
    
    message = {
        "text": message
    }

    response = urequests.post(slack_webhook_url, json=message)
    print("Response Status Code:", response.status_code)
    response.close()

# Connect to WiFi
try:
    connect()
except KeyboardInterrupt:
    machine.reset()

# Initialize variables
start_time = time()

# store total time variable
total_time = 0

second = 1
min = 60 * second
hour = 60 * min

first_message = 3 * hour
second_message = 3.5 * hour

interval_time = 1 * hour 

debug_time = 0
debug_interval = 1 * min

# send a message to slack after 1 minute, after 2 minutes, and then every 30 seconds after that.
# every time you send a message, add the interval time to the total time
# send this message to slack: "Pi turned on for {total_time} mins."

        # print every 2 seconds
try :
    while True:
        # Calculate the elapsed time in minutes
        elapsed_time = (time() - start_time)

        print("{:.0f}".format(elapsed_time))

        if elapsed_time == first_message:
            print("Church heater has been on for 3 hours")
            total_time = 1
            send_slack_message(total_time)
        elif elapsed_time == second_message:
            print("Church heater has been on for 3.5 hours")
            total_time = 2
            send_slack_message(total_time)
        # once the elapsed time is greater than 2 minutes, print every 30 seconds after that
        elif elapsed_time > second_message:
            if (elapsed_time - second_message) % interval_time == 0:
                total_time += 0.5
                print("Church heater has been on for {} hours".format(total_time))
                send_slack_message(total_time)
        if elapsed_time > 5 and elapsed_time % (debug_interval) == 0: # adding the 5 second delay to make sure the debug message isn't sent immediately on connection
            debug_time += debug_interval
            send_debug_message("Connected for {} minutes".format(debug_time // min))
        sleep(1 * second)

except KeyboardInterrupt:
    machine.reset()
