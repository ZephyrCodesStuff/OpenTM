import json
import os
import socket
import subprocess
import sys
import time


sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from server_path import find_server   # noqa: E402

EXE = find_server()
fails = []


def check(name, cond, detail=''):
    print('  %-46s %s%s' % (name, 'PASS' if cond else 'FAIL', '' if cond else '  <- ' + str(detail)))
    if not cond:
        fails.append(name)


class Server:
    def __init__(self, port, allow=False):
        args = [EXE, '--port', str(port), '--quiet']
        if allow:
            args.append('--allow-mutating')
        self.p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        time.sleep(1.2)
        self.s = socket.create_connection(('127.0.0.1', port), timeout=5)
        self.f = self.s.makefile('rwb')
        self.n = 0

    def call(self, method, params=None):
        self.n += 1
        req = {'id': self.n, 'method': method}
        if params:
            req['params'] = params
        self.f.write((json.dumps(req) + '\n').encode())
        self.f.flush()
        while True:
            line = self.f.readline()
            if not line:
                raise RuntimeError('server closed')
            msg = json.loads(line)
            if msg.get('id') == self.n:
                return msg

    def close(self):
        try:
            self.s.close()
        finally:
            self.p.terminate()
            try:
                self.p.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.p.kill()


print('=== read-only server ===')
sv = Server(4401, allow=False)
try:
    r = sv.call('server.methods')
    names = {m['method'] for m in r['result']['methods']}
    check('server.methods lists >= 25 methods', len(names) >= 25, len(names))
    for expect in ('target.open', 'target.close', 'target.list', 'target.connect', 'fs.list', 'process.threads', 'process.load', 'settings.apply', 'tty.read'):
        check('  advertises %s' % expect, expect in names)

    mut = {m['method'] for m in r['result']['methods'] if m['mutating']}
    check('load/install/reset marked mutating', {'process.load', 'package.install', 'target.reset'} <= mut, sorted(mut))
    check('reads not marked mutating', not ({'fs.list', 'process.list', 'tty.read'} & mut))

    st = sv.call('server.status')['result']
    check('status: no targets open', st['targets'] == 0, st)
    check('status: read-only', st['allow_mutating'] is False)

    r = sv.call('target.connect')
    check('no targets: resolution fails', not r['ok'], r)
    check('  and says to open one', 'target.open' in r.get('error', ''), r)

    r = sv.call('target.open', {'host': '192.0.2.11', 'type': 'PS3_DBG_DEX', 'target': 'dex'})
    check('target.open accepts DEX', r['ok'], r)
    check('  DEX derives port 1000', r['result'].get('port') == 1000, r)

    r = sv.call('target.open', {'host': '192.0.2.11', 'type': 'PS3_DBG_DEX', 'target': 'dex'})
    check('re-open of same endpoint is idempotent', r['ok'], r)

    r = sv.call('target.open', {'host': '192.0.2.99', 'type': 'PS3_DBG_DEX', 'target': 'dex'})
    check('name collision on other endpoint rejected', not r['ok'], r)

    r = sv.call('target.status')
    check('single target resolves without a name', r['ok'], r)
    check('  and reports its host', r['result']['host'] == '192.0.2.11', r)
    check('  and reports a socket state', 'state' in r['result'], r)

    r = sv.call('target.connect')
    check('connect reply carries state', 'state' in r['result'], r)
    check('  and says whether it was already up', r['result'].get('already_connected') is False, r)
    sv.call('target.disconnect')

    r = sv.call('target.open', {'host': '192.0.2.10', 'type': 'PS3_DEH_TCP'})
    check('second target opens', r['ok'], r)
    check('  unnamed defaults to host:port',r['result']['target'] == '192.0.2.10:8530', r)
    check('  DECR derives port 8530', r['result']['port'] == 8530, r)

    lst = sv.call('target.list')['result']['targets']
    check('target.list shows both', len(lst) == 2, lst)

    r = sv.call('target.status')
    check('ambiguous target rejected', not r['ok'], r)
    check('  and asks for a name', 'target' in r.get('error', ''), r)

    r = sv.call('target.status', {'target': 'nope'})
    check('unknown target rejected', not r['ok'], r)

    r = sv.call('target.open', {'type': 'PS3_DEH_TCP'})
    check('open without host rejected', not r['ok'], r)

    r = sv.call('target.open', {'host': '192.0.2.12', 'type': 'NOT_A_TYPE'})
    check('bad type rejected', not r['ok'], r)

    r = sv.call('process.load', {'target': 'dex', 'path': 'x.self'})
    check('mutating refused when read-only', not r['ok'], r)
    check('  refusal names the flag', '--allow-mutating' in r.get('error', ''))

    r = sv.call('fs.list', {'target': 'dex', 'path': '/'})
    check('session-needing refused with no session', not r['ok'], r)
    check('  refusal says connect first', 'connect' in r.get('error', '').lower())

    r = sv.call('tty.read', {'target': 'dex'})
    check('tty.read works without a session', r['ok'] and r['result']['cursor'] == 0)

    r = sv.call('target.close', {'target': 'dex'})
    check('target.close works', r['ok'], r)
    check('  and drops it from the list', len(sv.call('target.list')['result']['targets']) == 1)

    r = sv.call('nope')
    check('unknown method rejected', not r['ok'])
