# Cyberpunk 2077 Vehicle Manager

Handles loading vehicles into the main vehicle_list and (eventually) into the player's garage.

Requires:

* RED4ext
* TweakDBext

To use, place your vehicle's .bin file into `r6/tweakdbs/vehicles` with the filename that matches the record, for example:

    r6/tweakdbs/vehicles/
    - Vehicle.v_sport1_docworks_arcadia.bin

An optional configuration file can also be placed at `r6/vehicle_manager.yaml`, with the option to load the vehicle into the garage (which doesn't do anything currently), with this format:

```yaml
vehicles:
  Vehicle.v_sport1_docworks_arcadia:
    addToGarage: true
```

`addToGarage` is the only available option right now.

The configuration file is optional, as all files in the `vehicles` folder will be added to the list.

`r6/tweakdbs/vehicle_manager/vehicle_list.bin` is created as a result of the merging process, to be read by TweakDBext.