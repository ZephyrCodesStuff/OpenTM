import sys, os, json, argparse, collections
from scapy.all import rdpcap, TCP, IP

MAGIC = b"\x30\x10"
DIR_FAMILY = {
    b"HM": ("HM", "host->machine",   "netmp"),
    b"MH": ("MH", "machine->host",   "netmp"),
    b"HT": ("HT", "host->target",    "dfmp"),
    b"TH": ("TH", "target->host",    "dfmp"),
    b"MT": ("MT", "manager->target", "netmp_cfw"),
    b"TM": ("TM", "target->manager", "netmp_cfw"),
}
GREETING = b"\x00" * 6
KNOWN_PORTS = (1000, 8530)

def reassemble(pcap):
    pkts = rdpcap(pcap)
    streams = {}
    for p in pkts:
        if IP not in p or TCP not in p: continue
        sip,dip = p[IP].src, p[IP].dst
        sp,dp = p[TCP].sport, p[TCP].dport
        key = tuple(sorted([(sip,sp),(dip,dp)]))
        streams.setdefault(key, []).append(p)
    out=[]
    for key, ps in streams.items():
        ps.sort(key=lambda x:(x.time, x[TCP].seq))
        a,b = key
        if a[1] in KNOWN_PORTS: server,client = a,b
        elif b[1] in KNOWN_PORTS: server,client = b,a
        else: server,client = (a,b) if a[1]<b[1] else (b,a)
        c2s=bytearray(); s2c=bytearray()
        for p in ps:
            pl=bytes(p[TCP].payload)
            if not pl: continue
            if p[IP].src==client[0] and p[TCP].sport==client[1]: c2s += pl
            else: s2c += pl
        out.append({"client":client,"server":server,"c2s":bytes(c2s),"s2c":bytes(s2c),"pkts":len(ps)})
    out.sort(key=lambda s:-(len(s["c2s"])+len(s["s2c"])))
    return out

def parse_frames(buf, skip=0):
    i=skip; n=len(buf)
    while i<n:
        if n-i<16:
            yield {"_error":"short","off":i}; return
        if buf[i:i+2]!=MAGIC:
            yield {"_error":"bad_magic","off":i,"bytes":buf[i:i+2].hex()}; return
        length = int.from_bytes(buf[i+6:i+8],"big")
        if length<16 or i+length>n:
            yield {"_error":"bad_len","off":i,"length":length}; return
        db = buf[i+8:i+10]
        sa = int.from_bytes(buf[i+2:i+6],"big")
        sb = int.from_bytes(buf[i+10:i+14],"big")
        cat = int.from_bytes(buf[i+14:i+16],"big")
        payload = bytes(buf[i+16:i+length])
        fam = DIR_FAMILY.get(db,(db.hex(),"unknown","unknown"))
        yield {"off":i,"len":length,"dir":fam[0],"family":fam[2],
               "sess_a":sa,"sess_b":sb,"category":cat,"payload":payload}
        i += length

def extract_strings(p, m=4):
    out=[]; run=bytearray()
    for b in p:
        if 32<=b<127: run.append(b)
        else:
            if len(run)>=m: out.append(run.decode("ascii","ignore"))
            run=bytearray()
    if len(run)>=m: out.append(run.decode("ascii","ignore"))
    return out

def decode(pcap):
    streams = reassemble(pcap)
    out = {"pcap":os.path.basename(pcap), "streams":[]}
    for i,s in enumerate(streams):
        e = {"stream_index":i,"client":f"{s['client'][0]}:{s['client'][1]}",
             "server":f"{s['server'][0]}:{s['server'][1]}","tcp_packets":s["pkts"],"directions":{}}
        for side, buf, label in [("c2s",s["c2s"],"C->S"),("s2c",s["s2c"],"S->C")]:
            skip=0; greeting=None
            if side=="s2c" and len(buf)>=6 and buf[:6]==GREETING:
                greeting=buf[:6].hex(); skip=6
            frames=[]; err=None
            for it in parse_frames(buf, skip):
                if "_error" in it: err=it; break
                cw = int.from_bytes(it["payload"][:2],"big") if len(it["payload"])>=2 else None
                frames.append({"off":it["off"],"len":it["len"],"dir":it["dir"],
                               "family":it["family"],
                               "sess_a":f"0x{it['sess_a']:08x}","sess_b":f"0x{it['sess_b']:08x}",
                               "category":f"0x{it['category']:04x}",
                               "cmd_word":f"0x{cw:04x}" if cw is not None else None,
                               "payload_hex":it["payload"].hex(),
                               "strings":extract_strings(it["payload"])})
            e["directions"][side] = {"label":label,"bytes":len(buf),"greeting":greeting,"frames":frames,"tail":err}
        out["streams"].append(e)
    return out

