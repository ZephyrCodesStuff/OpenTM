import os


def find_server():
    if os.environ.get('OPENTM_SERVER'):
        return os.environ['OPENTM_SERVER']
    root = os.path.dirname(os.path.dirname(os.path.dirname(
        os.path.abspath(__file__))))
    candidates = [
        'build/vs2022/bin/Release/opentm_server.exe',
        'build/vs2022/bin/Debug/opentm_server.exe',
        'build/msvc-release/bin/opentm_server.exe',
        'build/msvc-debug/bin/opentm_server.exe',
        'build/linux-release/bin/opentm_server',
        'build/linux-debug/bin/opentm_server',
        'build/bin/opentm_server',
    ]
    for rel in candidates:
        path = os.path.join(root, *rel.split('/'))
        if os.path.exists(path):
            return path
    raise SystemExit('opentm_server not found; set OPENTM_SERVER')
