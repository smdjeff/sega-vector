#!/usr/bin/env python3

import os, sys, time
import math
import argparse
import struct
import turtle
import canvasvg
import re
from xml.dom import minidom
from svg.path import parse_path
from svg.path.path import Line, CubicBezier


parser = argparse.ArgumentParser(
    prog='svg2sega',
    description='Translates SVG to Sega G80 vector symbol',
    formatter_class=argparse.ArgumentDefaultsHelpFormatter
)
parser.add_argument('filename')
parser.add_argument('-s', '--scale', nargs='?', const=1.0, type=float, default=1.0,
                    help='scale image up or down by percentage')
parser.add_argument(      '--rotate', nargs='?', default=0, const=0, type=int, 
                    help='rotate angle in degrees')
parser.add_argument('-d', '--debug', action='store_true', help='print debug information')
parser.add_argument(      '--no-retrace', action='store_true',
                    help='disable dashed gray retrace overlay (default: enabled)')
parser.add_argument(      '--no-opt', action='store_true',
                    help='disable stroke ordering optimization to reduce beam moves')
parser.add_argument(      '--merge', nargs='?', const=1.0, type=float, default=1.0,
                    help='simplify by merging collinear adjacent segments (10+ for agressive)')
parser.add_argument(      '--rdp', nargs='?', const=1.0, type=float, default=0.0,
                    help='simplify with Ramer–Douglas–Peucker (10.0+ for agressive)')
parser.add_argument(      '--speed', type=int, default=10, help='0=full, 1=slow 10=fast')
parser.add_argument(      '--close', action='store_true', help='close window on complete')
parser.add_argument(      '--origin-left', action='store_true',
                    help='set origin to left-center of artwork (accounts for --rotate and --scale)')
parser.add_argument(      '--origin-top-left', action='store_true',
                    help='set origin to top-left of artwork; final position is top-right (consistent baseline for font glyphs)')
args = parser.parse_args()

# Used to detect breaks between subpaths.
JOIN_EPS2 = 1e-12


def extract_rgb(style):
    style = style or ""
    if "stroke: rgb" in style:
        color_part = style.split("stroke: rgb(")[1].split(")")[0]
        return tuple(map(int, color_part.split(",")))
    return (255, 255, 255)


def hex_to_rgb(hex_color):
    hex_color = (hex_color or "").lstrip('#')
    if len(hex_color) == 3:
        hex_color = ''.join(c * 2 for c in hex_color)
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))


def rgb_to_2bit(r, g, b):
    r = round(r / 85)
    g = round(g / 85)
    b = round(b / 85)
    return (r << 4) | (g << 2) | b


def apply_rotation(x, y, cx, cy, angle_deg):
    angle_rad = math.radians(angle_deg)
    cos_theta = math.cos(angle_rad)
    sin_theta = math.sin(angle_rad)
    x_new = cx + (x - cx) * cos_theta - (y - cy) * sin_theta
    y_new = cy + (x - cx) * sin_theta + (y - cy) * cos_theta
    return x_new, y_new






def inherited_stroke_rgb(node):
    """Find stroke color by walking up parents (handles stroke=#hex or rgb())."""
    def parse_color(s):
        s = (s or "").strip()
        if not s or s.lower() == "none":
            return None
        if s.startswith("#"):
            return hex_to_rgb(s)
        if s.lower().startswith("rgb(") and s.endswith(")"):
            inside = s[s.find("(") + 1:-1]
            parts = [p.strip() for p in inside.split(",")]
            if len(parts) >= 3:
                return (int(parts[0]), int(parts[1]), int(parts[2]))
        return None

    n = node
    while n is not None and getattr(n, "nodeType", None) == n.ELEMENT_NODE:
        c = parse_color(n.getAttribute("stroke") if n.hasAttribute("stroke") else "")
        if c is not None:
            return c

        style = (n.getAttribute("style") or "")
        if "stroke" in style:
            for part in style.split(";"):
                part = part.strip()
                if part.startswith("stroke:"):
                    c = parse_color(part.split(":", 1)[1].strip())
                    if c is not None:
                        return c

        n = n.parentNode

    return None


translate_re = re.compile(r"translate\(\s*([-\d.+eE]+)(?:[ ,]\s*([-\d.+eE]+))?\s*\)")


def sum_parent_translate(node):
    """Sum all translate() terms found while walking up parents (translate-only fix)."""
    tx, ty = 0.0, 0.0
    n = node
    while n is not None and getattr(n, "nodeType", None) == n.ELEMENT_NODE:
        if n.hasAttribute("transform"):
            t = n.getAttribute("transform") or ""
            for m in translate_re.finditer(t):
                tx += float(m.group(1))
                ty += float(m.group(2) or 0.0)
        n = n.parentNode
    return tx, ty


