#!/usr/bin/env python3
"""
Minimal signaling server for av_module.

Protocol: JSON-per-line over TCP (UTF-8 + '\n').
Supports: register, list_clients, call, accept, reject, hangup.
"""

import argparse
import json
import selectors
import socket
import threading
from dataclasses import dataclass, field
from typing import Dict, Optional, Tuple


@dataclass
class ClientConn:
    sock: socket.socket
    addr: Tuple[str, int]
    inbuf: bytearray = field(default_factory=bytearray)
    client_id: Optional[str] = None


class SignalingServer:
    def __init__(self, host: str, port: int) -> None:
        self.host = host
        self.port = port
        self.sel = selectors.DefaultSelector()
        self.server_sock: Optional[socket.socket] = None

        self._lock = threading.Lock()
        self._by_sock: Dict[socket.socket, ClientConn] = {}
        self._by_id: Dict[str, ClientConn] = {}
        self._active_peer: Dict[str, str] = {}

    def start(self) -> None:
        self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_sock.bind((self.host, self.port))
        self.server_sock.listen(128)
        self.server_sock.setblocking(False)
        self.sel.register(self.server_sock, selectors.EVENT_READ, self._accept)

        print(f"[signaling] listening on {self.host}:{self.port}")
        try:
            while True:
                for key, _ in self.sel.select(timeout=1.0):
                    callback = key.data
                    callback(key.fileobj)
        except KeyboardInterrupt:
            print("[signaling] stopped by Ctrl+C")
        finally:
            self._shutdown()

    def _shutdown(self) -> None:
        with self._lock:
            conns = list(self._by_sock.values())
        for conn in conns:
            self._close_conn(conn)

        if self.server_sock is not None:
            try:
                self.sel.unregister(self.server_sock)
            except Exception:
                pass
            try:
                self.server_sock.close()
            except Exception:
                pass
        self.sel.close()

    def _accept(self, srv_sock: socket.socket) -> None:
        sock, addr = srv_sock.accept()
        sock.setblocking(False)
        conn = ClientConn(sock=sock, addr=addr)
        with self._lock:
            self._by_sock[sock] = conn
        self.sel.register(sock, selectors.EVENT_READ, self._read)
        print(f"[signaling] connected: {addr[0]}:{addr[1]}")

    def _read(self, sock: socket.socket) -> None:
        with self._lock:
            conn = self._by_sock.get(sock)
        if conn is None:
            return

        try:
            data = sock.recv(4096)
        except OSError:
            data = b""

        if not data:
            self._close_conn(conn)
            return

        conn.inbuf.extend(data)
        while True:
            nl = conn.inbuf.find(b"\n")
            if nl < 0:
                break
            line = bytes(conn.inbuf[:nl]).strip()
            del conn.inbuf[: nl + 1]
            if not line:
                continue
            self._handle_line(conn, line)

    def _handle_line(self, conn: ClientConn, line: bytes) -> None:
        try:
            obj = json.loads(line.decode("utf-8"))
        except Exception:
            self._send(conn, {"type": "error", "message": "invalid json"})
            return

        if not isinstance(obj, dict):
            self._send(conn, {"type": "error", "message": "json must be object"})
            return

        typ = str(obj.get("type", ""))

        if typ == "register":
            self._on_register(conn, obj)
            return
        if typ == "list_clients":
            self._on_list_clients(conn)
            return
        if typ == "call":
            self._on_call(conn, obj)
            return
        if typ == "accept":
            self._on_accept(conn, obj)
            return
        if typ == "reject":
            self._on_reject(conn, obj)
            return
        if typ == "hangup":
            self._on_hangup(conn)
            return

        self._send(conn, {"type": "error", "message": f"unknown type: {typ}"})

    def _on_register(self, conn: ClientConn, obj: dict) -> None:
        client_id = str(obj.get("client_id", "")).strip()
        if not client_id:
            self._send(conn, {"type": "error", "message": "client_id required"})
            return

        with self._lock:
            old = self._by_id.get(client_id)
            if old is not None and old is not conn:
                self._send(old, {
                    "type": "error",
                    "message": "same client_id logged in elsewhere",
                })
                self._close_conn(old)

            conn.client_id = client_id
            self._by_id[client_id] = conn

        self._send(conn, {"type": "ack", "op": "register", "ok": True})
        print(f"[signaling] register: {client_id} <- {conn.addr[0]}:{conn.addr[1]}")

    def _on_list_clients(self, conn: ClientConn) -> None:
        if not conn.client_id:
            self._send(conn, {"type": "error", "message": "not registered"})
            return

        with self._lock:
            ids = sorted(self._by_id.keys())
        self._send(conn, {"type": "list_clients", "clients": ids})

    def _on_call(self, conn: ClientConn, obj: dict) -> None:
        if not conn.client_id:
            self._send(conn, {"type": "error", "message": "not registered"})
            return

        to_id = str(obj.get("to", "")).strip()
        rtp_port = int(obj.get("rtp_port", 0))
        if not to_id or rtp_port <= 0:
            self._send(conn, {"type": "error", "message": "to/rtp_port invalid"})
            return

        with self._lock:
            peer = self._by_id.get(to_id)

        if peer is None:
            self._send(conn, {"type": "reject", "from": to_id, "reason": "offline"})
            return

        from_id = conn.client_id
        assert from_id is not None

        self._send(peer, {
            "type": "call",
            "from": from_id,
            "peer_ip": conn.addr[0],
            "rtp_port": rtp_port,
        })

    def _on_accept(self, conn: ClientConn, obj: dict) -> None:
        if not conn.client_id:
            self._send(conn, {"type": "error", "message": "not registered"})
            return

        to_id = str(obj.get("to", "")).strip()
        rtp_port = int(obj.get("rtp_port", 0))
        if not to_id or rtp_port <= 0:
            self._send(conn, {"type": "error", "message": "to/rtp_port invalid"})
            return

        with self._lock:
            caller = self._by_id.get(to_id)

        if caller is None:
            self._send(conn, {"type": "error", "message": "caller offline"})
            return

        from_id = conn.client_id
        assert from_id is not None

        # Prefer observed source IP instead of client-reported peer_ip.
        self._send(caller, {
            "type": "accept",
            "from": from_id,
            "peer_ip": conn.addr[0],
            "rtp_port": rtp_port,
        })

        with self._lock:
            self._active_peer[from_id] = to_id
            self._active_peer[to_id] = from_id

    def _on_reject(self, conn: ClientConn, obj: dict) -> None:
        if not conn.client_id:
            self._send(conn, {"type": "error", "message": "not registered"})
            return

        to_id = str(obj.get("to", "")).strip()
        reason = str(obj.get("reason", "rejected"))
        if not to_id:
            self._send(conn, {"type": "error", "message": "to invalid"})
            return

        with self._lock:
            caller = self._by_id.get(to_id)

        if caller is not None:
            self._send(caller, {
                "type": "reject",
                "from": conn.client_id,
                "reason": reason,
            })

    def _on_hangup(self, conn: ClientConn) -> None:
        if not conn.client_id:
            self._send(conn, {"type": "error", "message": "not registered"})
            return

        me = conn.client_id
        with self._lock:
            peer_id = self._active_peer.pop(me, None)
            if peer_id:
                self._active_peer.pop(peer_id, None)
                peer = self._by_id.get(peer_id)
            else:
                peer = None

        if peer is not None:
            self._send(peer, {"type": "hangup", "from": me})

    def _close_conn(self, conn: ClientConn) -> None:
        with self._lock:
            if conn.sock in self._by_sock:
                del self._by_sock[conn.sock]

            if conn.client_id:
                current = self._by_id.get(conn.client_id)
                if current is conn:
                    del self._by_id[conn.client_id]

                peer_id = self._active_peer.pop(conn.client_id, None)
                if peer_id:
                    self._active_peer.pop(peer_id, None)
                    peer = self._by_id.get(peer_id)
                    if peer is not None:
                        self._send(peer, {"type": "hangup", "from": conn.client_id})

        try:
            self.sel.unregister(conn.sock)
        except Exception:
            pass
        try:
            conn.sock.close()
        except Exception:
            pass

        cid = conn.client_id or "<unregistered>"
        print(f"[signaling] disconnected: {cid} {conn.addr[0]}:{conn.addr[1]}")

    @staticmethod
    def _send(conn: ClientConn, obj: dict) -> None:
        try:
            payload = (json.dumps(obj, ensure_ascii=False, separators=(",", ":")) + "\n").encode("utf-8")
            conn.sock.sendall(payload)
        except OSError:
            pass


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="av_module signaling server")
    parser.add_argument("--host", default="0.0.0.0", help="listen host")
    parser.add_argument("--port", type=int, default=9000, help="listen port")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    srv = SignalingServer(args.host, args.port)
    srv.start()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
