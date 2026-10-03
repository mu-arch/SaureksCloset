"""Exercise the actual 32-bit DLL transport against the sibling Rust relay.

Requires Wine, OpenSSL, LLVM-MinGW (CLOSET_TOOLCHAIN) and a built debug relay.
Uses only synthetic identities, disposable SQLite and an isolated Wine prefix.
Ports 8787, 8788 and 19443 must be free. No third-party Python modules required.
"""
import asyncio
import hashlib
import os
from pathlib import Path
import sqlite3
import ssl
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
RELAY = ROOT.parent / "sharing-relay/target/debug/saureks-closet-relay"


async def exercise(base, exe, env, wine_env):
    clients = []
    server = subprocess.Popen([str(RELAY)], env=env, stdout=subprocess.DEVNULL)
    listener = None
    try:
        for _ in range(50):
            await asyncio.sleep(.1)
            if server.poll() is not None:
                raise RuntimeError("Disposable relay failed to start; check for occupied ports")
            try:
                _, writer = await asyncio.open_connection("127.0.0.1", 8787)
                writer.close()
                await writer.wait_closed()
                break
            except ConnectionRefusedError:
                continue
        else:
            raise RuntimeError("Disposable relay did not start")

        async def proxy(reader, writer):
            async def pipe(source, target):
                try:
                    while data := await source.read(16384):
                        target.write(data)
                        await target.drain()
                finally:
                    target.close()
            try:
                upstream, output = await asyncio.open_connection("127.0.0.1", 8787)
                await asyncio.gather(pipe(reader, output), pipe(upstream, writer), return_exceptions=True)
            except (OSError, asyncio.CancelledError):
                writer.close()

        tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        tls.load_cert_chain(base / "cert.pem", base / "key.pem")
        listener = await asyncio.start_server(proxy, "127.0.0.1", 19443, ssl=tls)
        trust = await asyncio.create_subprocess_exec(
            "wine", str(exe), "trust", str(base / "cert.der"), env=wine_env,
            stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.STDOUT)
        clients.append(trust)
        output, _ = await asyncio.wait_for(trust.communicate(), 90)
        if trust.returncode:
            raise RuntimeError("Test certificate setup failed: " + output.decode(errors="replace"))
        for guid in (10, 20):
            clients.append(await asyncio.create_subprocess_exec(
                "wine", str(exe), str(guid), env=wine_env,
                stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.STDOUT))
        for client in clients[1:]:
            output, _ = await asyncio.wait_for(client.communicate(), 50)
            print(output.decode(errors="replace"), end="")
            if client.returncode:
                raise RuntimeError(f"Windows transport fixture failed: {client.returncode}")
    finally:
        for client in clients:
            if client.returncode is None:
                client.kill()
                await client.wait()
        if listener:
            listener.close()
            await listener.wait_closed()
        server.terminate()
        server.wait(timeout=5)
        subprocess.run(["wineserver", "-k"], env=wine_env, check=False)


def main():
    compiler = Path(os.environ["CLOSET_TOOLCHAIN"]) / "bin/i686-w64-mingw32-clang++"
    if not RELAY.is_file():
        raise SystemExit("Build the sibling sharing-relay first: cargo build --locked")
    with tempfile.TemporaryDirectory(prefix="closet-sharing-wss-") as directory:
        base = Path(directory)
        exe = base / "transport.exe"
        subprocess.run([str(compiler), "-std=c++17", "-Os", "-static",
                        str(ROOT / "tests/sharing_transport_windows.cpp"),
                        "-lwinhttp", "-lcrypt32", "-o", str(exe)], check=True)
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
                        "-days", "1", "-subj", "/CN=localhost",
                        "-addext", "subjectAltName=DNS:localhost",
                        "-keyout", str(base / "key.pem"), "-out", str(base / "cert.pem")],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run(["openssl", "x509", "-in", str(base / "cert.pem"), "-outform", "DER",
                        "-out", str(base / "cert.der")], check=True)
        env = dict(os.environ, CLOSET_DATABASE=str(base / "test.sqlite"), CLOSET_ADMIN_TOKEN="f" * 64)
        subprocess.run([str(RELAY), "provision", "fixture", "server", "realm", "10"],
                       env=env, check=True, stdout=subprocess.DEVNULL)
        with sqlite3.connect(base / "test.sqlite") as db:
            db.execute("DELETE FROM credentials")
            for guid, letter in ((10, "a"), (20, "b")):
                db.execute("INSERT INTO credentials VALUES(?,?,?,?,?)", (
                    hashlib.sha256((letter * 64).encode()).hexdigest(),
                    "fixture" + str(guid), "server", "realm", str(guid)))
        prefix = base / "wine"
        prefix.mkdir()
        wine_env = dict(os.environ, WINEPREFIX=str(prefix), WINEDEBUG="-all",
                        WINEDLLOVERRIDES="winemenubuilder.exe=d")
        asyncio.run(exercise(base, exe, env, wine_env))


if __name__ == "__main__":
    main()
