# MonoSynth

## Author

Gregory Buchenberger

## Description

This is a simple mono-synth firmware written for the [Daisy Pod](https://www.electro-smith.com/daisy/pod) development board. It's mostly a remix of the code examples provided by Electrosmith to help me learn the process. It is a work in progress.

## Features
* One waveform woo!
* One ladder filter with frequecy controlled by pot 1
* One AR envelop that is fixed

Basically you can play a midi note and tweak the filter frequency. So pretty deep.

## Development

This firmware was developed on an Apple Mac M1 Big Sur using VSCode with the following extensions installed.

* [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) needed for programming the Daisy ARM chip
* [C/C++ Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) for IntelliSence and debugging
* [Task Buttons](https://marketplace.visualstudio.com/items?itemName=spencerwmiles.vscode-task-buttons) because I'm lazy

