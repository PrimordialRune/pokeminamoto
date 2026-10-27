#!/usr/bin/env python3
"""Screenshot tour of the editor (needs `pip install playwright && playwright install chromium`).

    python3 tools/editor/tests/tour.py OUT_DIR [--video]

Starts the server on a free port with its saves in a temp directory, walks the four editors, edits a trainer level,
simulates and screenshots the win-rate band. Nothing is written to the repo.
"""
import os
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from medit import paths, server  # noqa: E402


def main():
    out = os.path.abspath(sys.argv[1])
    video = "--video" in sys.argv
    os.makedirs(out, exist_ok=True)
    from playwright.sync_api import sync_playwright
    tmp = tempfile.mkdtemp(prefix="medit-tour-")
    srv = server.make_server(paths.Store(out=tmp), 0)
    port = srv.server_address[1]
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    base = "http://127.0.0.1:%d/" % port
    errors = []
    with sync_playwright() as p:
        b = p.chromium.launch()
        ctx = b.new_context(viewport={"width": 1280, "height": 900}, record_video_dir=os.path.join(out, "video") if video else None,
                            record_video_size={"width": 1280, "height": 900})
        pg = ctx.new_page()
        pg.on("console", lambda m: errors.append(m.text) if m.type == "error" and "fonts.g" not in m.text and "ERR_" not in m.text else None)
        pg.on("pageerror", lambda e: errors.append(str(e)))
        shot = lambda name: pg.screenshot(path=os.path.join(out, name + ".png"))   # noqa: E731
        pg.goto(base + "#/trainers")
        pg.wait_for_selector(".li")
        time.sleep(0.6); shot("01-trainers-list")
        pg.goto(base + "#/trainers/TRAINER_YUJIROU")
        pg.wait_for_selector(".mon")
        time.sleep(0.8); shot("02-trainer-yujirou")
        # edit: a three-bird party at the design's levels (Yujirou is a stub: one Lv 2 Rattata)
        lv = pg.locator("input[aria-label=level]").first
        lv.fill("9"); lv.dispatch_event("change")
        pg.select_option(".mon select[aria-label=species]", label="Spearow")
        time.sleep(0.6); shot("03-trainer-edited")
        pg.get_by_role("button", name="Simulate this fight").click()
        pg.wait_for_selector(".egauge", timeout=240000)
        time.sleep(0.5)
        pg.locator(".sim").first.scroll_into_view_if_needed(); shot("04-trainer-simulated-band")
        pg.get_by_role("button", name="Save", exact=True).click()
        time.sleep(1.0); shot("05-trainer-saved")
        for name, url, sel in (("06-learnset-pikachu", "#/species/PIKACHU", "table.ed"), ("07-encounters-route101", "#/encounters/0", "table.ed"),
                               ("08-arena-knobs", "#/arena", "table.ed"), ("09-saved-diff", "#/saved", "pre.diff")):
            pg.goto(base + url)
            pg.wait_for_selector(sel)
            time.sleep(0.7); shot(name)
        pg.goto(base + "#/encounters/0")
        pg.wait_for_selector("table.ed")
        pg.get_by_role("button", name="Simulate this route").click()
        pg.wait_for_selector(".egauge", timeout=240000)
        time.sleep(0.5); pg.locator(".sim").first.scroll_into_view_if_needed(); shot("10-route-simulated")
        ctx.close(); b.close()
    srv.shutdown()
    print("errors:", errors or "none")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
