import json
import select
import socket
import subprocess
import sys
import time

import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from server_path import find_server     # noqa: E402

EXE = find_server()
PORT = 4411
fails = []


def check(name, cond, detail=''):
    print('  %-52s %s%s' % (name, 'PASS' if cond else 'FAIL', '' if cond else '  <- ' + str(detail)))
    if not cond:
        fails.append(name)


class Client:
    def __init__(self, port, label):
        self.label = label
        self.s = socket.create_connection(('127.0.0.1', port), timeout=5)
        self.s.setblocking(False)
        self.buf = b''
        self.n = 0
        self.events = []

    def _pump(self, timeout):
        end = time.time() + timeout
        while time.time() < end:
            r, _, _ = select.select([self.s], [], [], max(0.0, end - time.time()))
            if not r:
                break
            chunk = self.s.recv(65536)
            if not chunk:
                break
            self.buf += chunk
            while b'\n' in self.buf:
                line, self.buf = self.buf.split(b'\n', 1)
                if line.strip():
                    yield json.loads(line)

    def call(self, method, params=None, timeout=5.0):
        self.n += 1
        req = {'id': self.n, 'method': method}
        if params:
            req['params'] = params
        self.s.sendall((json.dumps(req) + '\n').encode())
        for msg in self._pump(timeout):
            if msg.get('id') == self.n:
                return msg
            self.events.append(msg)
        raise RuntimeError('%s: no reply to %s' % (self.label, method))

    def drain(self, seconds=1.0):
        for msg in self._pump(seconds):
            self.events.append(msg)

    def close(self):
        self.s.close()


proc = subprocess.Popen([EXE, '--port', str(PORT), '--quiet'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
time.sleep(1.2)

try:
    a = Client(PORT, 'gui')
    b = Client(PORT, 'debugger')

    st = a.call('server.status')['result']
    check('server sees both clients', st['clients'] == 2, st['clients'])

    a.call('target.open', {'host': '192.0.2.11', 'type': 'PS3_DBG_DEX', 'target': 'dex'})
    stb = b.call('target.status', {'target': 'dex'})['result']
    check('B sees the target A opened', stb['host'] == '192.0.2.11' and stb['port'] == 1000, stb)

    b.call('target.open', {'host': '192.0.2.10', 'type': 'PS3_DEH_TCP', 'target': 'decr'})
    lst = a.call('target.list')['result']['targets']
    check('A sees both consoles', len(lst) == 2, lst)

    a.drain(0.5)
    b.drain(0.5)
    a.events.clear()
    b.events.clear()

    a.call('target.connect', {'target': 'dex'})
    time.sleep(1.5)
    a.drain(0.8)
    b.drain(0.8)

    a_ev = {e.get('event') for e in a.events}
    b_ev = {e.get('event') for e in b.events}
    check('A received events', len(a_ev) > 0, a_ev)
    check('B received events it did not request', len(b_ev) > 0, b_ev)
    check('both saw the same event kinds', a_ev == b_ev, 'A=%s B=%s' % (sorted(a_ev), sorted(b_ev)))
    check('events are tagged with their target', all('target' in e for e in a.events if 'event' in e), [e for e in a.events if 'event' in e and 'target' not in e])
    tagged = {e['target'] for e in a.events if e.get('event')}
    check('only the connecting target emitted', tagged == {'dex'}, tagged)

    a.close()
    time.sleep(0.6)
    st = b.call('server.status')['result']
    check('B survives A disconnecting', st['clients'] == 1, st['clients'])
    check('both sessions outlive A', st['targets'] == 2, st)
    stb = b.call('target.status', {'target': 'dex'})['result']
    check('session config persists after A leaves', stb['host'] == '192.0.2.11' and stb['port'] == 1000, stb)

    r = b.call('server.methods')
    check('B still fully functional', r['ok'] and len(r['result']['methods']) >= 25)

    b.close()
finally:
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()

print('\n%d checks failed' % len(fails))
sys.exit(1 if fails else 0)
