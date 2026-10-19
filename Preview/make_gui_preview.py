#!/usr/bin/env python3
"""Render faithful GUI previews from the actual plugin code.

VST3 (v1.1 professional GUI): layout/colours/strings mirror
PluginEditor.cpp at its default 940x630 size — header with live
lookahead/PDC/sample-rate chips, swell waveform panel (left), swell
engine / presets / level meters panel (right), custom knob row (bottom).
The waveform itself is REAL plugin data: dry/wet peak taps exported by
VST3/Tools/TestHarness.cpp to Demo/scope_peaks.csv at the GUI's own
240 columns/sec, shown at t=3.55s with the default 2.41s window — the
violet swell crescendos into the pale dry note near the right edge.

JSFX: mirrors the @gfx section: 480x160, bg (0.06,0.05,0.12),
button rect centred 340x58 at y22, rgb (0.45,0.25,0.95).

This is a drawing of the coded UI, not a screenshot of a compiled binary
(JUCE's exact anti-aliasing/shading may differ very slightly).
"""
import csv
import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).parent
ROOT = OUT.parent
R = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
B = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

def font(size, bold=False):
    return ImageFont.truetype(B if bold else R, max(6, int(size)))

def fit_font(d, text, max_w, size, bold=False):
    while size > 6:
        f = font(size, bold)
        if d.textlength(text, font=f) <= max_w:
            return f
        size -= 0.5
    return font(6, bold)

def hx(c): return tuple(int(c[i:i+2], 16) for i in (0, 2, 4))

def load_scope_slice(now=3.55, visible=2.41, cols_per_sec=240):
    path = ROOT / "Demo" / "scope_peaks.csv"
    count = int(round(visible * cols_per_sec))
    if path.exists():
        rows = list(csv.DictReader(open(path)))
        end = int(round(now * cols_per_sec))
        seg = rows[max(0, end - count):end]
        dry = [float(r["dry_peak"]) for r in seg]
        wet = [float(r["wet_peak"]) for r in seg]
        dry = [0.0] * (count - len(dry)) + dry
        wet = [0.0] * (count - len(wet)) + wet
        return dry, wet
    # fallback shape if the CSV has not been generated yet
    dry, wet = [], []
    for i in range(count):
        t = i / count
        wet.append(0.38 * max(0.0, (t - 0.35) / 0.5) ** 2 if t < 0.86 else 0.12)
        dry.append(0.56 * math.exp(-(t - 0.86) * 9) if t >= 0.86 else 0.0)
    return dry, wet