def calculate_angle_distance(x0, y0, x1, y1):
    distance = math.sqrt((x1 - x0) ** 2 + (y1 - y0) ** 2)
    distance *= float(args.scale)
    angle_radians = math.atan2(y1 - y0, x1 - x0)
    angle = math.degrees(angle_radians) + float(args.rotate)
    if angle < 0:
        angle += 360
    if angle > 360:
        angle -= 360
    return round(distance), round(angle)


def parse_transform(transform_str):
    transform_str = (transform_str or "").strip()
    cx = cy = 0.0
    angle = 0.0
    sx = sy = 1.0

    if not transform_str:
        return cx, cy, angle, sx, sy

    # 1) Detect flip/scale about a point: translate(p) scale(s) translate(-p)
    #    Example: translate(42,64) scale(-1,1) translate(-42,-64)
    m = re.search(
        r'translate\(\s*([-\d.]+)[ ,]+([-\d.]+)\s*\)\s*'
        r'scale\(\s*([-\d.]+)(?:[ ,]+([-\d.]+))?\s*\)\s*'
        r'translate\(\s*([-\d.]+)[ ,]+([-\d.]+)\s*\)',
        transform_str
    )
    if m:
        px = float(m.group(1)); py = float(m.group(2))
        sx = float(m.group(3))
        sy = float(m.group(4)) if m.group(4) is not None else sx
        nx = float(m.group(5)); ny = float(m.group(6))

        # sanity: second translate is typically -px, -py
        # If it's not, we still assume pivot is (px,py) because that's the intent.
        cx, cy = px, py

    # 2) Parse rotate(...) if present (SVG allows rotate(a) or rotate(a cx cy))
    m = re.search(r'rotate\(\s*([-\d.]+)(?:[ ,]+([-\d.]+)[ ,]+([-\d.]+))?\s*\)', transform_str)
    if m:
        angle = float(m.group(1))
        if m.group(2) is not None and m.group(3) is not None:
            cx = float(m.group(2))
            cy = float(m.group(3))

    # 3) If there is a bare scale(...) (and we didn't already capture sx/sy above), grab it.
    if sx == 1.0 and sy == 1.0:
        m = re.search(r'scale\(\s*([-\d.]+)(?:[ ,]+([-\d.]+))?\s*\)', transform_str)
        if m:
            sx = float(m.group(1))
            sy = float(m.group(2)) if m.group(2) is not None else sx

    # 4) Legacy: if we still don't have a pivot and there is translate(x,y), use it as pivot
    #    (this matches your old assumption-ish behavior)
    if cx == 0.0 and cy == 0.0:
        m = re.search(r'translate\(\s*([-\d.]+)[ ,]+([-\d.]+)\s*\)', transform_str)
        if m:
            cx = float(m.group(1))
            cy = float(m.group(2))

    return cx, cy, angle, sx, sy


def apply_scale(x, y, cx, cy, sx, sy):
    return (cx + (x - cx) * sx, cy + (y - cy) * sy)

def polyline_to_path(polyline):
    points = (polyline.getAttribute("points") or "").strip()
    if not points:
        return None

    transform = polyline.getAttribute("transform")
    cx, cy, angle, sx, sy = parse_transform(transform)

    nums = points.replace(",", " ").split()
    if len(nums) % 2 != 0:
        raise ValueError(f"Invalid number of coordinates: {len(nums)}")

    pts = []
    for i in range(0, len(nums), 2):
        x = float(nums[i])
        y = float(nums[i + 1])

        # Apply scale/flip first (SVG order in your flip case is translate->scale->translate,
        # which is equivalent to scaling about (cx,cy))
        if sx != 1.0 or sy != 1.0:
            x, y = apply_scale(x, y, cx, cy, sx, sy)

        # Then rotation
        if angle:
            x, y = apply_rotation(x, y, cx, cy, angle)

        pts.append((x, y))

    d = f"M {pts[0][0]} {pts[0][1]}"
    for x, y in pts[1:]:
        d += f" L {x} {y}"

    if args.debug:
        print(f"polyline_to_path: {d}")
    return d


def polygon_to_path(polygon):
    points = (polygon.getAttribute("points") or "").strip()
    if not points:
        return None

    # FIX: allow "x,y x,y" and "x y x y"
    coords = points.replace(",", " ").split()
    if not coords:
        return None

    d = f"M {coords[0]} {coords[1]}"
    for i in range(2, len(coords), 2):
        d += f" L {coords[i]} {coords[i + 1]}"
    d += f" L {coords[0]} {coords[1]} Z"

    if args.debug:
        print(f"polygon_to_path: {d}")
    return d


