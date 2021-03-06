
#include <string.h>

typedef unsigned char byte;
typedef unsigned char word;

__sfr __at (0x0) input0;
__sfr __at (0x1) input1;
__sfr __at (0x2) input2;
__sfr __at (0x2) bitshift_offset;
__sfr __at (0x3) bitshift_read;
__sfr __at (0x4) bitshift_value;
__sfr __at (0x6) watchdog_strobe;
byte __at (0x2400) vidmem[224][32]; // 256x224x1 video memory

#define FIRE1 (input1 & 0x10)
#define LEFT1 (input1 & 0x20)
#define RIGHT1 (input1 & 0x40)

void scanline96() __interrupt;
void scanline224() __interrupt;

void main();
// start routine @ 0x0
// set stack pointer, enable interrupts
void start() {
__asm
	LD      SP,#0x2400
        EI
        NOP
__endasm;
	main();
}

// scanline 96 interrupt @ 0x8
// we don't have enough bytes to make this an interrupt
// because the next routine is at 0x10
void _RST_8()  {
__asm
	NOP
        NOP
        NOP
        NOP
        NOP
__endasm;
	scanline96();
}

// scanline 224 interrupt @ 0x10
// this one, we make an interrupt so it saves regs.
void scanline224() __interrupt {
  vidmem[2]++;
}

// scanline 96 function, saves regs
void scanline96() __interrupt {
  vidmem[0]++;
}

/// GRAPHICS FUNCTIONS

/*
void draw_hline(byte y, byte x1, byte x2) {
}
*/

void draw_vline(byte x, byte y1, byte y2) {
  byte yb1 = y1/8;
  byte yb2 = y2/8;
  byte* dest = &vidmem[x][yb1];
  signed char nbytes = yb2 - yb1;
  *dest++ ^= 0xff << (y1&7);
  if (nbytes > 0) {
    while (--nbytes > 0) {
      *dest++ ^= 0xff;
    }
    *dest ^= 0xff >> (~y2&7);
  } else {
    *--dest ^= 0xff << ((y2+1)&7);
  }
}

#define LOCHAR 0x20
#define HICHAR 0x5e

