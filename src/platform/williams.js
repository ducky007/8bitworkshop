"use strict";

var WILLIAMS_PRESETS = [
];

var WilliamsPlatform = function(mainElement, proto) {
  var self = this;
  this.__proto__ = new (proto?proto:Base6809Platform)();

  var SCREEN_HEIGHT = 304;
  var SCREEN_WIDTH = 256;

  var cpu, ram, rom, nvram;
  var portsel = 0;
  var banksel = 0;
  var watchdog_counter;
  var pia6821 = new RAM(8).mem;
  var blitregs = new RAM(8).mem;

  var video, timer, pixels, displayPCs;
  var membus, iobus;
  var screenNeedsRefresh = false;
  var video_counter;

  var xtal = 12000000;
  var cpuFrequency = xtal/3/4;
  var cpuCyclesPerFrame = cpuFrequency/60; // TODO
  var INITIAL_WATCHDOG = 64;
  var PIXEL_ON = 0xffeeeeee;
  var PIXEL_OFF = 0xff000000;

  var DEFENDER_KEYCODE_MAP = makeKeycodeMap([
    [Keys.VK_SPACE, 4, 0x1],
    [Keys.VK_RIGHT, 4, 0x2],
    [Keys.VK_Z, 4, 0x4],
    [Keys.VK_X, 4, 0x8],
    [Keys.VK_2, 4, 0x10],
    [Keys.VK_1, 4, 0x20],
    [Keys.VK_LEFT, 4, 0x40],
    [Keys.VK_DOWN, 4, 0x80],
    [Keys.VK_UP, 6, 0x1],
    [Keys.VK_5, 0, 0x4],
    [Keys.VK_7, 0, 0x1],
    [Keys.VK_8, 0, 0x2],
    [Keys.VK_9, 0, 0x8],
  ]);

  var ROBOTRON_KEYCODE_MAP = makeKeycodeMap([
    [Keys.VK_W, 0, 0x1],
    [Keys.VK_S, 0, 0x2],
    [Keys.VK_A, 0, 0x4],
    [Keys.VK_D, 0, 0x8],
    [Keys.VK_1, 0, 0x10],
    [Keys.VK_2, 0, 0x20],
    [Keys.VK_UP,    0, 0x40],
    [Keys.VK_DOWN,  0, 0x80],
    [Keys.VK_LEFT,  2, 0x1],
    [Keys.VK_RIGHT, 2, 0x2],
    [Keys.VK_7, 4, 0x1],
    [Keys.VK_8, 4, 0x2],
    [Keys.VK_5, 4, 0x4],
    [Keys.VK_9, 4, 0x8],
  ]);

  var palette = [];
  for (var ii=0; ii<16; ii++)
    palette[ii] = 0xff000000;

  this.getPresets = function() {
    return WILLIAMS_PRESETS;
  }

  // Defender

  var ioread_defender = new AddressDecoder([
    [0x400, 0x5ff, 0x1ff, function(a) { return nvram.mem[a]; }],
    [0x800, 0x800, 0,     function(a) { return video_counter; }],
    [0xc00, 0xc07, 0x7,   function(a) { return pia6821[a]; }],
    [0x0,   0xfff, 0,     function(a) { /*console.log('ioread',hex(a));*/ }],
  ]);

  var iowrite_defender = new AddressDecoder([
    [0x0,   0xf,   0xf,   setPalette],
    [0x3fc, 0x3ff, 0,     function(a,v) { if (v == 0x38) watchdog_counter = INITIAL_WATCHDOG; }],
    [0x400, 0x5ff, 0x1ff, function(a,v) { nvram.mem[a] = v; }],
    [0xc00, 0xc07, 0x7,   function(a,v) { pia6821[a] = v; }],
    [0x0,   0xfff, 0,     function(a,v) { console.log('iowrite',hex(a),hex(v)); }],
  ]);

  var memread_defender = new AddressDecoder([
    [0x0000, 0xbfff, 0xffff, function(a) { return ram.mem[a]; }],
    [0xc000, 0xcfff, 0x0fff, function(a) {
      switch (banksel) {
        case 0: return ioread_defender(a);
        case 1: return rom[a+0x3000];
        case 2: return rom[a+0x4000];
        case 3: return rom[a+0x5000];
        case 7: return rom[a+0x6000];
        default: return 0; // TODO: error light
      }
    }],
    [0xd000, 0xffff, 0xffff, function(a) { return rom ? rom[a-0xd000] : 0; }],
  ]);

  var memwrite_defender = new AddressDecoder([
    [0x0000, 0x97ff, 0,      write_display_byte],
    [0x9800, 0xbfff, 0,      function(a,v) { ram.mem[a] = v; }],
    [0xc000, 0xcfff, 0x0fff, iowrite_defender],
    [0xd000, 0xdfff, 0,      function(a,v) { banksel = v&0x7; }],
    [0,      0xffff, 0,      function(a,v) { console.log(hex(a), hex(v)); }],
  ]);

  // Robotron, Joust, Bubbles, Stargate

  var ioread_williams = new AddressDecoder([
    [0x804, 0x807, 0x3,   function(a) { return pia6821[a]; }],
    [0x80c, 0x80f, 0x3,   function(a) { return pia6821[a+4]; }],
    [0xb00, 0xbff, 0,     function(a) { return video_counter; }],
    [0xc00, 0xfff, 0x3ff, function(a) { return nvram.mem[a]; }],
    [0x0,   0xfff, 0,     function(a) { /* console.log('ioread',hex(a)); */ }],
  ]);

  var iowrite_williams = new AddressDecoder([
    [0x0,   0xf,   0xf,   setPalette],
    [0x804, 0x807, 0x3,   function(a,v) { console.log('iowrite',a); }], // TODO: sound
    [0x80c, 0x80f, 0x3,   function(a,v) { console.log('iowrite',a+4); }], // TODO: sound
    [0x900, 0x9ff, 0,     function(a,v) { banksel = v & 0x1; }],
    [0xa00, 0xa07, 0x7,   setBlitter],
    [0xbff, 0xbff, 0,     function(a,v) { if (v == 0x39) watchdog_counter = INITIAL_WATCHDOG; }],
    [0xc00, 0xfff, 0x3ff, function(a,v) { nvram.mem[a] = v; }],
    //[0x0,   0xfff, 0,     function(a,v) { console.log('iowrite',hex(a),hex(v)); }],
  ]);

  var memread_williams = new AddressDecoder([
    [0x0000, 0x8fff, 0xffff, function(a) { return banksel ? rom[a] : ram.mem[a]; }],
    [0x9000, 0xbfff, 0xffff, function(a) { return ram.mem[a]; }],
    [0xc000, 0xcfff, 0x0fff, ioread_williams],
    [0xd000, 0xffff, 0xffff, function(a) { return rom ? rom[a-0x4000] : 0; }],
  ]);

  var memwrite_williams = new AddressDecoder([
    [0x0000, 0x8fff, 0,      write_display_byte],
    [0x9000, 0xbfff, 0,      function(a,v) { ram.mem[a] = v; }],
    [0xc000, 0xcfff, 0x0fff, iowrite_williams],
    //[0x0000, 0xffff, 0,      function(a,v) { console.log(hex(a), hex(v)); }],
  ]);

  // d1d6 ldu $11 / beq $d1ed

  function setPalette(a,v) {
    // RRRGGGBB
    var color = 0xff000000 | ((v&7)<<5) | (((v>>3)&7)<<13) | (((v>>6)<<22));
    if (color != palette[a]) {
      palette[a] = color;
      screenNeedsRefresh = true;
    }
  }

  function write_display_byte(a,v) {
    ram.mem[a] = v;
    drawDisplayByte(a, v);
    displayPCs[a] = cpu.getPC(); // save program counter
  }

  function drawDisplayByte(a,v) {
    var ofs = ((a&0xff00)<<1) | ((a&0xff)^0xff);
    pixels[ofs] = palette[v>>4];
    pixels[ofs+256] = palette[v&0xf];
  }

  function setBlitter(a,v) {
    if (a) {
      blitregs[a] = v;
    } else {
      var cycles = doBlit(v);
      cpu.setTstates(cpu.getTstates() + cycles);
    }
  }

  function doBlit(flags) {
    //console.log(hex(flags), blitregs);
    flags &= 0xff;
    var offs = SCREEN_HEIGHT - blitregs[7];
    var sstart = (blitregs[2] << 8) + blitregs[3];
    var dstart = (blitregs[4] << 8) + blitregs[5];
    var w = blitregs[6] ^ 4; // blitter bug fix
    var h = blitregs[7] ^ 4;
    if(w==0) w++;
    if(h==0) h++;
    if(h==255) h++;
    var sxinc = (flags & 0x1) ? 256 : 1;
    var syinc = (flags & 0x1) ? 1 : w;
    var dxinc = (flags & 0x2) ? 256 : 1;
    var dyinc = (flags & 0x2) ? 1 : w;
    var pixdata = 0;
    for (var y = 0; y < h; y++) {
      var source = sstart & 0xffff;
      var dest = dstart & 0xffff;
      for (var x = 0; x < w; x++) {
        var data = memread_williams(source);
        if (flags & 0x20) {
          pixdata = (pixdata << 8) | data;
          blit_pixel(dest, (pixdata >> 4) & 0xff, flags);
        } else {
          blit_pixel(dest, data, flags);
        }
        source += sxinc;
        source &= 0xffff;
        dest += dxinc;
        dest &= 0xffff;
      }
      if (flags & 0x2)
        dstart = (dstart & 0xff00) | ((dstart + dyinc) & 0xff);
      else
        dstart += dyinc;
      if (flags & 0x1)
        sstart = (sstart & 0xff00) | ((sstart + syinc) & 0xff);
      else
        sstart += syinc;
    }
    return w * h * (2 + ((flags&0x4)!=0)); // # of memory accesses
  }

  function blit_pixel(dstaddr, srcdata, flags) {
    var curpix = dstaddr < 0xc000 ? ram.mem[dstaddr] : memread_williams(dstaddr);
    var solid = blitregs[1];
    var keepmask = 0xff;          //what part of original dst byte should be kept, based on NO_EVEN and NO_ODD flags
    //even pixel (D7-D4)
    if((flags & 0x8) && !(srcdata & 0xf0)) {   //FG only and src even pixel=0
        if(flags & 0x80) keepmask &= 0x0f; // no even
    } else {
        if(!(flags & 0x80)) keepmask &= 0x0f; // not no even
    }
    //odd pixel (D3-D0)
    if((flags & 0x8) && !(srcdata & 0x0f)) {   //FG only and src odd pixel=0
        if(flags & 0x40) keepmask &= 0xf0; // no odd
    } else {
        if(!(flags & 0x40)) keepmask &= 0xf0; // not no odd
    }
    curpix &= keepmask;
    if(flags & 0x10) // solid bit
      curpix |= (solid & ~keepmask);
    else
      curpix |= (srcdata & ~keepmask);
    if (dstaddr < 0x9000) // can cause recursion otherwise
      memwrite_williams(dstaddr, curpix);
  }

// TODO
/*
  var trace = false;
  var _traceinsns = {};
  function _trace() {
    var pc = cpu.getPC();
    if (!_traceinsns[pc]) {
      _traceinsns[pc] = 1;
      console.log(hex(pc), cpu.getTstates());
    }
  }
*/
  this.start = function() {
    ram = new RAM(0xc000);
    nvram = new RAM(0x400);
    // TODO: save in browser storage?
    // defender nvram.mem.set([240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,242,241,242,247,240,244,244,245,242,244,250,240,241,248,243,241,245,245,243,244,241,244,253,240,241,245,249,242,240,244,252,244,245,244,244,240,241,244,242,248,245,245,240,244,247,244,244,240,241,242,245,242,240,244,243,245,242,244,242,240,241,241,240,243,245,244,253,245,242,245,243,240,240,248,242,246,245,245,243,245,243,245,242,240,240,246,240,241,240,245,244,244,253,244,248,240,240,245,250,240,241,240,240,240,243,240,243,240,241,240,244,240,241,240,241,240,240,240,240,240,240,240,245,241,245,240,241,240,245,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240]);

    displayPCs = new Uint16Array(new ArrayBuffer(0x9800*2));
    rom = padBytes(new lzgmini().decode(ROBOTRON_ROM).slice(0), 0xc001);
    membus = {
      read: memread_williams,
			write: memwrite_williams,
    };
    cpu = self.newCPU(membus);
    video = new RasterVideo(mainElement, SCREEN_WIDTH, SCREEN_HEIGHT, {rotate:-90});
    video.create();
		$(video.canvas).click(function(e) {
			var x = Math.floor(e.offsetX * video.canvas.width / $(video.canvas).width());
			var y = Math.floor(e.offsetY * video.canvas.height / $(video.canvas).height());
			var addr = (x>>3) + (y*32) + 0x400;
			console.log(x, y, hex(addr,4), "PC", hex(displayPCs[addr],4));
		});
    var idata = video.getFrameData();
    setKeyboardFromMap(video, pia6821, ROBOTRON_KEYCODE_MAP);
    pixels = video.getFrameData();
    timer = new AnimationTimer(60, function() {
			if (!self.isRunning())
				return;
      // interrupts happen every 1/4 of the screen
      for (var quarter=0; quarter<4; quarter++) {
        video_counter = [0x00, 0x3c, 0xbc, 0xfc][quarter];
        if (membus.read != memread_defender || pia6821[7] == 0x3c) { // TODO?
          if (cpu.interrupt)
            cpu.interrupt();
          if (cpu.requestInterrupt)
            cpu.requestInterrupt();
        }
        self.runCPU(cpu, cpuCyclesPerFrame/4);
      }
      if (screenNeedsRefresh) {
        for (var i=0; i<0x9800; i++)
          drawDisplayByte(i, ram.mem[i]);
        screenNeedsRefresh = false;
      }
      video.updateFrame();
      if (watchdog_counter-- <= 0) {
        console.log("WATCHDOG FIRED, PC =", cpu.getPC().toString(16)); // TODO: alert on video
        // TODO: self.breakpointHit(cpu.T());
        self.reset();
      }
      self.restartDebugState();
    });
  }

  this.loadROM = function(title, data) {
    if (data.length > 2) {
      rom = padBytes(data, 0xc000);
    }
    // TODO
    self.reset();
  }

  this.loadState = function(state) {
    cpu.loadState(state.c);
    ram.mem.set(state.b);
    nvram.mem.set(state.nvram);
    //pia6821.set(state.pia);
    blitregs.set(state.blt);
    watchdog_counter = state.wdc;
    banksel = state.bs;
    portsel = state.ps;
  }
  this.saveState = function() {
    return {
      c:self.getCPUState(),
      b:ram.mem.slice(0),
      nvram:nvram.mem.slice(0),
      pia:pia6821.slice(0),
      blt:blitregs.slice(0),
      wdc:watchdog_counter,
      bs:banksel,
      ps:portsel,
    };
  }
  this.getCPUState = function() {
    return cpu.saveState();
  }

  this.isRunning = function() {
    return timer.isRunning();
  }
  this.pause = function() {
    timer.stop();
  }
  this.resume = function() {
    timer.start();
  }
  this.reset = function() {
    cpu.reset();
    watchdog_counter = INITIAL_WATCHDOG;
    banksel = 1;
  }
  this.readAddress = function(addr) {
    return membus.read(addr);
  }
}

var WilliamsZ80Platform = function(mainElement) {
  this.__proto__ = new WilliamsPlatform(mainElement, BaseZ80Platform);

  this.ramStateToLongString = function(state) {
    var blt = state.blt;
    var sstart = (blt[2] << 8) + blt[3];
    var dstart = (blt[4] << 8) + blt[5];
    var w = blt[6] ^ 4; // blitter bug fix
    var h = blt[7] ^ 4;
    return "\nBLIT"
      + " " + hex(sstart,4) + " " + hex(dstart,4)
      + " w:" + hex(w) + " h:" + hex(h)
      + " f:" + hex(blt[0]) + " s:" + hex(blt[1]);
  }
}

PLATFORMS['williams'] = WilliamsPlatform;
PLATFORMS['williams-z80'] = WilliamsZ80Platform;
