# Installation

- Make sure the project root folder is three levels deep in the openframeworks
  root folder, e.g. `openframeworks/apps/myApps/NeuronMIDIApp`

- checkout the [ofxMidi addon](https://github.com/danomatika/ofxMidi)
  in `openframeworks/addons/ofxMidi`
    + check the project page and issues section to make sure it's updated
      for the OpenFrameworks version you are using
    + if not: checkout forks section

- install ZMQ and link header files

``` sh
sudo [apt/dnf] install cppzmq-devel
ln -s /usr/include/zmq.h 3rdparty/include
ln -s /usr/include/zmq.hpp 3rdparty/include
ln -s /usr/include/zmq_utils.h 3rdparty/include
```
    + change `config.cmake` to point to correct zmq library

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

--------------------------------------------------------------------------------

# Usage

## OpenFrameworks

To get started with the OpenFrameworks examples see the included [`docs` folder](https://github.com/openframeworks/openFrameworks/blob/master/docs/table_of_contents.md)
in the root directory.

## Neuron MIDI Control

To run the example, first start a NEURON simulation that publishes variable
samples to a ZMQ socket:

```sh
python test_observe_vars.py
```

Then start the NeuroConductor app to listen to the sample stream and
plot it using OpenFrameworks:

```sh
./bin/NeuronMIDIApp -c myconfig.toml
```
