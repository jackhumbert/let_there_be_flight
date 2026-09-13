"""Scrape Nexus Mods comments (the mod page "Posts" tab) into JSON using a real Chrome
driven over the DevTools protocol, because nexusmods.com sits behind a Cloudflare
challenge that blocks plain HTTP clients and reader proxies.

Usage:
  python tools/nexus_comments.py --mod 5208 [--since 2025-07-01] [--max-pages 80] [--out comments_5208.json]

Requires: Chrome installed, `pip install websocket-client`. Launches its own Chrome
instance with a throwaway profile on a remote-debugging port; close it when done.
"""
import argparse
import datetime as dt
import json
import os
import subprocess
import sys
import time
import urllib.request

import websocket

CHROME = r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe"
PORT = 9223
GAME_ID = 3333  # cyberpunk2077

EXTRACT_FN = r"""
(document) => {
  const out = [];
  const comments = document.querySelectorAll('li.comment');
  comments.forEach(li => {
    const id = li.id.replace('comment-', '');
    const name = li.querySelector(':scope > .comment-head .comment-name a');
    const t = li.querySelector(':scope > .comment-content time[data-date]');
    const body = li.querySelector(':scope > .comment-content .comment-content-text');
    const sticky = li.querySelector(':scope > .comment-content .sticky');
    const parentLi = li.parentElement && li.parentElement.closest && li.parentElement.closest('li.comment');
    out.push({
      id,
      author: name ? name.textContent.trim() : null,
      date: t ? parseInt(t.getAttribute('data-date'), 10) : null,
      text: body ? body.innerText.trim() : '',
      sticky: !!(sticky && sticky.style.display !== 'none'),
      reply_to: parentLi ? parentLi.id.replace('comment-', '') : null,
    });
  });
  const cnt = document.querySelector('#comment-count');
  const reload = document.querySelector('#reload-tab-url');
  return {
    title: document.title,
    count: cnt ? parseInt(cnt.getAttribute('data-comment-count'), 10) : null,
    reload: reload ? reload.value : null,
    comments: out,
  };
}
"""
EXTRACT_JS = "JSON.stringify((" + EXTRACT_FN + ")(document))"
# runs inside the Posts tab: fetch a comment page over XHR (same origin, cookies, Cloudflare clearance) and extract it
FETCH_PAGE_JS = """(async (url) => {
  const r = await fetch(url, {credentials: 'include', headers: {'X-Requested-With': 'XMLHttpRequest'}});
  const t = await r.text();
  const d = new DOMParser().parseFromString(t, 'text/html');
  const res = (%s)(d);
  res.status = r.status; res.length = t.length;
  return JSON.stringify(res);
})(%s)"""


class CDP:
    def __init__(self, ws_url):
        self.ws = websocket.create_connection(ws_url, suppress_origin=True)
        self.ws.settimeout(60)
        self.n = 0

    def call(self, method, **params):
        self.n += 1
        self.ws.send(json.dumps({"id": self.n, "method": method, "params": params}))
        while True:
            msg = json.loads(self.ws.recv())
            if msg.get("id") == self.n:
                if "error" in msg:
                    raise RuntimeError(msg["error"])
                return msg.get("result", {})

    def eval(self, expr):
        r = self.call("Runtime.evaluate", expression=expr, returnByValue=True)
        return r.get("result", {}).get("value")

    def navigate(self, url, settle=1.5, timeout=90):
        self.call("Page.navigate", url=url)
        deadline = time.time() + timeout
        while time.time() < deadline:
            time.sleep(settle)
            try:
                state = self.eval("document.readyState")
                title = self.eval("document.title") or ""
            except Exception:
                continue
            if state in ("interactive", "complete") and "Just a moment" not in title and "Attention Required" not in title:
                return title
        raise TimeoutError(f"page did not settle: {url}")


def launch_chrome(profile_dir):
    try:
        urllib.request.urlopen(f"http://127.0.0.1:{PORT}/json/version", timeout=2)
        return None
    except Exception:
        pass
    proc = subprocess.Popen([
        CHROME, f"--remote-debugging-port={PORT}", f"--user-data-dir={profile_dir}",
        "--no-first-run", "--no-default-browser-check", "--window-size=1200,900",
        "about:blank",
    ])
    for _ in range(60):
        time.sleep(0.5)
        try:
            urllib.request.urlopen(f"http://127.0.0.1:{PORT}/json/version", timeout=2)
            return proc
        except Exception:
            continue
    raise RuntimeError("chrome did not start")


