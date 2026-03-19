#!/usr/bin/env python3

import argparse
import struct
import turtle
import canvasvg
import os,sys,time, io, re, math
from collections import namedtuple

def hex_int(x):
    return int(x, 16)

parser = argparse.ArgumentParser(
                    prog='sega2svg',
                    description='Translate Sega G80 vector symbols to SVG',
 formatter_class=argparse.ArgumentDefaultsHelpFormatter
)
parser.add_argument('filename')
parser.add_argument('-r', '--render', type=hex_int, help='render vector symbol at address')
parser.add_argument('-w', '--write', action='store_true', help='write vector symbols to disk as svg')
parser.add_argument('-s', '--search', nargs='?', const=3, type=int, help='search ROM for symbols of at least x vectors')
parser.add_argument(      '--speed', type=int, default=10, help='0=full, 1=slow 10=fast')
parser.add_argument(      '--rotate', nargs='?', default=90, const=0, type=int, help='rotate 90 degrees, typical for sega roms')
parser.add_argument('-t', '--text', action='store_true',
                    help='treat input as text/C header; extract 0xNN bytes instead of reading raw binary')
parser.add_argument('-d', '--debug', action='store_true', help='print debug information')
args = parser.parse_args()



def decodeSegaVector( addr ):
    fp.seek( addr )

    while True:
        stat = struct.unpack('<B', fp.read(1))[0]
        stat_last = stat & 0x80
        stat_visible = stat & 0x01
        stat_symbol = (stat & 0x7E) >> 1
        x = struct.unpack('<H', fp.read(2))[0]
        y = struct.unpack('<H', fp.read(2))[0]
        addr = struct.unpack('<H', fp.read(2))[0]
        angle = struct.unpack('<H', fp.read(2))[0]
        angle_deg = int(round(angle/2.845))
        size = struct.unpack('<B', fp.read(1))[0]
        scale = round((size / 0x80),1)

        print(f"{x},{y} angle:{angle_deg}\N{DEGREE SIGN} scale:{scale} stat:{stat:0>2x} addr:{addr:0>4x}")

        if (x<512 or x>1536):
            print('x value out of range')
            return

        if (y<512 or y>1536):
            print('y value out of range')
            return

        if (angle_deg > 360):
            print('angle value out of range')
            return

        if (addr<0xe000 or addr>0xefff):
            print('addrress value out of range')
            return

        if (stat == 0x81 ):
            print()
            break


def segaSixBitColor( value ):
    r = (value >> 4) & 0x03
    g = (value >> 2) & 0x03
    b = (value >> 0) & 0x03
    return (r * 64, g * 64, b * 64)


Vector = namedtuple('Vector',['byte_data','last','visible','color','angle','length'])


def decodeSegaSymbol( addr ):
    vectors = []

    while ( addr + 4 < end_of_file ):

        fp.seek(addr)
        byte_data = fp.read(4)

        if (args.debug):
            print()
            print(f"{addr:0>4x} ", end="")
            for byte in byte_data:
                print(f"{byte:0>2x}", end=" ")

        fp.seek(addr)
        stat = struct.unpack('<B', fp.read(1))[0]
        length = struct.unpack('<B', fp.read(1))[0] * 2
        angle = struct.unpack('<H', fp.read(2))[0]
        addr += 4

        last = bool( stat & 0x80 )
        visible = bool( stat & 0x01 )
        color = segaSixBitColor( (stat & 0x7E) >> 1 )

        if (angle > 0x03ff):
            mystery = [0x45,0x5d,0x41,4,5,6,7]
            if ( angle >> 8 in mystery ):
                if (args.debug):
                    print(f"suspicious angle",end="")
                angle &= 0x03ff
            else:
                if (args.debug):
                    print(f"rejected angle")
                return []

        angle = int(round(angle/2.845))

        vectors.append( Vector( byte_data, last, visible, color, angle, length ) )

        if (last):
            break

    return vectors


def drawVector( visible, color, length ):
    if (visible):
        skk.color( color )
        skk.forward( length )
    else:
        skk.color('light gray')
        cycles = int(length / 10.0)
        if cycles:
            step = length / cycles / 2.0
            for i in range(cycles):
                skk.penup()
                skk.forward( step )
                skk.pendown()
                skk.forward( step )
        else:
            skk.forward( length )



