import os
import sys
import socket
import struct
import json
import re
import uuid

OP_HANDSHAKE = 0
OP_FRAME = 1
OP_CLOSE = 2
OP_PING = 3
OP_PONG = 4

class DiscordIPC:
    def __init__(self, client_id):
        self.isWindows = sys.platform == 'win32'
        self.client_id = client_id
        self.pid = os.getpid()
        self._connect()
        self.connected = True

    def _connect(self):
        if self.isWindows:
            for i in range(10):
                self.ipc_path = f'\\\\?\\pipe\\discord-ipc-{i}'
                try:
                    self.descriptor = open(self.ipc_path, 'w+b')
                    break
                except OSError:
                    pass
            else:
                raise Exception('Can\'t connect to discord client')

        else:
            self.descriptor = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            path = os.environ.get('XDG_RUNTIME_DIR') or os.environ.get('TMPDIR') \
                or os.environ.get('TMP') or os.environ.get('TEMP') or '/tmp'
            for i in range(10):
                self.ipc_path = re.sub(r'\/$', '', path) + f'/discord-ipc-{i}'
                try:
                    self.descriptor.connect(self.ipc_path)
                    break
                except:
                    pass
            else:
                raise Exception('Can\'t connect to discord client')

        self._send(OP_HANDSHAKE, {
            'v': 1,
            'client_id': self.client_id
        })

        data = self._recv()
        # todo

    def _encode(self, opcode, payload):
        payload = json.dumps(payload)
        payload = payload.encode('utf-8')
        return struct.pack('<ii', opcode, len(payload)) + payload

    def _send(self, opcode, payload):
        encoded_payload = self._encode(opcode, payload)
        try:
            if self.isWindows:
                self.descriptor.write(encoded_payload)
                self.descriptor.flush()
            else:
                self.descriptor.send(encoded_payload)
        except:
            raise Exception('Error sending data to discord IPC')

    def _recv(self):
        return ''


    def close(self):
        if self.connected:
            try:
                self._send(OP_CLOSE, {})
            except:
                pass
            finally:
                self.descriptor.close()
            self.connected = False

    def update_activity(self, activity):
        payload = {
            'cmd': 'SET_ACTIVITY',
            'args': {
                'pid': self.pid,
                'activity': activity
            },
            'nonce': str(uuid.uuid4())
        }

        self._send(OP_FRAME, payload)
        self._recv()