def print_decoded(d):
    print(f"\n==== {d['pcap']} ====")
    for s in d["streams"]:
        print(f"\n  -- stream {s['stream_index']}: {s['client']} <-> {s['server']} ({s['tcp_packets']} pkts) --")
        for side in ("c2s","s2c"):
            x = s["directions"][side]
            print(f"  [{x['label']}] {x['bytes']}B"+(f" greeting={x['greeting']}" if x['greeting'] else "")+f", {len(x['frames'])} frames")
            for f in x["frames"]:
                hx=f["payload_hex"][:80]; sh=" ".join(hx[i:i+2] for i in range(0,len(hx),2))
                more = f" +{(len(f['payload_hex'])-80)//2}B" if len(f["payload_hex"])>80 else ""
                strs = f"  strs={f['strings']}" if f["strings"] else ""
                print(f"    @0x{f['off']:04x} len={f['len']:4d} {f['dir']:2} cat={f['category']} cmd={f['cmd_word']} sa={f['sess_a']} sb={f['sess_b']}  {sh}{more}{strs}")
            if x["tail"]: print(f"    tail: {x['tail']}")

def short(p):
    return (p.replace(".pcapng","").replace("_from_tm","").replace("_in_tm","")
            .replace("_decr","").replace("_to_","").replace("_b00","-B00")
            .replace("traversing_directories_and_deleting_files","trav_del")
            .replace("attempt_to_rename","rename").replace("resetting","reset")
            .replace("it_system_software_mode","ssm").replace("debug_mode","dbg")
            .replace("upload_dummy_file","upload"))

def aggregate(d):
    t = collections.defaultdict(lambda:{"count":0,"pcaps":set(),"dirs":set()})
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".frames.json"): continue
        data = json.load(open(os.path.join(d,fn)))
        for s in data["streams"]:
            for side in ("c2s","s2c"):
                for f in s["directions"][side]["frames"]:
                    k=(f["family"],f["category"],f["cmd_word"] or "????")
                    r=t[k]; r["count"]+=1; r["pcaps"].add(data["pcap"]); r["dirs"].add(f["dir"])
    print(f"{'family':10} {'cat':8} {'cmd':8} {'dirs':5} {'count':>6}  pcaps")
    print("-"*110)
    for k in sorted(t.keys()):
        r=t[k]
        pcs=",".join(short(p) for p in sorted(r["pcaps"]))
        print(f"{k[0]:10} {k[1]:8} {k[2]:8} {','.join(sorted(r['dirs'])):5} {r['count']:>6}  {pcs}")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("paths", nargs="*")
    ap.add_argument("--json-sidecar", action="store_true")
    ap.add_argument("--aggregate", metavar="DIR")
    ap.add_argument("--quiet", action="store_true")
    args=ap.parse_args()
    if args.aggregate:
        aggregate(args.aggregate); return
    for p in args.paths:
        d = decode(p)
        if not args.quiet: print_decoded(d)
        if args.json_sidecar:
            out=os.path.splitext(p)[0]+".frames.json"
            for s in d["streams"]:
                for side in ("c2s","s2c"):
                    for f in s["directions"][side]["frames"]:
                        if len(f["payload_hex"])>512:
                            f["payload_hex_full_len"]=len(f["payload_hex"])//2
                            f["payload_hex"]=f["payload_hex"][:512]+"..."
            with open(out,"w") as fh: json.dump(d,fh,indent=2)
            print(f"wrote {out}")

if __name__=="__main__":
    main()
