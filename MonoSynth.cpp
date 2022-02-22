#include "daisy_pod.h"
#include "daisysp.h"
#include <stdio.h>
#include <string.h>

// Interleaved audio
#define LEFT (i)
#define RIGHT (i + 1)

using namespace daisy;
using namespace daisysp;

static DaisyPod pod;
static Oscillator osc;
static MoogLadder flt;
static AdEnv ad;
static Adsr adsr;
static Metro      tick;
static Parameter cutoffParam, resParam;
static ReverbSc verb;

bool gate;
int wave, mode;
float oscFreq, attack, release, cutoff, fltRes;
float oldk1, oldk2, k1, k2;
bool selfCycle;

void ConditionalParameter(float oldVal,
                          float newVal,
                          float &param,
                          float update);

void Controls();

void NextSamples(float &sig)
{
    float adsr_out = adsr.Process(gate);
    osc.SetAmp(adsr_out);
    //float ad_out = ad.Process();
    sig = osc.Process();
    //sig *= ad_out;
    sig = flt.Process(sig);
}

void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t size)
{
    Controls();

    float sig;

    for (size_t i = 0; i < size; i += 2)
    {

        NextSamples(sig);

        // Add reverb
        verb.Process(sig, sig, &out[LEFT], &out[RIGHT]);

    }
}

// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent midiEvent)
{
    switch (midiEvent.type)
    {
    case NoteOn:
    {
        gate = true;
        NoteOnEvent noteOnEvent = midiEvent.AsNoteOn();
        osc.SetFreq(mtof(noteOnEvent.note));
        osc.SetAmp((noteOnEvent.velocity / 127.0f));
        // ad.Trigger();
        break;
    }
    case NoteOff:
    {
        gate = false;
        // NoteOffEvent notOffEvent = midiEvent.AsNoteOff();
        break;
    }
    default:
        break;
    }
}

// Main -- Init, and Midi Handling
int main(void)
{
    // Init
    float samplerate;
    oldk1 = oldk2 = 0;
    k1 = k2 = 0;
    attack    = .01f;
    release   = .2f;
    cutoff    = 10000;
    mode = 0;

    pod.Init();
    pod.SetAudioBlockSize(4);
    samplerate = pod.AudioSampleRate();
    pod.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);
    System::Delay(250);
    adsr.Init(samplerate);
    osc.Init(samplerate);
    ad.Init(samplerate);
    flt.Init(samplerate);

    // Synthesis
    samplerate = pod.AudioSampleRate();

    // Osc
    wave = osc.WAVE_SIN;
    osc.SetWaveform(wave);
    osc.SetAmp(0.25);

    //Set ADSR envelope parameters
    adsr.SetTime(ADSR_SEG_ATTACK, .1);
    adsr.SetTime(ADSR_SEG_DECAY, .1);
    adsr.SetTime(ADSR_SEG_RELEASE, .01);
    adsr.SetSustainLevel(.25);

    // Set AD envelope parameters
    ad.SetTime(ADENV_SEG_ATTACK, 0.1);
    ad.SetTime(ADENV_SEG_DECAY, 0.1);
    ad.SetMax(1);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    // Ladder filter parameters
    flt.SetFreq(10000);
    flt.SetRes(0.5);

    // Reverb parameters
    verb.Init(samplerate);
    verb.SetFeedback(0.9f);
    verb.SetLpFreq(18000.0f);

    // Parameter parameters
    cutoffParam.Init(pod.knob1, 100, 20000, cutoffParam.LOGARITHMIC);
    resParam.Init(pod.knob2, 0.01, 1, resParam.LOGARITHMIC);

    // Start stuff.
    pod.StartAdc();
    pod.StartAudio(AudioCallback);
    pod.midi.StartReceive();

    // Execute
    while(1) {
         pod.midi.Listen();
        // Handle MIDI Events
        while (pod.midi.HasEvents())
        {
            HandleMidiMessage(pod.midi.PopEvent());
        }
    }
}

/// Updates values if knob had changed
void ConditionalParameter(float oldVal,
                          float newVal,
                          float &param,
                          float update)
{
    if (abs(oldVal - newVal) > 0.00005)
    {
        param = update;
    }
}

// Controls Helpers
void UpdateEncoder()
{
    wave += pod.encoder.RisingEdge();
    wave %= osc.WAVE_POLYBLEP_TRI;
    osc.SetWaveform(wave);

    mode += pod.encoder.Increment();
    
    if (mode > 1){
        mode = 1;
    }

    if (mode < 0){
        mode = 0;
    }

}

void UpdateKnobs()
{
    k1 = pod.knob1.Process();
    k2 = pod.knob2.Process();

    switch(mode)
    {
        case 0:
            ConditionalParameter(oldk1, k1, cutoff, cutoffParam.Process());
            ConditionalParameter(oldk2, k2, fltRes, resParam.Process());
            flt.SetFreq(cutoff);
            flt.SetRes(fltRes);
            break;
        case 1:
            ConditionalParameter(oldk1, k1, attack, pod.knob1.Process());
            ConditionalParameter(oldk2, k2, release, pod.knob2.Process());
            ad.SetTime(ADENV_SEG_ATTACK, attack);
            ad.SetTime(ADENV_SEG_DECAY, release);
            break;
        default: break;
    }
}

void UpdateLeds()
{
    pod.led1.Set(mode == 2, mode == 1, mode == 0);

    switch(wave){
        case osc.WAVE_SIN:
            pod.led2.Set(0, 0, 1);//blue
            break;
        case osc.WAVE_TRI:
            pod.led2.Set(1, 1, 0);//yellow
            break;
        case osc.WAVE_SAW:
            pod.led2.Set(1, 0, 1);//pink
            break;
        case osc.WAVE_RAMP:
            pod.led2.Set(1, 0, 0);//red
            break;
        case osc.WAVE_SQUARE:
            pod.led2.Set(0, 1, 0);//green
            break;
        default:
            pod.led2.Set(1, 0, 1);
            break;
    }

    oldk1 = k1;
    oldk2 = k2;

    pod.UpdateLeds();
}


void Controls()
{
    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();

    UpdateEncoder();

    UpdateKnobs();

    UpdateLeds();

}