const byte font8x8[HICHAR-LOCHAR+1][8] = {
{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, { 0x00,0x00,0x00,0x79,0x79,0x00,0x00,0x00 }, { 0x00,0x70,0x70,0x00,0x00,0x70,0x70,0x00 }, { 0x14,0x7f,0x7f,0x14,0x14,0x7f,0x7f,0x14 }, { 0x00,0x12,0x3a,0x6b,0x6b,0x2e,0x24,0x00 }, { 0x00,0x63,0x66,0x0c,0x18,0x33,0x63,0x00 }, { 0x00,0x26,0x7f,0x59,0x59,0x77,0x27,0x05 }, { 0x00,0x00,0x00,0x10,0x30,0x60,0x40,0x00 }, { 0x00,0x00,0x1c,0x3e,0x63,0x41,0x00,0x00 }, { 0x00,0x00,0x41,0x63,0x3e,0x1c,0x00,0x00 }, { 0x08,0x2a,0x3e,0x1c,0x1c,0x3e,0x2a,0x08 }, { 0x00,0x08,0x08,0x3e,0x3e,0x08,0x08,0x00 }, { 0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00 }, { 0x00,0x08,0x08,0x08,0x08,0x08,0x08,0x00 }, { 0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00 }, { 0x00,0x01,0x03,0x06,0x0c,0x18,0x30,0x20 }, { 0x00,0x3e,0x7f,0x49,0x51,0x7f,0x3e,0x00 }, { 0x00,0x01,0x11,0x7f,0x7f,0x01,0x01,0x00 }, { 0x00,0x23,0x67,0x45,0x49,0x79,0x31,0x00 }, { 0x00,0x22,0x63,0x49,0x49,0x7f,0x36,0x00 }, { 0x00,0x0c,0x0c,0x14,0x34,0x7f,0x7f,0x04 }, { 0x00,0x72,0x73,0x51,0x51,0x5f,0x4e,0x00 }, { 0x00,0x3e,0x7f,0x49,0x49,0x6f,0x26,0x00 }, { 0x00,0x60,0x60,0x4f,0x5f,0x70,0x60,0x00 }, { 0x00,0x36,0x7f,0x49,0x49,0x7f,0x36,0x00 }, { 0x00,0x32,0x7b,0x49,0x49,0x7f,0x3e,0x00 }, { 0x00,0x00,0x00,0x12,0x12,0x00,0x00,0x00 }, { 0x00,0x00,0x00,0x13,0x13,0x00,0x00,0x00 }, { 0x00,0x08,0x1c,0x36,0x63,0x41,0x41,0x00 }, { 0x00,0x14,0x14,0x14,0x14,0x14,0x14,0x00 }, { 0x00,0x41,0x41,0x63,0x36,0x1c,0x08,0x00 }, { 0x00,0x20,0x60,0x45,0x4d,0x78,0x30,0x00 }, { 0x00,0x3e,0x7f,0x41,0x59,0x79,0x3a,0x00 }, { 0x00,0x1f,0x3f,0x68,0x68,0x3f,0x1f,0x00 }, { 0x00,0x7f,0x7f,0x49,0x49,0x7f,0x36,0x00 }, { 0x00,0x3e,0x7f,0x41,0x41,0x63,0x22,0x00 }, { 0x00,0x7f,0x7f,0x41,0x63,0x3e,0x1c,0x00 }, { 0x00,0x7f,0x7f,0x49,0x49,0x41,0x41,0x00 }, { 0x00,0x7f,0x7f,0x48,0x48,0x40,0x40,0x00 }, { 0x00,0x3e,0x7f,0x41,0x49,0x6f,0x2e,0x00 }, { 0x00,0x7f,0x7f,0x08,0x08,0x7f,0x7f,0x00 }, { 0x00,0x00,0x41,0x7f,0x7f,0x41,0x00,0x00 }, { 0x00,0x02,0x03,0x41,0x7f,0x7e,0x40,0x00 }, { 0x00,0x7f,0x7f,0x1c,0x36,0x63,0x41,0x00 }, { 0x00,0x7f,0x7f,0x01,0x01,0x01,0x01,0x00 }, { 0x00,0x7f,0x7f,0x30,0x18,0x30,0x7f,0x7f }, { 0x00,0x7f,0x7f,0x38,0x1c,0x7f,0x7f,0x00 }, { 0x00,0x3e,0x7f,0x41,0x41,0x7f,0x3e,0x00 }, { 0x00,0x7f,0x7f,0x48,0x48,0x78,0x30,0x00 }, { 0x00,0x3c,0x7e,0x42,0x43,0x7f,0x3d,0x00 }, { 0x00,0x7f,0x7f,0x4c,0x4e,0x7b,0x31,0x00 }, { 0x00,0x32,0x7b,0x49,0x49,0x6f,0x26,0x00 }, { 0x00,0x40,0x40,0x7f,0x7f,0x40,0x40,0x00 }, { 0x00,0x7e,0x7f,0x01,0x01,0x7f,0x7e,0x00 }, { 0x00,0x7c,0x7e,0x03,0x03,0x7e,0x7c,0x00 }, { 0x00,0x7f,0x7f,0x06,0x0c,0x06,0x7f,0x7f }, { 0x00,0x63,0x77,0x1c,0x1c,0x77,0x63,0x00 }, { 0x00,0x70,0x78,0x0f,0x0f,0x78,0x70,0x00 }, { 0x00,0x43,0x47,0x4d,0x59,0x71,0x61,0x00 }, { 0x00,0x00,0x7f,0x7f,0x41,0x41,0x00,0x00 }, { 0x00,0x20,0x30,0x18,0x0c,0x06,0x03,0x01 }, { 0x00,0x00,0x41,0x41,0x7f,0x7f,0x00,0x00 }, { 0x00,0x08,0x18,0x3f,0x3f,0x18,0x08,0x00 }
};

const byte bitmap1[] =
{6,63,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00,0x80,0x63,0x00,0x00,0x00,0x00,0xc0,0xc0,0x00,0x00,0x00,0x00,0x5e,0x80,0x00,0x00,0x00,0x00,0x73,0x80,0x00,0x00,0x00,0x00,0xe3,0x83,0x00,0x00,0x00,0x00,0xe3,0x9f,0x00,0x00,0x00,0x00,0xc2,0xbf,0x00,0x00,0x00,0xc0,0x03,0xff,0x00,0x00,0x00,0x70,0x06,0xfe,0x01,0x00,0x00,0x18,0x0c,0x00,0x03,0x00,0x00,0x0c,0x18,0x00,0x02,0x00,0x00,0x02,0x30,0x00,0x02,0x00,0x00,0x0f,0xe0,0x00,0x03,0x00,0xc0,0x10,0x80,0xef,0x01,0x00,0x40,0x27,0x00,0x38,0x00,0x00,0xa0,0x47,0x00,0x60,0x00,0x00,0xe0,0x4f,0xf8,0x60,0x00,0x00,0xe0,0x4f,0x0c,0xe1,0x00,0x00,0xe0,0x4f,0x06,0xe2,0x01,0x00,0xe0,0x4f,0xf2,0xe2,0x03,0xfc,0xe0,0x4f,0xfb,0xe4,0x03,0xfe,0xff,0x4f,0xf9,0xe4,0x0f,0xff,0x87,0x27,0xf9,0xe5,0x0f,0xff,0x07,0x36,0xf9,0xe5,0x1f,0xff,0x07,0x18,0xf9,0xe4,0x3f,0xff,0x07,0x00,0xfa,0xe4,0x7f,0xfe,0x07,0x00,0x72,0xe2,0x7f,0xfc,0x03,0x00,0x06,0xe2,0x7f,0xdc,0x00,0x02,0xb8,0xe1,0xff,0x08,0x00,0x04,0x60,0xe0,0xff,0x30,0x00,0x1c,0x00,0xe0,0xff,0x60,0x80,0x03,0x00,0xc0,0x7f,0xc0,0xff,0x01,0x00,0x80,0x7f,0x00,0x00,0x01,0x00,0x80,0x3f,0x00,0x00,0x01,0x00,0x00,0x0e,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x80,0x01,0x00,0x01,0x02,0x00,0x80,0x00,0x00,0x09,0x02,0x00,0xc0,0x00,0x00,0x09,0x02,0x00,0xc0,0x00,0x00,0x09,0x02,0x00,0xc0,0x00,0x00,0x09,0x02,0x00,0xc0,0x00,0x80,0x08,0x02,0x00,0xc0,0x00,0x80,0x04,0x03,0x00,0xa0,0x00,0x80,0x04,0x03,0x00,0xa0,0x01,0x40,0x04,0x01,0x00,0x20,0x02,0x70,0x82,0x01,0x00,0x70,0xf4,0x1f,0x83,0x00,0x00,0x98,0x1d,0xf8,0xc0,0x00,0x00,0x88,0x0f,0x00,0x60,0x00,0x00,0x88,0x08,0x07,0x38,0x00,0x80,0x8f,0x08,0x3f,0x0e,0x00,0x80,0x98,0x08,0xe1,0x03,0x00,0x80,0x80,0x07,0x01,0x00,0x00,0x80,0xc1,0x0c,0x01,0x00,0x00,0x00,0x83,0x90,0x01,0x00,0x00,0x00,0xfe,0x81,0x00,0x00,0x00,0x00,0x00,0x86,0x00,0x00,0x00,0x00,0x00,0xd8,0x00,0x00,0x00,0x00,0x00,0x60,0x00,0x00,0x00}
;
const byte player_bitmap[] =
{2,27,0,0,0,0,0x0f,0x00,0x3e,0x00,0xf4,0x07,0xec,0x00,0x76,0x00,0x2b,0x00,0x33,0x00,0x75,0x00,0xf5,0x00,0xeb,0x31,0xbf,0xef,0x3f,0xcf,0xbf,0xef,0xeb,0x31,0xf5,0x00,0x75,0x00,0x33,0x00,0x2b,0x00,0x76,0x00,0xec,0x00,0xf4,0x07,0x3e,0x00,0x0f,0x00,0x00,0x00,0,0}
;
const byte bullet_bitmap2[] =
{4,2,0,0x88,0x88,0,0,0x44,0x44,0,}
;
const byte bullet_bitmap[] =
{2,2,0x88,0x88,0x44,0x44}
;
const byte enemy1_bitmap[] =
{2,24,0x0f,0x00,0x3e,0x00,0xf4,0x07,0xec,0x00,0x76,0x00,0x2b,0x00,0x33,0x00,0x75,0x00,0xf5,0x00,0xeb,0x31,0xbf,0xef,0x3f,0xcf,0xbf,0xef,0xeb,0x31,0xf5,0x00,0x75,0x00,0x33,0x00,0x2b,0x00,0x76,0x00,0xec,0x00,0xf4,0x07,0x3e,0x00,0x0f,0x00,0x00,0x00,0,0}
;

void draw_sprite(const byte* src, byte x, byte y) {
  byte i,j;
  byte* dest = &vidmem[x][y];
  byte w = *src++;
  byte h = *src++;
  for (j=0; j<h; j++) {
    for (i=0; i<w; i++) {
      *dest++ = *src++;
    }
    dest += 32-w;
  }
}

byte xor_sprite(const byte* src, byte x, byte y) {
  byte i,j;
  byte result = 0;
  byte* dest = &vidmem[x][y];
  byte w = *src++;
  byte h = *src++;
  for (j=0; j<h; j++) {
    for (i=0; i<w; i++) {
      result |= (*dest++ ^= *src++);
    }
    dest += 32-w;
  }
  return result;
}

void erase_sprite(const byte* src, byte x, byte y) {
  byte i,j;
  byte* dest = &vidmem[x][y];
  byte w = *src++;
  byte h = *src++;
  for (j=0; j<h; j++) {
    for (i=0; i<w; i++) {
      *dest++ = 0;
    }
    dest += 32-w;
  }
}

void draw_char(char ch, byte x, byte y) {
  byte i;
  const byte* src = &font8x8[(ch-LOCHAR)][0];
  byte* dest = &vidmem[x*8][y];
  for (i=0; i<8; i++) {
    *dest ^= *src;
    dest += 32;
    src += 1;
  }
}

void draw_string(const char* str, byte x, byte y) {
  do {
    byte ch = *str++;
    if (!ch) break;
    draw_char(ch, x, y);
    x++;
  } while (1);
}

byte player_x;
byte bullet_x;
byte bullet_y;

#define MAX_ENTITIES 28

typedef struct {
  byte x,y;
  const byte* shape; // need const here
} Entity;

Entity entities[MAX_ENTITIES];
byte entity_index;
byte num_entities;

typedef struct {
  byte right:1;
  byte down:1;
} MarchMode;

MarchMode this_mode, next_mode;

void init_entities() {
  int i,x,y;
  x=0;
  y=26;
  for (i=0; i<MAX_ENTITIES; i++) {
    Entity* e = &entities[i];
    e->x = x;
    e->y = y;
    e->shape = enemy1_bitmap;
    x += 28;
    if (x > 180) {
      x = 0;
      y -= 3;
    }
  }
  num_entities = MAX_ENTITIES;
  this_mode.right = 1;
  this_mode.down = 0;
  next_mode.right = 1;
  next_mode.down = 0;
}

void delete_entity(Entity* e) {
  erase_sprite(e->shape, e->x, e->y);
  memmove(e, e+1, sizeof(Entity)*(entities-e+MAX_ENTITIES-1));
  num_entities--; // update_next_entity() will check entity_index
}

void update_next_entity() {
  Entity* e;
  if (entity_index >= num_entities) {
    entity_index = 0;
    memcpy(&this_mode, &next_mode, sizeof(this_mode));
  }
  e = &entities[entity_index];
  erase_sprite(e->shape, e->x, e->y);
  if (this_mode.down) {
    e->y--;
    next_mode.down = 0;
  } else {
    if (this_mode.right) {
      e->x += 2;
      if (e->x >= 200) {
        next_mode.down = 1;
        next_mode.right = 0;
      }
    } else {
      e->x -= 2;
      if (e->x == 0) {
        next_mode.down = 1;
        next_mode.right = 1;
      }
    }
  }
  draw_sprite(e->shape, e->x, e->y);
  entity_index++;
}

void draw_bunker(byte x, byte y, byte y2, byte h, byte w) {
  byte i;
  for (i=0; i<h; i++) {
    draw_vline(x+i, y+i, y+y2+i*2);
    draw_vline(x+h*2+w-i-1, y+i, y+y2+i*2);
  }
  for (i=0; i<w; i++) {
    draw_vline(x+h+i, y+h, y+y2+h*2);
  }
}

char in_rect(Entity* e, byte x, byte y, byte w, byte h) {
  byte eh = e->shape[0];
  byte ew = e->shape[1];
  return (x >= e->x-w && x <= e->x+ew && y >= e->y-h && y <= e->y+eh);
}

Entity* find_entity_at(byte x, byte y) {
  byte i;
  for (i=0; i<num_entities; i++) {
    Entity* e = &entities[i];
    if (in_rect(e, x, y, 2, 0)) {
      return e;
    }
  }
  return NULL;
}

void check_bullet_hit(byte x, byte y) {
  Entity* e = find_entity_at(x,y);
  if (e) { 
    delete_entity(e);
  }
}

void gameloop() {
  watchdog_strobe = 0b0111 / 14;
  draw_bunker(30, 40, 15, 15, 20);
  draw_bunker(120, 40, 15, 15, 20);
  draw_sprite(enemy1_bitmap, 10, 20);
  init_entities();
  //draw_sprite(bitmap1, 0, 0);
  player_x = 96;
  while (1) {
    if (LEFT1 && player_x>0) player_x -= 2;
    if (RIGHT1 && player_x<200) player_x += 2;
    if (FIRE1 && bullet_y == 0) {
      bullet_x = player_x + 13;
      bullet_y = 3;
      xor_sprite(bullet_bitmap, bullet_x, bullet_y); // draw
    }
    draw_sprite(player_bitmap, player_x, 1);
    draw_sprite(player_bitmap, player_x, 1);
    if (bullet_y) {
      byte leftover = xor_sprite(bullet_bitmap, bullet_x, bullet_y); // erase
      if (leftover || bullet_y > 26) {
        erase_sprite(bullet_bitmap, bullet_x, bullet_y);
        check_bullet_hit(bullet_x, bullet_y+2);
        bullet_y = 0;
      } else {
        bullet_y++;
        xor_sprite(bullet_bitmap, bullet_x, bullet_y); // draw
      }
    }
    update_next_entity();
    watchdog_strobe = 0;
  }
}

void clrscr() {
  memset(vidmem, 0, sizeof(vidmem));
}

void main() {
  // TODO: clear memory
  clrscr();
  draw_string("PLAYER 1", 0, 31);
  draw_string("PLAYER 2", 20, 31);
  gameloop();
}
