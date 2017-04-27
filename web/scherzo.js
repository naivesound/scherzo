"use strict";

function midiBind(scherzo, cc) {
  return function() { scherzo.midi(0xb0, cc, this.value | 0); };
}

var Button = {
  view : (c) => {
    return m('.btn', {
      className : (c.attrs.active ? 'active' : ''),
      onmousedown : c.attrs.onclick,
      ontouchstart : c.attrs.onclick,
    },
	     c.attrs.name);
  }
};

var Knob = {
  view : (c) => {
    return m('input[type=range]',
	     {min : 0, max : 127, oninput : c.attrs.oninput});
  }
};

var Keyboard = {
  view : (c) => {
    let pianoLayout = [ 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1 ];
    let scherzo = c.attrs.scherzo;
    let notes = scherzo.notes;
    let rootNote = scherzo.rootNote;
    let keys = [];
    for (let i = 0; i < pianoLayout.length; i++) {
      let note = rootNote + i;
      let className = (pianoLayout[i] ? 'white' : 'black');
      if (notes[note]) {
	className += ' active';
      }
      keys.push(m('.piano-key', {
	className : className,
	ontouchstart : () => { scherzo.noteOn(note, 127) },
	ontouchend : () => { scherzo.noteOff(note, 127) },
	onmousedown : () => { scherzo.noteOn(note, 127) },
	onmouseup : () => { scherzo.noteOff(note, 127) },
	onmouseleave : () => { scherzo.noteOff(note, 127) },
	onmouseenter : (e) => { (e.buttons & 1) && scherzo.noteOn(note, 127) },
      }));
    }
    return m('.piano', m('.piano-kbd', m('.piano-keys', keys)));
  }
};

var Layout = {
  view : (c) => {
    return m('.container', m(Knob, {oninput : midiBind(c.attrs.scherzo, 0x07)}),
	     m(Knob, {oninput : midiBind(c.attrs.scherzo, 0x0d)}),
	     m(Knob, {oninput : midiBind(c.attrs.scherzo, 0x0c)}),
	     m(Knob, {oninput : midiBind(c.attrs.scherzo, 0x09)}), m(Button, {
	       name : 'Tap',
	       active : scherzo.beatIndicator,
	       onclick : () => scherzo.tap()
	     }),
	     m(Button, {
	       name : 'Loop',
	       active : scherzo.loopIndicator,
	       onclick : () => scherzo.loop()
	     }),
	     m(Button, {name : 'Cancel', onclick : () => scherzo.cancel()}),
	     m(Keyboard, {scherzo : scherzo}));
  }
};

class Scherzo {
  constructor() {
    const audioContext = new window.AudioContext();

    if (navigator.requestMIDIAccess) {
      navigator.requestMIDIAccess().then((midi) => {
	const enumerate = () => {
	  let inputs = midi.inputs.values();
	  for (let input = inputs.next(); input && !input.done;
	       input = inputs.next()) {
	    input.value.onmidimessage =
		(m) => { this.midi(m.data[0], m.data[1], m.data[2]); };
	  }
	};
	enumerate();
	midi.onstatechange = enumerate;
      });
    }

    const AUDIO_BUFFER_SIZE = 1024;
    const pcm = audioContext.createScriptProcessor(AUDIO_BUFFER_SIZE, 0, 2);
    const analyser = audioContext.createAnalyser();

    this.sampleRate = audioContext.sampleRate;
    this.scherzo =
	Module.ccall('scherzo_create', 'number', [ 'number', 'number' ],
		     [ this.sampleRate, 64 ]);

    const sizeInBytes = AUDIO_BUFFER_SIZE * 2 * Int16Array.BYTES_PER_ELEMENT;
    this.int16BufPtr = Module._malloc(sizeInBytes);
    pcm.onaudioprocess = this.audioCallback.bind(this);
    this.pcm = pcm;
    this.pcm.connect(audioContext.destination);

    this.loadInstrument(0);

    this.rootNote = 48;
    this.notes = {};
    this.beatIndicator = this.loopIndicator = false;
  }
  loadInstrument(n) {
    Module.ccall('scherzo_load_instrument', 'int',
		 [ 'number', 'string', 'number' ], [ this.scherzo, 'sf2', n ]);
  }
  midi(msg, a, b) {
    if (msg == 0x90 && b > 0) {
      if (this.notes[a]) {
	return;
      }
      this.notes[a] = b;
    } else if (msg == 0x80 || (msg == 0x90 && b == 0)) {
      delete this.notes[a];
    }
    console.log(msg, a, b, this.notes);
    m.redraw();
    Module.ccall('scherzo_midi', 'void',
		 [ 'number', 'number', 'number', 'number' ],
		 [ this.scherzo, msg, a, b ]);
  }
  loop() { this.midi(0xb0, 0x51, 1); }
  cancel() { this.midi(0xb0, 0x52, 1); }
  tap() { this.midi(0xb0, 0x50, 1); }
  noteOn(note, velocity) {
    velocity = velocity || 127;
    this.midi(0x90, note, velocity);
  }
  noteOff(note) { this.midi(0x80, note, 0); }
  audioCallback(e) {
    const left = e.outputBuffer.getChannelData(0);
    const right = e.outputBuffer.getChannelData(1);
    const events = Module.ccall(
	'scherzo_write_stereo', 'number', [ 'number', 'number', 'number' ],
	[ this.scherzo, this.int16BufPtr, left.length ]);
    for (let i = 0; i < left.length; i++) {
      const leftLo = Module.getValue(this.int16BufPtr + i * 4, 'i8');
      const leftHi = Module.getValue(this.int16BufPtr + i * 4 + 1, 'i8');
      const rightLo = Module.getValue(this.int16BufPtr + i * 4 + 2, 'i8');
      const rightHi = Module.getValue(this.int16BufPtr + i * 4 + 3, 'i8');

      right[i] = (rightHi * 256 + (rightLo & 0xff)) / 0x8000;
      left[i] = (leftHi * 256 + (leftLo & 0xff)) / 0x8000;
    }

    let nextBeatIndicator = !!(events & (1 << 1));
    let nextLoopIndicator = !!(events & (1 << 2));

    if (nextBeatIndicator != this.beatIndicator ||
	nextLoopIndicator != this.loopIndicator) {
      this.beatIndicator = nextBeatIndicator;
      this.loopIndicator = nextLoopIndicator;
      m.redraw();
    }
  }
}

