#!/usr/bin/env python3
"""Generate the desktop client's Region menu, and the GEOS client's region panel, from the servers fly.io runs,
so they can't drift.

Usage:
    gen_regions.py h      print C header (desktop/src/regions_tab.h)
    gen_regions.py c [--config FILE] [--host HOST] [--seats-port N]
                          print the GEOS client's (client/build/geos/regions_tab.h): FujiNet devicespecs. A test
                          build gives a servers file of its own, the host its servers listen on and its web port.

Each [[server]] in server/servers.fly.toml is one entry: its region, its pace (description) and its port, all
dialled at the [lobby] address, the public host name. REGION_SEATS_PORT is where that host answers GET /SEATS
(the seats taken at each server) in plain HTTP: the fly.toml port, without TLS, of the service for [web]."""
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FLY = ROOT / "server" / "servers.fly.toml"
FLY_APP = ROOT / "fly.toml"


def seats_port(web_port):
    for service in tomllib.loads(FLY_APP.read_text()).get("services", []):
        if service.get("internal_port") == web_port:
            for p in service.get("ports", []):
                if p.get("handlers") == ["http"] and not p.get("force_https"):
                    return p["port"]
    sys.exit("fly.toml: no plain http port for the [web] service")


def gen_h():
    conf = tomllib.loads(FLY.read_text())
    host = conf["lobby"]["address"]
    L = ["/* GENERATED from server/servers.fly.toml and fly.toml by tools/gen_regions.py -- do not edit. */",
         "#ifndef REGIONS_TAB_H", "#define REGIONS_TAB_H", "",
         f'#define REGION_HOST "{host}"',
         f"#define REGION_SEATS_PORT {seats_port(conf['web']['port'])}",
         f"#define NREGIONS {len(conf['server'])}", "",
         "struct region_entry { const char *name, *pace; unsigned short port; };",
         "static const struct region_entry regions[NREGIONS] = {"]
    for s in conf["server"]:
        L.append(f'    {{ "{s["region"]}", "{s.get("description", "")}", {s["port"]} }},')
    L += ["};", "", "#endif", ""]
    return "\n".join(L)


def gen_c(args):
    opts = dict(zip(args[::2], args[1::2]))
    if len(args) % 2 or set(opts) - {"--config", "--host", "--seats-port"}:
        sys.exit(__doc__)
    conf = tomllib.loads(Path(opts.get("--config", FLY)).read_text())
    host = opts.get("--host") or conf["lobby"]["address"]
    port = int(opts["--seats-port"]) if "--seats-port" in opts else seats_port(conf["web"]["port"])
    servers = conf["server"]
    for s in servers:
        if len(s["region"]) > 12:
            sys.exit(f"{s['region']}: more than 12 characters, the client's region[]")
    q = lambda xs: ", ".join(f'"{x}"' for x in xs)
    return "\n".join([
        "/* GENERATED from server/servers.fly.toml and fly.toml by tools/gen_regions.py -- do not edit. */",
        "#ifndef REGIONS_TAB_H", "#define REGIONS_TAB_H", "",
        f"#define REGION_URLS {{ {q(f'N:TCP://{host}:{s['port']}/' for s in servers)} }}",
        f'#define REGION_SEATS "N:HTTP://{host}:{port}/SEATS"',
        f"#define NREGIONS {len(servers)}",
        f"#define REGION_NAMES {{ {q(s['region'] for s in servers)} }}",
        f"#define REGION_PACES {{ {q(s.get('description', '') for s in servers)} }}",
        "", "#endif", ""])


if __name__ == "__main__":
    if sys.argv[1:] == ["h"]:
        sys.stdout.write(gen_h())
    elif sys.argv[1:2] == ["c"]:
        sys.stdout.write(gen_c(sys.argv[2:]))
    else:
        sys.exit(__doc__)
