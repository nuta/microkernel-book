"""

HinaOSの自動テスト (pytest)

使い方:

    $ make test
    $ make test RELEASE=1
    $ make test CPUS=<プロセッサ数>

テストの書き方:

    - test_ で始まる関数を定義する。
    - run_hinaos関数を呼ぶとHinaOSを自動でビルド・起動し、指定されたスクリプトを実行する。
      成功するとログが入ったResultオブジェクトが返る。

"""
import http
import http.server
import threading

def test_hello_world(run_hinaos):
    r = run_hinaos("echo howdy")
    assert "howdy" in r.log

def test_start_hello(run_hinaos):
    r = run_hinaos("start hello; echo howdy")
    assert "Hello World!" in r.log
    assert "howdy" in r.log

def test_read_file(run_hinaos):
    r = run_hinaos("cat hello.txt")
    assert "Hello World from HinaFS" in r.log

def test_write_file(run_hinaos):
    r = run_hinaos("write lfg.txt LFG; cat lfg.txt; ls")
    assert '[FILE] "lfg.txt"' in r.log
    assert '[shell] LFG' in r.log

def test_ls(run_hinaos):
    r = run_hinaos("ls")
    assert "hello.txt" in r.log

def test_mkdir(run_hinaos):
    r = run_hinaos("mkdir new_dir; ls")
    assert '[DIR ] "new_dir"' in r.log

def test_hinavm(run_hinaos):
    r = run_hinaos("start hello_hinavm")
    assert "hinavm_server: pc=7: 123" in r.log
    assert "reply value: 42" in r.log

def test_crack(run_hinaos):
    # crackに成功するまでタイムアウトを伸ばしていく
    for i in range(1, 5):
        r = run_hinaos("start crack", timeout=2**i, ignore_timeout=True)
        if "exploited!" in r.log:
            break
    else:
        assert False, "all attempts failed"

def test_http(run_hinaos):
    class TeapotServer(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            self.send_response(418)
            self.end_headers()
            self.wfile.write("🫖".encode("utf-8"))
    httpd = http.server.HTTPServer(("", 1234), TeapotServer)
    httpd_thread = threading.Thread(target=lambda: httpd.serve_forever(), daemon=True)
    httpd_thread.start()

    r = run_hinaos("http http://10.0.2.100:1234/teapot",
        qemu_net0_options=["guestfwd=tcp:10.0.2.100:1234-tcp:127.0.0.1:1234"])
    assert "🫖" in r.log

    httpd.shutdown()
    httpd.server_close()
    httpd_thread.join()