def draw_dashed_line(t, x0, y0, x1, y1, dash=8.0, gap=6.0):
    """Draw a dashed line from (x0,y0) to (x1,y1) with turtle t."""
    dx = x1 - x0
    dy = y1 - y0
    dist = math.hypot(dx, dy)
    if dist <= 1e-9:
        return
    ux = dx / dist
    uy = dy / dist

    pos = 0.0
    t.penup()
    while pos < dist:
        # dash segment
        a = pos
        b = min(pos + dash, dist)
        t.goto(x0 + ux * a, y0 + uy * a)
        t.pendown()
        t.goto(x0 + ux * b, y0 + uy * b)
        t.penup()
        pos = b + gap

sega_vector_count = 0

def printSegaVector(sega_color, x0, y0, x1, y1):
    distance, angle = calculate_angle_distance(x0, y0, x1, y1)
    while distance > 0:
        sega_size = min(distance, 0xFF)
        distance -= sega_size
        sega_angle = int(angle * 1024 / 360)
        sega_angle_lsb = sega_angle & 0xFF
        sega_angle_msb = (sega_angle >> 8) & 0xFF
        print("   0x{0:02x}, 0x{1:02x}, 0x{2:02x}, 0x{3:02x},".format(
            sega_color if distance == 0 else (sega_color & 0x7F),
            sega_size, sega_angle_lsb, sega_angle_msb
        ))
        global sega_vector_count
        sega_vector_count += 1


def dist2(a, b):
    dx = a[0] - b[0]
    dy = a[1] - b[1]
    return dx * dx + dy * dy


def order_strokes_greedy(strokes, start_pt=(0.0, 0.0)):
    """
    Greedy nearest-neighbor stroke ordering with per-stroke reversal.
    Returns list of (stroke_index, reversed_bool).
    """
    remaining = set(range(len(strokes)))
    order = []
    cur = start_pt

    while remaining:
        best_i = None
        best_rev = False
        best_d = None

        for i in remaining:
            pts = strokes[i]["pts"]
            a = pts[0]
            b = pts[-1]
            d_a = dist2(cur, a)
            d_b = dist2(cur, b)

            if best_d is None or d_a < best_d:
                best_d = d_a
                best_i = i
                best_rev = False
            if d_b < best_d:
                best_d = d_b
                best_i = i
                best_rev = True

        order.append((best_i, best_rev))
        remaining.remove(best_i)
        pts = strokes[best_i]["pts"]
        cur = pts[0] if best_rev else pts[-1]

    return order


def total_jump_cost(strokes, order, start_pt=(0.0, 0.0)):
    cur = start_pt
    cost = 0.0
    for idx, rev in order:
        pts = strokes[idx]["pts"]
        entry = pts[-1] if rev else pts[0]
        exitp = pts[0] if rev else pts[-1]
        cost += math.sqrt(dist2(cur, entry))
        cur = exitp
    return cost


def two_opt(order, strokes, start_pt=(0.0, 0.0), max_passes=2):
    """
    Lightweight 2-opt improvement.
    When reversing a block, also flips the reversal flags so endpoints stay consistent.
    """
    best = order[:]
    best_cost = total_jump_cost(strokes, best, start_pt)
    n = len(best)

    for _ in range(max_passes):
        improved = False
        for i in range(0, n - 1):
            for k in range(i + 1, n):
                cand = best[:]
                cand[i:k + 1] = [(idx, not rev) for (idx, rev) in reversed(cand[i:k + 1])]
                c = total_jump_cost(strokes, cand, start_pt)
                if c + 1e-9 < best_cost:
                    best = cand
                    best_cost = c
                    improved = True
        if not improved:
            break

    return best


def _point_line_dist2(p, a, b):
    # squared distance from p to infinite line through a-b
    ax, ay = a; bx, by = b; px, py = p
    vx, vy = bx-ax, by-ay
    wx, wy = px-ax, py-ay
    denom = vx*vx + vy*vy
    if denom <= 1e-18:
        return (px-ax)*(px-ax) + (py-ay)*(py-ay)
    # area^2 / |v|^2
    cross = vx*wy - vy*wx
    return (cross*cross) / denom