//
// Main part: create glitch audio player, handle global events, render layout
//
var Module = window.Module || {
  'print' : function(text) { console.log(text) },
  'printErr' : function(text) { console.warn(text) }
};

(function(d) {
  var script = d.createElement('script');
  script.type = 'text/javascript';
  script.async = true;
  if ('WebAssembly' in window) {
    script.src = 'scherzo-loader.js';
  } else {
    script.src = 'scherzo-asm.js';
  }
  d.getElementsByTagName('head')[0].appendChild(script);
})(document);

Module['onRuntimeInitialized'] = function() {
  FS.mkdir('/sf2');
  FS.createPreloadedFile('/sf2', 'piano.sf3', '/piano.sf3', true, false,
			 () => { main(); });
};

const keyDownBindings = {
  // Global hotkeys for metronome and looper
  'Space' : (scherzo) => scherzo.tap(),
  'Enter' : (scherzo) => scherzo.loop(),
  'Backspace' : (scherzo) => scherzo.cancel(),
  'Delete' : (scherzo) => scherzo.cancel(),
  'Ctrl+KeyZ' : (scherzo) => scherzo.cancel(),
  // TODO: master gain, looper gain, metronome gain, looper decay
  // Octave switch
  'KeyZ' : (scherzo) => scherzo.rootNote = Math.max(scherzo.rootNote - 12, 0),
  'KeyX' : (scherzo) => scherzo.rootNote = Math.min(scherzo.rootNote + 12, 108),
  // Piano keyboard
  'KeyA' : (scherzo) => scherzo.noteOn(scherzo.rootNote, 127),
  'KeyW' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 1, 127),
  'KeyS' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 2, 127),
  'KeyE' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 3, 127),
  'KeyD' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 4, 127),
  'KeyF' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 5, 127),
  'KeyT' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 6, 127),
  'KeyG' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 7, 127),
  'KeyY' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 8, 127),
  'KeyH' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 9, 127),
  'KeyU' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 10, 127),
  'KeyJ' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 11, 127),
  'KeyK' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 12, 127),
  'KeyO' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 13, 127),
  'KeyL' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 14, 127),
  'KeyP' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 15, 127),
  'Semicolon' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 16, 127),
  'BracketRight' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 17, 127),
  'Quote' : (scherzo) => scherzo.noteOn(scherzo.rootNote + 18, 127),
};

const keyUpBindings = {
  'KeyA' : (scherzo) => scherzo.noteOff(scherzo.rootNote),
  'KeyW' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 1),
  'KeyS' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 2),
  'KeyE' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 3),
  'KeyD' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 4),
  'KeyF' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 5),
  'KeyT' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 6),
  'KeyG' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 7),
  'KeyY' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 8),
  'KeyH' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 9),
  'KeyU' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 10),
  'KeyJ' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 11),
  'KeyK' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 12),
  'KeyO' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 13),
  'KeyL' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 14),
  'KeyP' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 15),
  'Semicolon' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 16),
  'BracketRight' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 17),
  'Quote' : (scherzo) => scherzo.noteOff(scherzo.rootNote + 18),
};

function main() {
  console.log('staring scherzo');
  const scherzo = new Scherzo();
  document.onkeydown = (e) => {
    let key = (e.ctrlKey ? 'Ctrl+' : '') + e.code;
    let f = keyDownBindings[key];
    if (f) {
      f(scherzo);
    }
  };
  document.onkeyup = (e) => {
    let key = (e.ctrlKey ? 'Ctrl+' : '') + e.code;
    let f = keyUpBindings[key];
    if (f) {
      f(scherzo);
    }
  };
  window.scherzo = scherzo;
  m.mount(document.body, {view : () => m(Layout, {scherzo : scherzo})});
}