def drawSegaSymbol(vectors, speed):
    # contain/letter box non-square image
    minx, miny, maxx, maxy = compute_bbox(vectors)
    pad = 20.0
    w = max(1.0, (maxx - minx))
    h = max(1.0, (maxy - miny))
    win_w = float(wn.window_width())
    win_h = float(wn.window_height())
    aspect = win_w / win_h if win_h > 0 else 1.0
    cx = (minx + maxx) * 0.5
    cy = (miny + maxy) * 0.5
    world_w = w
    world_h = h
    if (world_w / world_h) > aspect:
        world_h = world_w / aspect
    else:
        world_w = world_h * aspect
    x0 = cx - world_w * 0.5 - pad
    x1 = cx + world_w * 0.5 + pad
    y0 = cy - world_h * 0.5 - pad
    y1 = cy + world_h * 0.5 + pad

    wn.setworldcoordinates(x0, y0, x1, y1)

    skk.reset()
    skk.pensize(2)
    skk.speed( 0 if speed == 0 else speed )
    skk.hideturtle()
    skk.setundobuffer(None)
    skk.penup()
    skk.goto(0, 0)
    skk.pendown()

    if (speed == 0):
        wn.tracer(0, 0)
        update_every = 0
    else:
        wn.tracer(1, speed)
        update_every = 0

    skk.color('gray')
    skk.dot(8)

    ix = 0
    for v in vectors:
        skk.setheading( float(args.rotate) - v.angle)
        drawVector(v.visible, v.color, v.length/2.0)
        if (args.debug):
            skk.color('cyan')
            skk.write(f"{ix}")
        ix += 1
        drawVector(v.visible, v.color, v.length/2.0)
        if update_every and (ix % update_every == 0):
            wn.update()

    wn.update()



HEX_BYTE_RE = re.compile(r'0x([0-9A-Fa-f]{2})')
def read_text_as_bytes(path):
    s = open(path, 'r', encoding='utf-8', errors='replace').read()
    hex_pairs = HEX_BYTE_RE.findall(s)
    if not hex_pairs:
        raise SystemExit(f"No 0xNN byte tokens found in {path}")
    return bytes(int(h, 16) for h in hex_pairs)

def compute_bbox(vectors):
    x = y = 0.0
    minx = maxx = x
    miny = maxy = y

    for v in vectors:
        # draw v.length/2 twice => total step = v.length
        L = float(v.length)
        theta = math.radians(float(args.rotate) - v.angle)
        x += math.cos(theta) * L
        y += math.sin(theta) * L
        minx = min(minx, x); maxx = max(maxx, x)
        miny = min(miny, y); maxy = max(maxy, y)

    return minx, miny, maxx, maxy


####################################################

print()

if args.text:
    data = read_text_as_bytes(args.filename)
    fp = io.BytesIO(data)
else:
    fp = open(args.filename, mode='rb')

fp.seek(0, os.SEEK_END)
end_of_file = fp.tell()
fp.seek(0)

if ( args.write ):
    if not os.path.exists('dump'):
        os.makedirs('dump')

if ( args.render or args.write ):
    wn = turtle.Screen()
    wn.bgcolor("black")
    wn.title("Sega Vectors")
    wn.colormode(255)
    skk = turtle.Turtle()


addrs = []
if ( args.render ):
    addrs.append( args.render )

if (args.search):
    symbol_count = 0
    addr = 0x0000
    last_was_good = False
    while addr + 4 < end_of_file:
        vectors = decodeSegaSymbol( addr )
        if (args.debug):
            print()
        # results in some false positives, but good enough
        if vectors and ((last_was_good and len(vectors)>1) or (not last_was_good and len(vectors)>args.search)):
            symbol_count += 1
            last_was_good = True
            addrs.append( addr )
            if (args.debug):
                print(f"{addr:0>4x} GOOD")
            else:
                print(f"0x{addr:0>4x}",end=",")
            addr += len(vectors)*4
            fp.seek( addr )
        else:
            last_was_good = False
            addr += 1
    print()
    print(f"total symbols:{symbol_count}")

for addr in addrs:
        vectors = decodeSegaSymbol( addr )
        if vectors and len(vectors):
            print(f"{addr:0>4x} {len(vectors)} vectors")
            if len(vectors) > 2:
                if ( args.write ):
                    drawSegaSymbol( vectors, args.speed )
                    canvasvg.saveall( f"dump/{addr:0>4x}.svg", turtle.getcanvas() )
                if ( args.render ):
                    drawSegaSymbol( vectors, args.speed )
        else:
            print(f"{addr:0>4x}  no vector data")

if ( args.render ):
    wn.mainloop()
