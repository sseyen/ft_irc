# ft_irc test report

- **Commit tested:** `88b352d`
- **Platform:** macOS (Apple Silicon). A Linux run is still to do.
- **Tools:** AddressSanitizer + UBSan build, macOS `leaks`, `nc`, Python socket tests (`tests/irc_test.py`), the evaluation sheet.

The server never crashed and had no memory errors or leaks in any test. There are 6 issues below, ordered by risk.

## Issues

### 1. `errno` checked after `accept()` (risk: grade 0)

`src/Server.cpp:128`

```cpp
if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
    return false;
```

The evaluation sheet says that after accept/recv/send, errno must not be used to trigger specific action. Both branches return `false`, so behaviour does not depend on errno. A strict evaluator can still stop the defence when they see the check.

**Fix:** remove the errno check. Log the error and return `false`.

### 2. Lines ending in bare `\n` are never processed

`src/Server.cpp:197`: `recvMsg` only looks for `"\r\n"`, so a line ending in plain `\n` never reaches `handleBuffer`, even though `handleBuffer` accepts `\n`.

**Repro:** `nc localhost 6667` (no `-C`), send `PASS`/`NICK`/`USER`. The server sends no reply.

**Fix:** search for `'\n'` in `recvMsg`. The todo list marked this as item 4 (done), but the code still has the bug.

### 3. Empty port, port 0 and empty password are accepted

`src/main.cpp:6-23`

- `./ircserv '' pw`: `atoi("")` returns 0, so the server listens on a random port.
- `./ircserv 0 pw`: same result.
- `./ircserv 6667 ''`: the server starts with an empty password.

**Fix:** reject an empty port string, port 0 and an empty password. The todo list marked this as item 21 (done), but the code still has the bug.

### 4. A line of 1024+ bytes disconnects the client

`src/Server.cpp:185`: when the line is longer than `_sBuff[i]` (1024 bytes) and has no newline yet, the client is dropped. Lines of 511 to 1023 bytes correctly get 417.

**Repro:** send `PRIVMSG a :` + 1500 × `A` + `\r\n`. The client gets `Connection reset by peer`.

**Fix:** on overflow, discard input up to the next `\n` and send 417. Keep the client connected. Low priority: irssi splits long messages itself.

### 5. `memchr` runs before the `chk_r` check

`src/Server.cpp:176` runs before the `chk_r <= 0` check at `src/Server.cpp:183`. When `recv` returns -1, the length passed to `memchr` becomes `(size_t)-1`, which is undefined behaviour. It works today only because `_rBuff[0]` is zeroed.

**Fix:** move `if (chk_r <= 0) return dropClient(i);` above the `memchr` call.

### 6. SIGQUIT / SIGHUP end the server without the destructor

- Closing the terminal sends SIGHUP: exit 129, destructor not run.
- Ctrl-\ sends SIGQUIT: default action is termination with a core dump. Not reproduced in the test shell, which ignored SIGQUIT.

**Fix (optional):** register both in `setupSignals()` with `handleSignal`. Low priority.

## Passed

| Area | Result |
|---|---|
| Basic checks | one `poll()`; every `fcntl` is `fcntl(fd, F_SETFL, O_NONBLOCK)`; Makefile flags `-Wall -Wextra -Werror -std=c++98` |
| Partial commands | chunks `PA` / `SS p` / `w\r\nNI` assemble correctly; other clients stay responsive |
| Unexpected disconnects | client RST, kill mid-command: server stays up, new clients register |
| Ctrl-Z client + flood | `nc` stopped with SIGSTOP, 30000 lines flooded: server answers other clients; after SIGCONT all 30000 lines delivered |
| Memory | `leaks`: 0 during flood and 0 at exit after SIGINT with clients still connected; exit code 0 |
| ASan / UBSan | full test suite, no reports |
| PRIVMSG | channel and user targets, duplicate targets, 401 / 411 / 412 |
| Operator commands | regular user gets 482; `+i` `+t` `+k` `+o` `+l`, INVITE and KICK work |
| Junk input | NUL, `0xFF`, empty lines, lone `:`, bad MODE / JOIN / KICK params, 12 channels at once |
| Connection limit | 120 connections against a limit of 99: extras closed, a new client then registers |
| Server signals | SIGINT / SIGTERM: clean exit 0; Ctrl-Z + fg: server keeps working |

**Not tested:** a real IRC client (irssi / weechat were not installed). Test the team's reference client before the defence.

## How to rerun

```bash
make
python3 -I tests/irc_test.py ./ircserv 7001 /tmp/irc_stderr.log
```

Build with sanitizers to catch memory errors:

```bash
c++ -Wall -Wextra -Werror -std=c++98 -Iinclude -g -fsanitize=address,undefined src/*.cpp -o ircserv_asan
python3 -I tests/irc_test.py ./ircserv_asan 7002 /tmp/asan.log
grep -E "ERROR|runtime error" /tmp/asan.log
```

On Linux, check leaks with valgrind instead of `leaks`. Stop the server with Ctrl-C while clients are still connected:

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes ./ircserv 6667 pw
```

The expected result with the current code: every check passes except `register with bare LF` (issue 2).
