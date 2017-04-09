"use strict";

class Scherzo {
  constructor() {
    const audioContext = new window.AudioContext();

    if (navigator.requestMIDIAccess) {
      navigator.requestMIDIAccess().then((midi) => {
	const enumerate = () => {
	  let inputs = midi.inputs.values();
	  for (let input = inputs.next(); input && !input.done;
	       input = inputs.next()) {
	    input.value.onmidimessage = (m) => {
	      Module.ccall('scherzo_midi', 'number',
			   [ 'number', 'number', 'number' ],
			   [ this.scherzo, m.data[0], m.data[1], m.data[2] ]);
	    };
	  }
	};
	enumerate();
	midi.onstatechange = enumerate;
      });
    }

    const AUDIO_BUFFER_SIZE = 512;
    const pcm = audioContext.createScriptProcessor(AUDIO_BUFFER_SIZE, 0, 2);
    const analyser = audioContext.createAnalyser();

    this.sampleRate = audioContext.sampleRate;
    this.scherzo =
	Module.ccall('scherzo_create', 'number', [ 'number', 'number' ],
		     [ this.sampleRate, 32 ]);

    const sizeInBytes = AUDIO_BUFFER_SIZE * 2 * Int16Array.BYTES_PER_ELEMENT;
    this.int16BufPtr = Module._malloc(sizeInBytes);
    analyser.fftSize = 2048;
    analyser.smoothingTimeConstant = 0;
    analyser.connect(audioContext.destination);
    pcm.onaudioprocess = this.audioCallback.bind(this);
    this.analyser = analyser;
    this.pcm = pcm;
    this.pcm.connect(this.analyser);

    this.loadInstrument(0);
  }
  loadInstrument(n) {
    Module.ccall('scherzo_load_instrument', 'int', [ 'number', 'number' ],
		 [ this.scherzo, n ]);
  }
  looper(n) {
    Module.ccall('scherzo_looper', 'void', [ 'number', 'number' ],
		 [ this.scherzo, n ]);
  }
  tapBPM() {
    Module.ccall('scherzo_tap_bpm', 'void', [ 'number' ], [ this.scherzo ]);
  }
  audioCallback(e) {
    const left = e.outputBuffer.getChannelData(0);
    const right = e.outputBuffer.getChannelData(1);
    Module.ccall('scherzo_write_stereo', null, [ 'number', 'number', 'number' ],
		 [ this.scherzo, this.int16BufPtr, left.length ]);
    for (let i = 0; i < left.length; i++) {
      const leftLo = Module.getValue(this.int16BufPtr + i * 4, 'i8');
      const leftHi = Module.getValue(this.int16BufPtr + i * 4 + 1, 'i8');
      const rightLo = Module.getValue(this.int16BufPtr + i * 4 + 2, 'i8');
      const rightHi = Module.getValue(this.int16BufPtr + i * 4 + 3, 'i8');

      // console.log(this.int16BufPtr + i * 4, leftLo, leftHi, rightLo, rightHi,
      // left.length, right.length);
      right[i] = (rightHi * 256 + (rightLo & 0xff)) / 0x8000;
      left[i] = (leftHi * 256 + (leftLo & 0xff)) / 0x8000;
    }
    // console.log(left, right);
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
  FS.createPreloadedFile('/sf2', 'piano.sf2', '/piano.sf2', true, false, () => {
    FS.createPreloadedFile('/sf2', 'grand_piano.sf2', '/grand_piano.sf2', true,
			   false, () => { main(); });
  });
};

function main() {
  console.log('staring scherzo');
  const scherzo = new Scherzo();
  // Handle global hotkeys: Enter=looper, Del/BackSpace=cancel, Space=BPM
  document.onkeydown = (e) => {
    if (e.keyCode === 32) {
      scherzo.tapBPM();
    }
    if (e.keyCode === 13) {
      scherzo.looper(0);
    }
    if (e.keyCode === 8 || e.keyCode == 46) {
      scherzo.looper(1);
    }
  };
  window.scherzo = scherzo;
}