def merge_collinear_points(pts, tol=0.0):
    """
    Remove interior points that lie on (or near) the line between neighbors.
    tol is distance tolerance in same units as pts.
    """
    if len(pts) <= 2:
        return pts

    tol2 = tol * tol
    out = [pts[0]]
    for i in range(1, len(pts)-1):
        a = out[-1]
        b = pts[i]
        c = pts[i+1]

        # if b is close to line a-c, drop it
        if _point_line_dist2(b, a, c) <= tol2:
            continue
        out.append(b)
    out.append(pts[-1])
    return out


def _rdp_recursive(pts, eps2):
    # returns indices to keep
    a = pts[0]
    b = pts[-1]
    max_d2 = -1.0
    idx = -1
    for i in range(1, len(pts)-1):
        d2 = _point_line_dist2(pts[i], a, b)
        if d2 > max_d2:
            max_d2 = d2
            idx = i
    if max_d2 <= eps2 or idx < 0:
        return [0, len(pts)-1]
    left = _rdp_recursive(pts[:idx+1], eps2)
    right = _rdp_recursive(pts[idx:], eps2)
    # stitch, avoiding duplicate idx
    return left[:-1] + [i + idx for i in right]


def simplify_rdp(pts, eps):
    """Ramer–Douglas–Peucker for polyline points."""
    if len(pts) <= 2:
        return pts
    keep = sorted(set(_rdp_recursive(pts, eps*eps)))
    return [pts[i] for i in keep]


def circle_to_path(circle, segments=32):
    cx = float(circle.getAttribute("cx") or 0)
    cy = float(circle.getAttribute("cy") or 0)
    r  = float(circle.getAttribute("r")  or 0)
    if r <= 0:
        return None

    transform = circle.getAttribute("transform")
    tcx, tcy, angle, sx, sy = parse_transform(transform)

    pts = []
    for i in range(segments + 1):
        a = 2 * math.pi * i / segments
        x = cx + r * math.cos(a)
        y = cy + r * math.sin(a)
        if sx != 1.0 or sy != 1.0:
            x, y = apply_scale(x, y, tcx, tcy, sx, sy)
        if angle:
            x, y = apply_rotation(x, y, tcx, tcy, angle)
        pts.append((x, y))

    d = f"M {pts[0][0]} {pts[0][1]}"
    for x, y in pts[1:]:
        d += f" L {x} {y}"
    return d


def element_to_path_d(element):
    if element.tagName == "polyline":
        return polyline_to_path(element)
    if element.tagName == "polygon":
        return polygon_to_path(element)
    if element.tagName == "circle":
        return circle_to_path(element)
    return element.getAttribute("d")


def split_path_into_strokes(d):
    """
    Convert a path d into one or more 'stroke' point lists.
    We split when a segment start doesn't match previous end (subpath break).
    Each stroke is list of (x,y) points: [start, p1, p2, ...]
    """
    strokes = []
    cur_pts = []
    last_end = None

    for seg in parse_path(d):
        if isinstance(seg, CubicBezier):
            seg = Line(seg.start, seg.end)

        if not isinstance(seg, Line):
            continue

        x0, y0 = seg.start.real, seg.start.imag
        x1, y1 = seg.end.real, seg.end.imag

        if last_end is None:
            cur_pts = [(x0, y0), (x1, y1)]
        else:
            if dist2((x0, y0), last_end) > JOIN_EPS2:
                # start a new stroke
                if len(cur_pts) >= 2:
                    strokes.append(cur_pts)
                cur_pts = [(x0, y0), (x1, y1)]
            else:
                cur_pts.append((x1, y1))

        last_end = (x1, y1)

    if len(cur_pts) >= 2:
        strokes.append(cur_pts)

    return strokes


# ============================================================

doc = minidom.parse(args.filename)
paths = doc.getElementsByTagName("path")
polylines = doc.getElementsByTagName("polyline")
polygons = doc.getElementsByTagName("polygon")
circles = doc.getElementsByTagName("circle")
elements = list(paths) + list(polylines) + list(polygons) + list(circles)

# PASS 1: build raw strokes (translated but not centered), and compute bounds
raw_strokes = []
minx = miny = float("inf")
maxx = maxy = float("-inf")

for element in elements:
    # color (inherited)
    c = inherited_stroke_rgb(element)
    if c is None:
        c = extract_rgb(element.getAttribute("style")) if element.hasAttribute("style") else (255, 255, 255)

    sega_color = rgb_to_2bit(*c) << 1

    if args.debug:
        print(element.toxml())

    # translate-only group offsets
    ptx, pty = sum_parent_translate(element)

    d = element_to_path_d(element)
    if not d:
        continue

    for stroke_pts in split_path_into_strokes(d):
        # apply parent translate now (still SVG coords)
        pts_t = []
        for (x, y) in stroke_pts:
            x2 = x + ptx
            y2 = y + pty
            pts_t.append((x2, y2))
            minx = min(minx, x2)
            maxx = max(maxx, x2)
            miny = min(miny, y2)
            maxy = max(maxy, y2)

        raw_strokes.append({
            "pts": pts_t,
            "rgb": c,
            "sega_color": sega_color,
        })