# ---------------------------------------------------------------- VST3 GUI
def draw_vst3():
    S = 2
    W, H = 940, 630

    # diagonal background gradient #0a0718 -> #1c1136
    small = Image.new("RGB", (256, 256))
    sp = small.load()
    a, b = hx("0a0718"), hx("1c1136")
    for yy in range(256):
        for xx in range(256):
            t = (xx + yy) / 510
            sp[xx, yy] = tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))
    img = small.resize((W * S, H * S)).convert("RGBA")
    d = ImageDraw.Draw(img, "RGBA")

    def X(v): return v * S
    def panel(box, top, bottom, rad=12):
        x0, y0, x1, y1 = [X(v) for v in box]
        grad = Image.new("RGBA", (max(1, int(x1-x0)), max(1, int(y1-y0))))
        gd = ImageDraw.Draw(grad)
        for yy in range(grad.height):
            t = yy / max(1, grad.height - 1)
            gd.line([(0, yy), (grad.width, yy)],
                    fill=tuple(round(top[i] + (bottom[i]-top[i])*t) for i in range(3)) + (255,))
        mask = Image.new("L", grad.size, 0)
        ImageDraw.Draw(mask).rounded_rectangle([0, 0, grad.width, grad.height], radius=rad*S, fill=255)
        img.paste(grad, (int(x0), int(y0)), mask)
        d.rounded_rectangle([x0, y0, x1, y1], radius=rad*S, outline=(56,49,88,255), width=max(1, S))

    def text(txt, x, y, size, fill, bold=False, max_w=None, align="left"):
        f = fit_font(d, txt, X(max_w) if max_w else 10**9, X(size), bold)
        w = d.textlength(txt, font=f)
        xx = X(x) - (w/2 if align == "center" else (w if align == "right" else 0))
        d.text((xx, X(y)), txt, font=f, fill=fill)
        return f

    def section(title, x, y):
        text(title, x, y, 11.5, (255,255,255,235), bold=True)
        d.rounded_rectangle([X(x), X(y+18), X(x+28), X(y+20)], radius=S, fill=hx("7a4dff"))

    # purple header glow + fine vertical texture + frame + screws
    glow = Image.new("RGBA", (W*S, H*S), (0,0,0,0))
    gd = ImageDraw.Draw(glow)
    gd.ellipse([X(W/2-420), X(-230), X(W/2+420), X(190)], fill=(122,77,255,36))
    img.alpha_composite(glow)
    # (the coded 1.8%-alpha machining texture is barely visible in JUCE;
    #  Pillow does not alpha-blend draw calls, so it is omitted here)
    d.rectangle([0, 0, X(W)-1, X(H)-1], outline=(58,51,88,255), width=S)
    for cx, cy in [(13,13),(W-13,13),(13,H-13),(W-13,H-13)]:
        d.ellipse([X(cx-4), X(cy-4), X(cx+4), X(cy+4)], fill=hx("4a4172"), outline=(0,0,0,120))
        d.line([(X(cx-2.6), X(cy+2.6)), (X(cx+2.6), X(cy-2.6))], fill=(0,0,0,140), width=S)

    # ---------------- header (y18..80) ----------------
    d.rounded_rectangle([X(18), X(26), X(64), X(72)], radius=X(11), fill=hx("6d4bd8"))
    pts = [(25,49),(30,46),(35,54),(40,40),(45,53),(49,34),(52,62),(57,49)]
    d.line([(X(x), X(y)) for x, y in pts], fill=(255,255,255), width=2*S, joint="curve")
    text("REVERSE REVERB", 78, 24, 24, (255,255,255), bold=True, max_w=330)
    text("ONE-CLICK SWELL  -  CLASSIC LOOKAHEAD REVERSE REVERB", 79, 55, 11.5,
         hx("a89fd6"), bold=True, max_w=430)
    for (x0, w, lab, val) in [(564,118,"LOOKAHEAD","1.20 s"),
                              (690,128,"PDC LATENCY","57,600 smp"),
                              (826,96,"SAMPLE RATE","48.0 kHz")]:
        panel((x0, 31, x0+w, 67), hx("211a3d"), hx("141026"), rad=7)
        text(lab, x0+9, 35, 8.5, hx("a89fd6"), bold=True, max_w=w-18)
        text(val, x0+9, 46, 13, (255,255,255), bold=True, max_w=w-18)
    d.line([(X(18), X(80)), (X(922), X(80))], fill=(255,255,255,23))

    # ---------------- waveform panel (18,90)-(624,388) ----------------
    panel((18, 90, 624, 388), hx("100c24"), hx("090716"))
    text("SWELL WAVEFORM", 34, 100, 12.5, (255,255,255), bold=True)
    d.rounded_rectangle([X(306), X(107), X(326), X(111)], radius=2*S, fill=hx("a78bfa"))
    text("Wet swell", 332, 100, 10.5, hx("c9c2ff"))
    d.rounded_rectangle([X(402), X(107), X(422), X(111)], radius=2*S, fill=hx("dff8ff"))
    text("Delayed dry", 428, 100, 10.5, hx("c9c2ff"))
    text("2.41 s window", 608, 100, 10.5, hx("c9c2ff"), align="right")

    # plot (global): x60..608, y130..346
    px0, py0, px1, py1 = 60, 130, 608, 346
    d.rounded_rectangle([X(px0), X(py0), X(px1), X(py1)], radius=X(7),
                        fill=(3,3,10,255), outline=(52,46,82,255), width=S)
    cy = (py0 + py1) / 2
    half = (py1 - py0) / 2 - 5
    for amp, lab in [(1.0,"0"), (0.5,"-6"), (0.25,"-12"), (0.0,"dB")]:
        for sgn in ([1] if amp == 0 else [1,-1]):
            yy = cy - sgn*amp*half
            d.line([(X(px0), X(yy)), (X(px1), X(yy))],
                   fill=(64,57,92,255) if amp == 0 else (32,28,52,255))
        text(lab, 56, cy-amp*half-6, 9.5, hx("8f86bd"), align="right")
    visible = 2.41
    ago = 2.0
    while ago >= -0.001:
        gx = px1 - ago/visible*(px1-px0)
        d.line([(X(gx), X(py0)), (X(gx), X(py1))],
               fill=(74,66,104,255) if ago == 0 else (30,26,50,255))
        text("NOW" if ago == 0 else f"-{ago:.1f}s", gx, py1+4, 9.5, hx("8f86bd"), align="center")
        ago = round(ago - 0.5, 2)

    dry, wet = load_scope_slice()
    plot_w_px = int(X(px1-px0))
    def agg(vals):
        out = []
        n = len(vals)
        for i in range(plot_w_px):
            s = i*n//plot_w_px; e = max(s+1, (i+1)*n//plot_w_px)
            out.append(min(1.05, max(vals[s:e]) if s < n else 0.0))
        return out
    wetp, dryp = agg(wet), agg(dry)
    top = [(X(px0)+i, X(cy - v*half)) for i, v in enumerate(wetp)]
    bot = [(X(px0)+i, X(cy + v*half)) for i, v in reversed(list(enumerate(wetp)))]
    d.polygon(top + bot, fill=(106,61,220,255))
    d.line(top, fill=(196,181,253,255), width=S)
    d.line(bot, fill=(196,181,253,255), width=S)
    for i in range(0, plot_w_px, 2):
        a = dryp[i]*half
        if a > 0.6:
            d.line([(X(px0)+i, X(cy-a)), (X(px0)+i, X(cy+a))], fill=(223,248,255,235), width=1)
    d.line([(X(px0), X(cy)), (X(px1), X(cy))], fill=(74,66,104,255))
    d.line([(X(px1-1), X(py0)), (X(px1-1), X(py1))], fill=(70,140,165,255))
    text("swell rises  >  dry hit", px0+8, py1-19, 10, (150,143,180,255))

    # ---------------- side panel (638,90)-(922,388) ----------------
    panel((638, 90, 922, 388), hx("17112e"), hx("100b22"))
    section("SWELL ENGINE", 654, 102)
    # one-click button + LED + state line
    d.rounded_rectangle([X(654), X(130), X(906), X(186)], radius=X(11), fill=(0,0,0,90))
    d.rounded_rectangle([X(654), X(128), X(906), X(184)], radius=X(11),
                        fill=hx("7a4dff"), outline=(196,181,253,217), width=S)
    d.ellipse([X(662), X(151), X(672), X(161)], fill=hx("5eead4"))
    text("1-CLICK REVERSE SWELL", 792, 146, 15.5, (255,255,255), bold=True,
         align="center", max_w=214)
    text("ACTIVE  -  lookahead swell engaged", 780, 189, 11, hx("5eead4"),
         align="center", max_w=252)
    # presets 2x2
    section("PRESETS", 654, 204)
    for i, name in enumerate(["Vocal","Snare Pop","Guitar Wash","Synth Pad"]):
        bx = 654 + (i % 2) * 130; by = 230 + (i // 2) * 40
        d.rounded_rectangle([X(bx), X(by+2), X(bx+122), X(by+34)], radius=X(7), fill=(0,0,0,90))
        d.rounded_rectangle([X(bx), X(by), X(bx+122), X(by+32)], radius=X(7),
                            fill=hx("241d3d"), outline=(64,56,98,255), width=S)
        text(name, bx+61, by+9, 12.5, (255,255,255), bold=True, align="center", max_w=114)
    # meters
    section("LEVELS", 654, 306)
    def meter(label, lvl_db, hold_db, row_y):
        text(label, 654, row_y+2, 10, hx("a89fd6"), bold=True)
        tx0, tx1 = 688, 846
        d.rounded_rectangle([X(tx0), X(row_y+3), X(tx1), X(row_y+13)], radius=X(3), fill=(0,0,0,115))
        norm = max(0.0, min(1.0, (lvl_db + 42) / 42))
        segs = int((tx1-tx0)//8)
        for s in range(segs):
            fr = s / segs
            col = hx("34d399") if fr < 0.68 else (hx("fbbf24") if fr < 0.86 else hx("f87171"))
            if fr >= norm: col = (30,26,52,255)
            sx = tx0 + fr*(tx1-tx0)
            d.rounded_rectangle([X(sx+1), X(row_y+4.5), X(sx+(tx1-tx0)/segs-1.5), X(row_y+11.5)],
                                radius=2, fill=col)
        hxpos = tx0 + max(0.0, min(1.0, (hold_db+42)/42))*(tx1-tx0)
        d.rectangle([X(hxpos-1), X(row_y+1), X(hxpos+1), X(row_y+15)], fill=(255,255,255,217))
        text(f"{lvl_db:.1f} dB", 906, row_y+2, 10.5, (255,255,255,217), align="right")
    meter("IN", -7.5, -4.7, 332)
    meter("OUT", -8.9, -6.0, 357)

    # ---------------- controls panel (18,402)-(922,582) ----------------
    panel((18, 402, 922, 582), hx("17112e"), hx("0f0a20"))
    section("SWELL SHAPING", 34, 414)
    text("All parameters are host-automatable", 906, 415, 10.5, hx("8f86bd"), align="right")

    knobs = [
        ("Swell Length", "1.20 s", (1.20-0.30)/(3.00-0.30)),
        ("Amount", "90.0 %", 90/150),
        ("Decay", "70.0 %", 0.70),
        ("Tone", "60.0 %", 0.60),
        ("Mix", "50.0 %", 0.50),
    ]
    for i, (label, value, frac) in enumerate(knobs):
        cx = 30 + i*176 + 88
        text(label, cx, 440, 12.5, hx("c9c2ff"), bold=True, align="center", max_w=160)
        cyy, r = 504, 40
        start = 126.0  # Pillow degrees equivalent of JUCE rotary start
        sweep = 288.0
        for t in range(11):
            ang = math.radians(start + t/10*sweep)
            rr1 = r-1; rr2 = r-(7 if t % 5 == 0 else 5)
            col = (183,156,255,255) if t/10 <= frac else (82,74,116,255)
            d.line([(X(cx+rr1*math.cos(ang)), X(cyy+rr1*math.sin(ang))),
                    (X(cx+rr2*math.cos(ang)), X(cyy+rr2*math.sin(ang)))], fill=col, width=2)
        d.arc([X(cx-(r-10)), X(cyy-(r-10)), X(cx+(r-10)), X(cyy+(r-10))],
              start, start+sweep, fill=hx("292241"), width=5*S)
        d.arc([X(cx-(r-10)), X(cyy-(r-10)), X(cx+(r-10)), X(cyy+(r-10))],
              start, start+frac*sweep, fill=hx("9a6bff"), width=5*S)
        br = r-16
        d.ellipse([X(cx-br), X(cyy-br+2.5), X(cx+br), X(cyy+br+2.5)], fill=(0,0,0,115))
        d.ellipse([X(cx-br), X(cyy-br), X(cx+br), X(cyy+br)],
                  fill=hx("2e2752"), outline=(84,75,120,255), width=S)
        d.ellipse([X(cx-br*0.55), X(cyy-br*0.62), X(cx+br*0.15), X(cyy+br*0.05)],
                  fill=(70,61,108,255))
        ang = math.radians(start + frac*sweep)
        p1 = (cx + br*0.30*math.cos(ang), cyy + br*0.30*math.sin(ang))
        p2 = (cx + (br-5)*math.cos(ang), cyy + (br-5)*math.sin(ang))
        d.line([(X(p1[0]), X(p1[1])), (X(p2[0]), X(p2[1]))], fill=(255,255,255), width=3*S)
        d.ellipse([X(p2[0]-3.5), X(p2[1]-3.5), X(p2[0]+3.5), X(p2[1]+3.5)], fill=(255,255,255))
        d.ellipse([X(cx-4), X(cyy-4), X(cx+4), X(cyy+4)], fill=hx("241d3d"))
        d.rounded_rectangle([X(cx-40), X(548), X(cx+40), X(568)], radius=X(4),
                            fill=hx("0d0a1c"), outline=(62,54,96,255), width=S)
        text(value, cx, 553, 11, (255,255,255), align="center", max_w=76)

    # ---------------- footer ----------------
    text("Tip: for vocals, run this on a send at Mix 100% and ride the return so swells bloom into phrase starts.",
         18, 594, 11, hx("9d94c4"), max_w=660)
    text("Reverse Reverb One-Click  v1.1  -  Lookahead / PDC", 922, 594, 11,
         hx("6f6894"), align="right")

    p = OUT / "GUI_VST3_Preview.png"
    img.convert("RGB").save(p)
    print(p, img.size)

# ---------------------------------------------------------------- JSFX @gfx
def draw_jsfx():
    S = 2
    Wd, Hd = 480*S, 160*S
    bg = (round(0.06*255), round(0.05*255), round(0.12*255))
    img = Image.new("RGB", (Wd, Hd), bg)
    d = ImageDraw.Draw(img)
    btn = (round(0.45*255), round(0.25*255), round(0.95*255))
    d.rectangle([70*S, 22*S, 410*S, 80*S], fill=btn)
    d.text((98*S, 38*S), "REVERSE SWELL : ON", font=font(24*S), fill=(255,255,255))
    grey = (round(0.75*255), round(0.75*255), round(0.88*255))
    line1 = "Classic lookahead reverse reverb - swells INTO the note. Mixing effect (not for live monitoring)."
    line2 = "Start: leave ON, press play. Then try Swell 0.8-1.6s for vocals, 0.3-0.6s for snare."
    d.text((18*S, 100*S), line1, font=fit_font(d, line1, 444*S, 15*S), fill=grey)
    d.text((18*S, 122*S), line2, font=fit_font(d, line2, 444*S, 15*S), fill=grey)
    p = OUT / "GUI_JSFX_Preview.png"
    img.save(p)
    print(p, img.size)

draw_vst3()
draw_jsfx()