finally:
    sv.close()

print('\n=== stable identity across renames ===')
sv = Server(4403)
try:
    tid = '6f1c7b2e-0000-4000-8000-abcdefabcdef'
    r = sv.call('target.open', {'id': tid, 'target': 'CECHB002', 'host': '192.0.2.37', 'port': 1000, 'type': 'PS3_DBG_DEX'})
    check('open returns the id as the handle', r['ok'] and r['result']['target'] == tid, r)
    check('  and reports the display name', r['result'].get('name') == 'CECHB002', r)

    r = sv.call('target.open', {'id': tid, 'target': 'devkit-renamed', 'host': '192.0.2.37', 'port': 1000,'type': 'PS3_DBG_DEX'})
    check('re-opening the id renames rather than duplicating', r['ok'] and r['result']['target'] == tid, r)
    check('  name actually changed', r['result'].get('name') == 'devkit-renamed', r)

    lst = sv.call('target.list')['result']['targets']
    check('  still exactly one session', len(lst) == 1, lst)

    r = sv.call('target.status', {'target': tid})
    check('handle still resolves after the rename', r['ok'], r)

    r = sv.call('target.status', {'target': 'devkit-renamed'})
    check('new name resolves too (CLI convenience)', r['ok'], r)

    r = sv.call('target.status', {'target': 'CECHB002'})
    check('old name no longer resolves', not r['ok'], r)

    r = sv.call('target.open', {'record': {
        'id': tid, 'name': 'devkit-renamed', 'host': '192.0.2.37',
        'port': 1000, 'type': 'PS3_DBG_DEX',
        'mac': '00:11:22:33:44:55', 'reset_mode': 3,
        'reset_boot_value': '81', 'reset_boot_mask': '17',
    }})
    check('full record accepted on re-open', r['ok'], r)
    st = sv.call('target.status', {'target': tid})['result']
    check('  and still the same session', st['target'] == tid, st)
finally:
    sv.close()

print('\n=== server with --allow-mutating ===')
sv = Server(4402, allow=True)
try:
    st = sv.call('server.status')['result']
    check('status: mutating allowed', st['allow_mutating'] is True)
    sv.call('target.open', {'host': '192.0.2.11', 'target': 'kit'})

    r = sv.call('process.load', {'target': 'kit', 'path': 'x.self'})
    check('load passes permission gate', 'allow-mutating' not in r.get('error', ''), r)
    check('  but still needs a session', not r['ok'] and 'connect' in r.get('error', '').lower())

    r = sv.call('target.reset', {'target': 'kit', 'mode': 'banana'})
    check('bad reset mode rejected', not r['ok'], r)

    r = sv.call('package.install', {'target': 'kit', 'path': 'does-not-exist.pkg'})
    check('install rejects missing file', not r['ok'], r)

    r = sv.call('process.threads', {'target': 'kit'})
    check('missing pid rejected', not r['ok'], r)
finally:
    sv.close()

print('\n%d checks failed' % len(fails))
sys.exit(1 if fails else 0)
