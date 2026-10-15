#!/usr/bin/env python3
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / "src" / "battery_percent.c"
OUTPUT = ROOT / "docs"

THEMES = {
    "light": {
        "surface": "#fcfcfb",
        "border": "#e4e3df",
        "grid": "#ecebe8",
        "axis": "#c9c8c2",
        "primary": "#0b0b0b",
        "secondary": "#52514e",
        "muted": "#73726c",
        "series": {
            "lipo": "#2a78d6",
            "li_ion": "#eb6834",
            "lifepo4": "#1baf7a",
            "nimh": "#eda100",
            "alkaline": "#e87ba4",
        },
    },
    "dark": {
        "surface": "#1a1a19",
        "border": "#2e2e2c",
        "grid": "#262625",
        "axis": "#3d3d3a",
        "primary": "#f0efec",
        "secondary": "#c3c2b7",
        "muted": "#9b9a92",
        "series": {
            "lipo": "#3987e5",
            "li_ion": "#d95926",
            "lifepo4": "#199e70",
            "nimh": "#c98500",
            "alkaline": "#d55181",
        },
    },
}

NAMES = {
    "lipo": "LiPo",
    "li_ion": "Li-ion 18650",
    "lifepo4": "LiFePO4",
    "nimh": "NiMH",
    "alkaline": "Alkaline",
}

PANELS = [
    {
        "title": "Lithium cells",
        "curves": ["lipo", "li_ion", "lifepo4"],
        "range": (2.4, 4.3),
        "ticks": [2.5, 3.0, 3.5, 4.0],
        "labels": {"lipo": (40, "above"), "li_ion": (40, "below"), "lifepo4": (70, "below")},
    },
    {
        "title": "AA cells",
        "curves": ["nimh", "alkaline"],
        "range": (0.8, 1.6),
        "ticks": [0.9, 1.1, 1.3, 1.5],
        "labels": {"nimh": (20, "above"), "alkaline": (20, "below")},
    },
]

WIDTH = 960
HEIGHT = 440
FONT = "-apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif"


def read_curves():
    text = SOURCE.read_text()
    curves = {}
    for name in NAMES:
        match = re.search(rf"{name}_points\[\] = \{{(.*?)\}};", text, re.S)
        points = [(int(mv) / 1000, int(pct)) for mv, pct in re.findall(r"\{(\d+), (\d+)\}", match.group(1))]
        curves[name] = points
    return curves


def voltage_at(points, percent):
    for (v0, p0), (v1, p1) in zip(points, points[1:]):
        if p0 <= percent <= p1:
            return v0 + (v1 - v0) * (percent - p0) / (p1 - p0)
    return points[-1][0]


def render(theme_name, curves):
    theme = THEMES[theme_name]
    out = []
    add = out.append
    add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" viewBox="0 0 {WIDTH} {HEIGHT}" role="img" aria-labelledby="title desc">')
    add('<title id="title">Battery voltage to state of charge curves built into battery-percent</title>')
    add('<desc id="desc">Open-circuit voltage against state of charge for LiPo, Li-ion 18650 and LiFePO4 cells, and for NiMH and alkaline AA cells.</desc>')
    add(f'<rect x="0.5" y="0.5" width="{WIDTH - 1}" height="{HEIGHT - 1}" rx="12" fill="{theme["surface"]}" stroke="{theme["border"]}"/>')
    add(f'<g font-family="{FONT}">')
    add(f'<text x="32" y="44" font-size="20" font-weight="600" fill="{theme["primary"]}">Voltage in, battery percentage out</text>')
    add(f'<text x="32" y="68" font-size="14" fill="{theme["secondary"]}">Built-in discharge curves, per cell, from datasheets and lab measurements</text>')

    plot_width = 360
    plot_height = 250
    top = 130
    for index, panel in enumerate(PANELS):
        left = 88 + index * 470
        low, high = panel["range"]

        def x_of(percent):
            return left + plot_width * percent / 100

        def y_of(volts):
            return top + plot_height * (high - volts) / (high - low)

        add(f'<text x="{left}" y="{top - 34}" font-size="14" font-weight="600" fill="{theme["primary"]}">{panel["title"]}</text>')
        legend_x = left
        for name in panel["curves"]:
            color = theme["series"][name]
            add(f'<line x1="{legend_x}" y1="{top - 16}" x2="{legend_x + 14}" y2="{top - 16}" stroke="{color}" stroke-width="2" stroke-linecap="round"/>')
            add(f'<text x="{legend_x + 20}" y="{top - 12}" font-size="12" fill="{theme["secondary"]}">{NAMES[name]}</text>')
            legend_x += 34 + len(NAMES[name]) * 7

        for tick in panel["ticks"]:
            y = y_of(tick)
            add(f'<line x1="{left}" y1="{y:.1f}" x2="{left + plot_width}" y2="{y:.1f}" stroke="{theme["grid"]}" stroke-width="1"/>')
            add(f'<text x="{left - 10}" y="{y + 4:.1f}" font-size="12" text-anchor="end" fill="{theme["muted"]}">{tick:.1f} V</text>')
        add(f'<line x1="{left}" y1="{top + plot_height}" x2="{left + plot_width}" y2="{top + plot_height}" stroke="{theme["axis"]}" stroke-width="1"/>')
        for percent in (0, 25, 50, 75, 100):
            add(f'<text x="{x_of(percent):.1f}" y="{top + plot_height + 20}" font-size="12" text-anchor="middle" fill="{theme["muted"]}">{percent}%</text>')
        add(f'<text x="{left + plot_width / 2}" y="{top + plot_height + 42}" font-size="12" text-anchor="middle" fill="{theme["secondary"]}">State of charge</text>')

        for name in panel["curves"]:
            path = " ".join(
                f'{"M" if i == 0 else "L"}{x_of(p):.1f},{y_of(v):.1f}' for i, (v, p) in enumerate(curves[name])
            )
            add(f'<path d="{path}" fill="none" stroke="{theme["series"][name]}" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>')

        for name, (percent, side) in panel["labels"].items():
            x = x_of(percent)
            y = y_of(voltage_at(curves[name], percent)) + (-10 if side == "above" else 20)
            add(f'<text x="{x:.1f}" y="{y:.1f}" font-size="12" font-weight="600" text-anchor="middle" fill="{theme["primary"]}" stroke="{theme["surface"]}" stroke-width="4" paint-order="stroke">{NAMES[name]}</text>')

    add("</g>")
    add("</svg>")
    return "\n".join(out) + "\n"


def main():
    curves = read_curves()
    OUTPUT.mkdir(exist_ok=True)
    for theme in THEMES:
        (OUTPUT / f"curves-{theme}.svg").write_text(render(theme, curves))


if __name__ == "__main__":
    main()