# Compute origin of artwork bounds
if minx == float("inf"):
    cx = cy = 0.0
else:
    if args.origin_top_left:
        cx = minx
        cy = miny
    elif args.origin_left:
        cx = minx
        cy = (miny + maxy) / 2.0
    else:
        cx = (minx + maxx) / 2.0
        cy = (miny + maxy) / 2.0

# PASS 2: center into final strokes
strokes = []
for st in raw_strokes:
    pts = []
    for (x, y) in st["pts"]:
        x -= cx
        y -= cy
        pts.append((x, y))
    strokes.append({
        "pts": pts,
        "rgb": st["rgb"],
        "sega_color": st["sega_color"],
    })

# PASS 3: simplfy strokes
for st in strokes:
    pts = st["pts"]
    if args.merge > 0.0:
        pts = merge_collinear_points(pts, tol=args.merge)
    if args.rdp > 0.0:
        pts = simplify_rdp(pts, eps=args.rdp)
    st["pts"] = pts


# Optimize stroke order to reduce beam moves
if args.no_opt or len(strokes) <= 1:
    order = [(i, False) for i in range(len(strokes))]
else:
    order = order_strokes_greedy(strokes, start_pt=(0.0, 0.0))
    order = two_opt(order, strokes, start_pt=(0.0, 0.0), max_passes=5)

# PASS 4: apply order + stitch touching strokes to eliminate retraces
def stitch_ordered_strokes(order, strokes, eps=1.0):
    eps2 = eps * eps
    result = []
    for idx, rev in order:
        st = strokes[idx]
        pts = list(reversed(st["pts"])) if rev else list(st["pts"])
        if result and dist2(result[-1]["pts"][-1], pts[0]) <= eps2:
            result[-1]["pts"] = result[-1]["pts"] + pts[1:]
        else:
            result.append({"pts": pts, "rgb": st["rgb"], "sega_color": st["sega_color"]})
    return result

stitched = stitch_ordered_strokes(order, strokes, eps=args.merge)

# Turtle setup
wn = turtle.Screen()
wn.bgcolor("black")
wn.title("Sega Vectors")
wn.colormode(255)

skk = turtle.Turtle()
skk.speed( args.speed )
skk.pensize(2)
skk.pendown()

x3, y3 = 0.0, 0.0  # start at origin after centering

# Draw + emit Sega vectors in optimized order
for st in stitched:
    pts = st["pts"]
    color = st["rgb"]
    sega_color = st["sega_color"]

    # Teleport (beam-off) to stroke start if needed
    wn.tracer(0, 0)
    sx, sy = pts[0]
    if sx != x3 or sy != y3:
        if not args.no_retrace:
            old_pen = skk.pen()
            skk.pen(pendown=False)
            skk.pensize(1)
            skk.pencolor(120, 120, 120)
            draw_dashed_line(skk, x3, y3, sx, sy, dash=10.0, gap=8.0)
            skk.pen(old_pen)
        skk.teleport(sx, sy)
        printSegaVector(sega_color, x3, y3, sx, sy)
        # keep sega vector stream in sync with turtle position
        x3, y3 = sx, sy
    if (args.speed): wn.tracer(1, args.speed)

    # Draw polyline (beam-on for each segment)
    skk.pencolor(color)
    skk.goto(sx, sy)
    for (nx, ny) in pts[1:]:
        skk.goto(nx, ny)
        printSegaVector(sega_color | 0x01, x3, y3, nx, ny)
        x3, y3 = nx, ny
    wn.update()

base = os.path.basename(args.filename)
name = os.path.splitext(base)[0]
macro = re.sub(r'[^A-Za-z0-9]+', '_', name).strip('_').upper()

# return to origin (or bottom-right for --origin-bottom-left) to make subsequent draws easier
if args.origin_top_left and minx != float("inf"):
    final_x = maxx - cx + 25   # width + letter gap
    final_y = 0.0               # top-right: same y as origin
else:
    final_x = 0.0
    final_y = 0.0
printSegaVector(sega_color | 0x80, x3, y3, final_x, final_y)
print(f'#define V_{macro}_SZ {sega_vector_count}')

doc.unlink()
skk.hideturtle()
if not args.close:
    wn.mainloop()
