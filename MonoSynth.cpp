#include "daisy_pod.h"
#include "daisysp.h"
#include <stdio.h>
#include <string.h>

using namespace daisy;
using namespace daisysp;

static DaisyPod   pod;
static Oscillator osc, lfo;
static MoogLadder flt;
static AdEnv      ad;
static Parameter  pitchParam, cutoffParam, lfoParam;

int   wave, mode;
float vibrato, oscFreq, lfoFreq, lfoAmp, attack, release, cutoff;
float oldk1, oldk2, k1, k2;
bool  selfCycle;

void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update);

void Controls();


void NextSamples(float &sig)
{
    float ad_out = ad.Process();
    sig = osc.Process();
    sig = flt.Process(sig);
    sig *= ad_out;
}


void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
	   Controls();
    
    for(size_t i = 0; i < size; i += 2)
    {
		float sig;
        NextSamples(sig);
        
        // left out
        out[i] = sig;

        // right out
        out[i + 1] = sig;
    }
}

// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent midiEvent)
{
    switch(midiEvent.type)
    {
        case NoteOn:
        {
            NoteOnEvent notOnEvent = midiEvent.AsNoteOn();
            char        buff[512];
            sprintf(buff,
                    "Note Received:\t%d\t%d\t%d\r\n",
                    midiEvent.channel,
                    midiEvent.data[0],
                    midiEvent.data[1]);
            pod.seed.usb_handle.TransmitInternal((uint8_t *)buff, strlen(buff));
            // This is to avoid Max/MSP Note outs for now..
            if(midiEvent.data[1] != 0)
            {
                notOnEvent = midiEvent.AsNoteOn();
                osc.SetFreq(mtof(notOnEvent.note));
                osc.SetAmp((notOnEvent.velocity / 127.0f));
                ad.Trigger();
            }
        }
        break;
        default: break;
    }
}


// Main -- Init, and Midi Handling
int main(void)
{
    // Init
    float samplerate;
    oldk1 = 0;
    k1 = 0;
	cutoff = 10000;

    pod.Init();
    pod.SetAudioBlockSize(4);
    samplerate = pod.AudioSampleRate();
    pod.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);
    System::Delay(250);

    osc.Init(samplerate);
    ad.Init(samplerate);
	flt.Init(samplerate);

    // Synthesis
    samplerate = pod.AudioSampleRate();

    // Osc
    osc.SetWaveform(osc.WAVE_POLYBLEP_TRI);
    wave = osc.WAVE_POLYBLEP_TRI;
    osc.SetAmp(1);

	// Ladder
	flt.SetFreq(10000);
    flt.SetRes(0.8);
    
    

    //Set envelope parameters
    ad.SetTime(ADENV_SEG_ATTACK, 0.01);
    ad.SetTime(ADENV_SEG_DECAY, 0.5);
    ad.SetMax(1);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    //set parameter parameters
    cutoffParam.Init(pod.knob1, 100, 20000, cutoffParam.LOGARITHMIC);
    
	// Start stuff.
    pod.StartAdc();
    pod.StartAudio(AudioCallback);
    pod.midi.StartReceive();
    for(;;)
    {
        pod.midi.Listen();
        // Handle MIDI Events
        while(pod.midi.HasEvents())
        {
            HandleMidiMessage(pod.midi.PopEvent());
        }
    }
}

///Updates values if knob had changed
void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update)
{
    if(abs(oldVal - newVal) > 0.00005)
    {
        param = update;
    }
}



void UpdateKnobs()
{
    k1 = pod.knob1.Process();
	ConditionalParameter(oldk1, k1, cutoff, cutoffParam.Process());
    flt.SetFreq(cutoff);
}





void Controls()
{
    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();
    UpdateKnobs();

}



