# Installation

- Make sure the project root folder is three levels deep in the openframeworks
  root folder, e.g. `openframeworks/apps/myApps/NeuronMIDIApp`

- checkout the [ofxMidi addon](https://github.com/danomatika/ofxMidi) 
  in `openframeworks/addons/ofxMidi`
    + check the project page and issues section to make sure it's updated
      for the OpenFrameworks version you are using

- build the project

```sh
cd openframeworks/apps/myApps/NeuronMIDIApp
make
```

## Install JACK

For routing between audio MIDI sources & sinks. See [this guide](http://tedfelix.com/linux/linux-midi.html) and [this one](https://libremusicproduction.com/articles/demystifying-jack-%E2%80%93-beginners-guide-getting-started-jack) for an explanation of what JACK and ALSA do.

```
sudo apt install qjackctl
```

## Install a MIDI Controller

If you don't have a hardware MIDI controller you can install a software MIDI device.

# Usage

## OpenFrameworks

To get started with the OpenFrameworks examples see the included [`docs` folder](https://github.com/openframeworks/openFrameworks/blob/master/docs/table_of_contents.md)
in the root directory.

## Neuron MIDI Control

```sh
./bin/NeuronMIDIApp -c myconfig.toml
```