def page_ws():
    targets = json.load(urllib.request.urlopen(f"http://127.0.0.1:{PORT}/json"))
    pages = [t for t in targets if t.get("type") == "page"]
    if not pages:
        raise RuntimeError("no page target")
    return pages[0]["webSocketDebuggerUrl"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mod", type=int, required=True)
    ap.add_argument("--since", default="2025-07-01", help="stop once a page's newest top-level comment is older than this (YYYY-MM-DD)")
    ap.add_argument("--max-pages", type=int, default=80)
    ap.add_argument("--out")
    ap.add_argument("--profile", default=os.path.join(os.environ.get("TEMP", "."), "nexus-chrome-profile"))
    args = ap.parse_args()
    since = int(dt.datetime.strptime(args.since, "%Y-%m-%d").replace(tzinfo=dt.timezone.utc).timestamp())
    out = args.out or f"comments_{args.mod}.json"

    proc = launch_chrome(args.profile)
    cdp = CDP(page_ws())
    cdp.call("Page.enable")
    cdp.call("Runtime.enable")

    posts = f"https://www.nexusmods.com/cyberpunk2077/mods/{args.mod}?tab=posts"
    title = cdp.navigate(posts)
    first = json.loads(cdp.eval(EXTRACT_JS))
    reload = first["reload"] or ""
    import re
    m = re.search(r"thread_id=(\d+)", reload)
    if not m:
        print("could not find thread_id on", posts, "title:", title, file=sys.stderr)
        sys.exit(1)
    thread = m.group(1)
    total = first["count"]
    print(f"mod {args.mod}: '{title}' thread {thread}, {total} comments", flush=True)

    all_comments = {}
    stop = False
    for page in range(1, args.max_pages + 1):
        # the site's RequestHelper format; a plain ?page=N query string is ignored by this endpoint
        url = ("https://www.nexusmods.com/Core/Libs/Common/Widgets/CommentContainer?RH_CommentContainer="
               f"game_id:{GAME_ID},object_id:{args.mod},object_type:1,thread_id:{thread},tabbed:1,page:{page},"
               "skip_opening_post:0,searchable:1,page_size:10")
        for attempt in range(3):
            try:
                r = cdp.call("Runtime.evaluate", expression=FETCH_PAGE_JS % (EXTRACT_FN, json.dumps(url)),
                             returnByValue=True, awaitPromise=True)
                if "exceptionDetails" in r:
                    raise RuntimeError(r["exceptionDetails"].get("text"))
                data = json.loads(r["result"]["value"])
                if data.get("status") != 200:
                    raise RuntimeError(f"HTTP {data.get('status')}")
                break
            except Exception as e:
                print(f"  page {page} attempt {attempt+1} failed: {e}", file=sys.stderr, flush=True)
                time.sleep(3)
        else:
            break
        comments = data["comments"]
        if not comments:
            print(f"  page {page}: no comments, stopping", flush=True)
            break
        for c in comments:
            all_comments[c["id"]] = c
        top = [c for c in comments if not c["reply_to"] and not c["sticky"]]
        newest = max((c["date"] or 0) for c in top) if top else 0
        oldest = min((c["date"] or 0) for c in top) if top else 0
        print(f"  page {page}: {len(comments)} comments, top-level {len(top)}, "
              f"{dt.datetime.fromtimestamp(oldest, dt.timezone.utc).date() if oldest else '?'}..{dt.datetime.fromtimestamp(newest, dt.timezone.utc).date() if newest else '?'}", flush=True)
        if top and newest < since:
            stop = True
        if stop:
            break
        time.sleep(1.0)

    result = {
        "mod": args.mod, "thread": thread, "total_comments": total, "since": args.since,
        "fetched": len(all_comments),
        "comments": sorted(all_comments.values(), key=lambda c: -(c["date"] or 0)),
    }
    for c in result["comments"]:
        if c["date"]:
            c["date_iso"] = dt.datetime.fromtimestamp(c["date"], dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    with open(out, "w", encoding="utf-8") as fh:
        json.dump(result, fh, ensure_ascii=False, indent=1)
    print(f"wrote {out}: {len(all_comments)} comments", flush=True)


if __name__ == "__main__":
    main()
