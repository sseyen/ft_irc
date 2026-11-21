"""ft_irc stress/regression test.

Usage: python3 -I tests/irc_test.py ./ircserv <port> <stderr-log>
Starts the server itself with password "pw", runs all checks, then sends SIGINT.
Prints PASS/FAIL per check. Run against an ASan build to catch memory errors."""
import socket, time, struct, sys, subprocess, os, signal

BIN = sys.argv[1]
PORT = int(sys.argv[2])
PW = "pw"
LOG = open(sys.argv[3], "w")
srv = subprocess.Popen([BIN, str(PORT), PW], stdout=subprocess.DEVNULL, stderr=LOG)
time.sleep(0.6)
results = []

def alive():
    return srv.poll() is None

def conn():
    s = socket.create_connection(("127.0.0.1", PORT))
    s.settimeout(0.4)
    return s

def rd(s, t=0.4):
    s.settimeout(t)
    out = b""
    try:
        while True:
            d = s.recv(65536)
            if not d:
                break
            out += d
    except (socket.timeout, ConnectionResetError, BlockingIOError):
        pass
    return out.decode(errors="replace")

def reg(nick, eol="\r\n"):
    s = conn()
    s.sendall(("PASS %s%sNICK %s%sUSER %s 0 * :real%s" % (PW, eol, nick, eol, nick, eol)).encode())
    r = rd(s)
    return s, r

def rst(s):
    s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
    s.close()

def check(name, cond, info=""):
    ok = bool(cond) and alive()
    results.append((ok, name, "" if ok else info))

# --- basics
a, r = reg("alice")
check("register with CRLF -> 001", " 001 alice" in r, r[:200])
s = conn(); s.sendall(b"PASS pw\nNICK bare\nUSER bare 0 * :x\n"); r = rd(s)
check("register with bare LF (nc without -C)", " 001 bare" in r, repr(r[:200]))
s.close()

# partial command
p = conn()
for chunk in [b"PA", b"SS p", b"w\r\nNI", b"CK par", b"t\r\nUSER part 0 * :x", b"\r\n"]:
    p.sendall(chunk); time.sleep(0.15)
r = rd(p)
check("partial command chunks register", " 001 part" in r, repr(r[:200]))
a.sendall(b"PING x\r\n"); r = rd(a)
check("other client fine during partial", "PONG" in r, r)

# A\r\nB in one chunk where B is incomplete
p.sendall(b"PING one\r\nPING tw"); r = rd(p)
check("complete line processed before trailing partial", "one" in r, repr(r))
p.sendall(b"o\r\n"); r = rd(p)
check("trailing partial completed later", "two" in r, repr(r))

# channels / broadcast
b, _ = reg("bob")
a.sendall(b"JOIN #c\r\n"); rd(a)
b.sendall(b"JOIN #c\r\n"); rd(b); rd(a)
a.sendall(b"PRIVMSG #c :hello all\r\n")
check("channel msg reaches others", "hello all" in rd(b))
check("sender doesn't get echo", "hello all" not in rd(a))
b.sendall(b"PRIVMSG alice,ALICE,alice :dup\r\n"); r = rd(a)
check("dedupe PRIVMSG targets", r.count("dup") == 1, r)
b.sendall(b"PRIVMSG nobody :x\r\nPRIVMSG #nochan :x\r\nPRIVMSG\r\nPRIVMSG alice\r\nPRIVMSG alice :\r\n"); r = rd(b)
check("PRIVMSG errors 401/401/411/412", r.count(" 401 ") == 2 and " 411 " in r and " 412 " in r, r)

# operator vs regular
b.sendall(b"MODE #c +i\r\nKICK #c alice\r\nTOPIC #c :x\r\n"); r = rd(b)
check("non-op MODE/KICK -> 482", r.count(" 482 ") >= 2, r)
a.sendall(b"MODE #c +t\r\n"); rd(a); rd(b)
b.sendall(b"TOPIC #c :x\r\n"); r = rd(b)
check("non-op TOPIC on +t -> 482", " 482 " in r, r)
a.sendall(b"MODE #c +o bob\r\n"); r = rd(b)
check("+o broadcast", "MODE #c +o bob" in r, r)
b.sendall(b"MODE #c -o alice\r\n"); rd(b); rd(a)
a.sendall(b"KICK #c bob\r\n"); r = rd(a)
check("de-opped alice can't KICK", " 482 " in r, r)
b.sendall(b"MODE #c +o alice\r\n"); rd(a); rd(b)
a.sendall(b"MODE #c +k secret\r\n"); rd(a); rd(b)
c, _ = reg("carol")
c.sendall(b"JOIN #c\r\n"); r = rd(c)
check("+k without key -> 475", " 475 " in r, r)
c.sendall(b"JOIN #c secret\r\n"); r = rd(c)
check("+k with key joins", "JOIN #c" in r, r)
a.sendall(b"KICK #c carol :bye\r\n"); r = rd(c)
check("KICK reaches target", "KICK #c carol" in r, r)
a.sendall(b"MODE #c -k\r\nMODE #c +i\r\n"); rd(a); rd(b)
c.sendall(b"JOIN #c\r\n"); r = rd(c)
check("+i -> 473", " 473 " in r, r)
a.sendall(b"INVITE carol #c\r\n"); r = rd(c)
check("INVITE delivered", "INVITE carol #c" in r, r)
c.sendall(b"JOIN #c\r\n"); r = rd(c)
check("invited joins +i", "JOIN #c" in r, r)
a.sendall(b"MODE #c -i\r\nMODE #c +l 3\r\n"); rd(a); rd(b); rd(c)
d, _ = reg("dave")
d.sendall(b"JOIN #c\r\n"); r = rd(d)
check("+l full -> 471", " 471 " in r, r)

