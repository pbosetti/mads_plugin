# Availabe plugins

## Templates

The available templates are:

* `template_source.cpp` for implementing source plugins
* `template_sink.cpp` for implementing sink plugins
* `template_filter.cpp` for implementing filter plugins

Use these files as a starting point for your own plugins.

## Example plugins

The followings are example plugins of little practucalu use, but they are good for learning the basics of plugin development.

* `clock`: a source that provides clock values
* `echoj`: a filter that echoes the input message
* `random`: a source that provides random values
* `to_console`: a sink that prints the input message to the console
* `twice`: a filter that duplicates the input message


# Plugins of actual use

## MQTT

This acts as a source plugin, which reads messages from an MQTT broker and sends them to the MADS broker.

Note that the plugin **only subscribes to the topic as specified in the configuration file** and does not publish any messages.

The **frequency** of messages depends on when they are received from the MQTT broker. The plugin will send the messages to the MADS broker as soon as they are received.

### Parameters

The accepted parameters are:

```ini
[mqtt]
broker_host = "localhost"
broker_port = 1883
topic = "capture/#"
```

### Notes

The MQTT broker must be running on the same address that has been set into the Siemens MindSphere settings. The root publishing topic (e.g. `capture`) is defined in the settings of the Siemens MindSphere application, while each acquisition procedure defined in the Edge internal webapp will append a unique identifier to the root topic (e.g. `capture/mads`). So, you typically want to subscribe to `capture/#` to get all the messages.


## Serial Reader

This acts as a source plugin, which reads messages from a serial port (typically connected to an Arduino) and sends them to the MADS broker. 

The **frequency** of messages depends on the configuration of the Arduino board. The plugin will send the messages to the MADS broker as soon as they are received from the Arduino board. In turn, this frequency can be set in the configuration file (`cfg_cmd` parameter).

### Parameters

The accepted parameters are:

```ini
[serial_reader]
port="/dev/ttyACM0"
baudrate=115200
# Arduino config serial command
cfg_cmd = "40p"
```

### Notes

The `cfg_cmd` parameter is used to configure the Arduino board. The default value is `40p`, which sets the Arduino board to send a message every 40 ms.


## Running average

This acts as a filter plugin, which calculates the running average of the input values.


### Parameters

The accepted parameters are:

```ini
[running_avg]
sub_topic = ["serial_reader"]
capa = 10          # running window size
field = "data"     # agerage all values in the dictionary "data"
out_field = "avg"  # output field
```

### Notes

It searches for a field in the message payload having the name specified in the `field` parameter. the data field must be a dictionary. of numerical values, e.g.:

```json
{
  "data": {
    "value1": 1,
    "value2": 2,
    "value3": 3
  }
}
```

The plugin will calculate the average of all the values in the dictionary and store it in a new field, whose name is specified in the `out_field` parameter:

```json
{
  "avg": {
    "value1": 1,
    "value2": 2,
    "value3": 3
  }
}
```