from scapy.all import rdpcap, TCP, IP
MAGIC=b"\x30\x10"
def host_frames(path, port=1000, from_console=False):
    buf=bytearray()
    for p in rdpcap(path):
        if IP not in p or TCP not in p: continue
        ok = (p[TCP].sport==port) if from_console else (p[TCP].dport==port)
        if not ok: continue
        buf+=bytes(p[TCP].payload)
    i=0; out=[]
    while i+16<=len(buf):
        if buf[i:i+2]!=MAGIC: i+=1; continue
        ln=int.from_bytes(buf[i+6:i+8],'big')
        if ln<16 or i+ln>len(buf): i+=1; continue
        out.append((buf[i+8:i+10].decode('latin1'), int.from_bytes(buf[i+10:i+14],'big'), int.from_bytes(buf[i+14:i+16],'big'), bytes(buf[i+16:i+ln])))
        i+=ln
    return out