# garbage input
g, _ = reg("gary")
junk = [b"\r\n", b"   \r\n", b":\r\n", b": \r\n", b":prefix\r\n", b"MODE\r\n", b"MODE #\r\n",
        b"JOIN ,,,,\r\n", b"JOIN #a,#b,#c,#d,#e,#f,#g,#h,#i,#j,#k,#l\r\n", b"KICK\r\n", b"KICK #c\r\n",
        b"INVITE\r\n", b"TOPIC\r\n", b"MODE #c +kkkkk\r\n", b"MODE #c +l -1\r\n",
        b"MODE #c +l 99999999999999999999\r\n", b"MODE gary +xyz\r\n", b"NICK\r\n", b"NICK #bad\r\n",
        b"USER again 0 * :x\r\n", b"PASS again\r\n", b"\xff\xfe\xfd\r\n", b"PRIVMSG #c :" + b"A" * 600 + b"\r\n",
        b"JOIN 0\r\n", b"JOIN #" + b"x" * 300 + b"\r\n", b"a " * 40 + b"\r\n", b"CAP LS\r\nCAP END\r\n",
        b"QUIT\r\n"]
for j in junk:
    try:
        g.sendall(j)
    except OSError:
        pass
    time.sleep(0.02)
rd(g)
check("server survives junk commands", alive())

# NUL byte
n = conn(); n.sendall(b"PASS pw\r\nNICK n\x00ul\r\n"); rd(n)
check("NUL byte doesn't crash", alive())

# overlong line without newline
o = conn(); o.sendall(b"A" * 5000); rd(o)
check("5000 bytes w/o newline doesn't crash", alive())

# unexpected kills
k, _ = reg("killme"); k.sendall(b"JOIN #c\r\n"); rd(k); rst(k); time.sleep(0.2)
a.sendall(b"PRIVMSG #c :after kill\r\n"); time.sleep(0.2)
check("RST client, others ok", "after kill" in rd(b))
h = conn(); h.sendall(b"PASS pw\r\nNICK ha"); rst(h); time.sleep(0.2)
nw, r = reg("newbie")
check("half command + kill, new client ok", " 001 newbie" in r, r)

# Ctrl-Z simulation: client stops reading, flood the channel
a.sendall(b"MODE #c -l\r\n"); rd(a)
z, _ = reg("zed"); z.sendall(b"JOIN #c\r\n"); time.sleep(0.2); rd(z)
z.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
N = 20000
t0 = time.time()
msg = b"".join(b"PRIVMSG #c :flood %d\r\n" % i for i in range(N))
a.sendall(msg)
a.sendall(b"PING still\r\n")
r = ""
dl = time.time() + 15
while "still" not in r and time.time() < dl:
    r += rd(a, 0.5)
check("server responsive while a client is stopped", "still" in r, "took >15s")
time.sleep(1)
got = ""
z.settimeout(2)
dl = time.time() + 20
while time.time() < dl and got.count("flood ") < N:
    try:
        d = z.recv(1 << 20)
        if not d: break
        got += d.decode(errors="replace")
    except socket.timeout:
        break
check("stopped client receives all flood msgs", got.count("flood ") == N, "got %d of %d" % (got.count("flood "), N))
for x in (b, c, d, g, nw, p):
    try: rd(x, 0.1)
    except Exception: pass

# many connections
many = []
for i in range(120):
    try:
        many.append(conn())
    except OSError:
        break
time.sleep(0.3)
for m in many: m.close()
time.sleep(0.3)
x, r = reg("after100")
check("120 connections, then new client works", " 001 after100" in r, r)

# nick collisions / case
y, r = reg("ALICE")
check("NICK ALICE collides with alice -> 433", " 433 " in r, r)
y.close()

# QUIT and slot reuse
q, _ = reg("quitter"); q.sendall(b"JOIN #c\r\nQUIT :bye\r\n"); r = rd(q)
check("QUIT gets ERROR and closes", "ERROR" in r, r)
q2, r = reg("quitter")
check("nick free after QUIT", " 001 quitter" in r, r)

# wrong password
w = conn(); w.sendall(b"PASS nope\r\nNICK w\r\nUSER w 0 * :w\r\n"); r = rd(w)
check("wrong password -> 464 + close", " 464 " in r, r)
w2 = conn(); w2.sendall(b"NICK w2\r\nUSER w2 0 * :w\r\n"); r = rd(w2)
check("no PASS -> not registered", " 001 " not in r, r)
w2.sendall(b"JOIN #c\r\n"); r = rd(w2)
check("command before registration -> 451", " 451 " in r, r)

# shutdown with active connections
srv.send_signal(signal.SIGINT)
try:
    code = srv.wait(5)
except subprocess.TimeoutExpired:
    code = "hang"
results.append((code == 0, "SIGINT with active clients exits 0", "exit=%s" % code))

for ok, name, info in results:
    print(("PASS " if ok else "FAIL ") + name + ("" if ok else "   -> " + info[:300]))
