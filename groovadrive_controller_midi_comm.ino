#include <MIDI.h>
#include <Adafruit_TinyUSB.h>
#include <queue>
#include <vector>

#define D0_PIN 2
#define D1_PIN 3
#define D2_PIN 4
#define D3_PIN 5

#define TL_PIN 6
#define TR_PIN 7
#define TH_PIN 8

struct TimedMsg {
  uint64_t ms;
  std::vector<byte> msg;
};

std::queue<byte> midi_msg_queue;
std::queue<TimedMsg> midi_timed_msg_queue;

Adafruit_USBD_MIDI usb_midi(1);
extern Adafruit_USBD_Device TinyUSBDevice;

MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, usbMIDI);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDISerial);

void onNoteOn(byte channel, byte noteNum, byte velocity) {
  midi_timed_msg_queue.push(TimedMsg{
    .ms = millis(),
    .msg = {(0x90+channel), noteNum, velocity}
  });
}

void onNoteOff(byte channel, byte noteNum, byte velocity) {
  midi_timed_msg_queue.push(TimedMsg {
    .ms = millis(),
    .msg = {(0x80+channel), noteNum, velocity}
  });
}

void onAfterTouchPoly(byte channel, byte noteNum, byte velocity)
{
  midi_timed_msg_queue.push(TimedMsg {
    .ms = millis(),
    .msg = {(0xa0+channel), noteNum, velocity}
  });
}

void onAfterTouchChannel(byte channel, byte velocity)
{
  midi_timed_msg_queue.push(TimedMsg {
    .ms = millis(),
    .msg = {(0xd0+channel), velocity}
  });
}

void onControlChange(byte channel, byte ccNum, byte value) {
  midi_timed_msg_queue.push(TimedMsg {
    .ms = millis(),
    .msg = {(0xb0+channel), ccNum, value}
  });
}

void onProgramChange(byte channel, byte program)
{
  midi_timed_msg_queue.push(TimedMsg {
    .ms = millis(),
    .msg = {(0xa0+channel), program}
  });
}

void onPitchBend(byte channel, int bend)
{
  midi_timed_msg_queue.push(TimedMsg {
    .ms = millis(),
    .msg = {(0xc0+channel), (bend >> 7) & 0x7f, bend & 0x7f}
  });
}

void onClock(void)
{
  midi_timed_msg_queue.push(TimedMsg {
    .ms = millis(),
    .msg = {0x78,}
  });
}

void setup() {
  TinyUSBDevice.setManufacturerDescriptor("Esquema Interativo");
  TinyUSBDevice.setProductDescriptor("Groovadrive MIDI");
  //TinyUSB_Device_Init(0);  
  
  usbMIDI.setHandleNoteOn(onNoteOn);
  usbMIDI.setHandleNoteOff(onNoteOff);
  usbMIDI.setHandleControlChange(onControlChange);
  usbMIDI.setHandleProgramChange(onProgramChange);
  usbMIDI.setHandleAfterTouchPoly(onAfterTouchPoly);
  usbMIDI.setHandleAfterTouchChannel(onAfterTouchChannel);
  usbMIDI.setHandlePitchBend(onPitchBend);
  usbMIDI.begin();
  
  usb_midi.setCableName(1, "Groovadrive MIDI");
  usb_midi.begin();

  
  Serial2.setPinout(8, 9);
  MIDISerial.setHandleNoteOn(onNoteOn);
  MIDISerial.setHandleNoteOff(onNoteOff);
  MIDISerial.setHandleControlChange(onControlChange);
  MIDISerial.setHandleProgramChange(onProgramChange);
  MIDISerial.setHandleAfterTouchPoly(onAfterTouchPoly);
  MIDISerial.setHandleAfterTouchChannel(onAfterTouchChannel);
  MIDISerial.setHandlePitchBend(onPitchBend);
  MIDISerial.begin(MIDI_CHANNEL_OMNI);

}

void setup1() {
  pinMode(D0_PIN, OUTPUT);
  pinMode(D1_PIN, OUTPUT);
  pinMode(D2_PIN, OUTPUT);
  pinMode(D3_PIN, OUTPUT);

  pinMode(TL_PIN, INPUT_PULLUP);
  pinMode(TH_PIN, INPUT_PULLUP);
  pinMode(TR_PIN, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  MIDISerial.read();
  usbMIDI.read();
  //digitalWrite(LED_BUILTIN, 1);
}

const uint64_t msg_timeout = 1000;

uint64_t last_msg_time = 0;

void loop1() {
  digitalWrite(LED_BUILTIN, midi_timed_msg_queue.empty());
  if (!midi_timed_msg_queue.empty()) {
    TimedMsg tmsg = midi_timed_msg_queue.front();
    uint64_t msg_time = millis() - tmsg.ms;
    if (msg_time > msg_timeout) {
      midi_timed_msg_queue.pop();
    } else if (midi_msg_queue.empty()) {
      for(const byte c : tmsg.msg) {
        midi_msg_queue.push(c);
      }
      midi_timed_msg_queue.pop();
    }
  }
  if (midi_msg_queue.empty()) {
    digitalWrite(TR_PIN, 1);    
  } else {
    byte msg = midi_msg_queue.front();
    digitalWrite(D0_PIN, msg&(1<<4));
    digitalWrite(D1_PIN, msg&(1<<5));
    digitalWrite(D2_PIN, msg&(1<<6));
    digitalWrite(D3_PIN, msg&(1<<7));
    digitalWrite(TR_PIN, 0);
    if (!digitalRead(TH_PIN)) {
      digitalWrite(D0_PIN, msg&(1));
      digitalWrite(D1_PIN, msg&(1<<1));
      digitalWrite(D2_PIN, msg&(1<<2));
      digitalWrite(D3_PIN, msg&(1<<3));
      while (!digitalRead(TH_PIN)) {}
      midi_msg_queue.pop();
    }
  }
}
