import socket
import time
import re
from datetime import datetime

HOST = '127.0.0.1'  # ft_irc server address
PORT = 8080        # ft_irc server port
PASSWORD = "ll"   # expected correct server password

# =============================================================
# Utility Logging & I/O
# =============================================================

def log(msg, direction=None):
    """Timestamped console logger.
    direction: None | 'SEND' | 'RECV'"""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    if direction == "SEND":
        print(f"[{timestamp}] >>> {msg.strip()}")
    elif direction == "RECV":
        print(f"[{timestamp}] <<< {msg.strip()}")
    else:
        print(f"[{timestamp}] {msg}")


def send(sock, msg):
    log(msg, "SEND")
    sock.sendall(msg.encode('utf-8'))
    time.sleep(0.3)


def recv(sock, timeout=1):
    """Return a *single* recv read (may contain multiple IRC msgs)."""
    sock.settimeout(timeout)
    try:
        data = sock.recv(4096)
        if data:
            decoded = data.decode('utf-8', errors='replace')
            log(decoded, "RECV")
            return decoded
    except socket.timeout:
        return ''
    except Exception as e:
        log(f"recv() error: {e}")
    return ''


def drain(sock, timeout=0.3, pause=0.0):
    """Drain all currently queued data (non-blocking-ish)."""
    out = []
    while True:
        if pause:
            time.sleep(pause)
        chunk = recv(sock, timeout)
        if not chunk:
            break
        out.append(chunk)
    return ''.join(out)


def extract_error_info(msg):
    """Return (numeric, text) from a server line if present."""
    match = re.search(r'\b(\d{3})\b.*?: (.*)', msg)
    if match:
        return match.group(1), match.group(2)
    return None, None

# =============================================================
# Client connection helpers
# =============================================================

def new_socket():
    return socket.socket(socket.AF_INET, socket.SOCK_STREAM)


def connect_raw():
    s = new_socket()
    s.connect((HOST, PORT))
    return s


def register_client(nick,
                    user,
                    password=PASSWORD,
                    send_pass=True,
                    send_nick=True,
                    send_user=True,
                    wait=0.2,
                    drain_welcome=True):
    """Flexible registration helper.

    Set send_pass=False to omit PASS.
    Set password to wrong value to test mismatch.
    Set send_nick=False / send_user=False to omit those commands.

    Returns socket.
    """
    s = connect_raw()
    if send_pass:
        send(s, f"PASS {password}\r\n")
        time.sleep(wait)
    if send_nick:
        send(s, f"NICK {nick}\r\n")
        time.sleep(wait)
    if send_user:
        send(s, f"USER {user} 0 * :{user}\r\n")
        time.sleep(wait)
    if drain_welcome:
        drain(s, timeout=0.3)
    return s

# Backwards‑compat wrapper matching your original helper name
connect_client = register_client

# =============================================================
# Assertion helpers
# =============================================================

# Common IRC numeric replies we care about
ERR_NICKNAMEINUSE   = '433'
ERR_ERRONEUSNICKNAME= '432'
ERR_NEEDMOREPARAMS  = '461'
ERR_ALREADYREGISTRED= '462'
ERR_PASSWDMISMATCH  = '464'
ERR_NOTONCHANNEL    = '404'
ERR_INVITEONLYCHAN  = '473'
ERR_CHANNELISFULL   = '471'
ERR_BADCHANNELKEY   = '475'


def expect_numeric(buf, numeric):
    return numeric in buf


def expect_any_numeric(buf, numerics):
    return any(n in buf for n in numerics)

# =============================================================
# Test Functions (all defined before run_all())
# =============================================================

def test_registration_pass_hi():
    log("\n===== Test: Registration and immediate 'hi' message =====")
    client = connect_client("RegHiUser", "reghiuser")
    time.sleep(0.2)
    hi_msg = "hi"
    send(client, f"PRIVMSG RegHiUser :{hi_msg}\r\n")
    time.sleep(0.2)
    response = recv(client)
    if hi_msg in response:
        print("✅[PASS] Self-message received immediately after registration")
    else:
        print("❌[FAIL] Self-message not received after registration")
    client.close()


def test_modes_extended():
    log("\n===== Test: Extended MODE (+b, +i, +k, +l) =====")
    alice = connect_client("Alice", "aliceuser")
    bob   = connect_client("Bob",   "bobuser")
    carol = connect_client("Carol", "caroluser")
    channel = "#modechannel"

    # Join channel
    for client in [alice, bob, carol]:
        send(client, f"JOIN {channel}\r\n")
        time.sleep(0.3)
        drain(client)

    # +i (invite-only)
    send(alice, f"MODE {channel} +i\r\n")
    time.sleep(0.3)
    drain(alice)

    # Dave tries to join without invite
    dave = connect_client("Dave", "daveuser")
    send(dave, f"JOIN {channel}\r\n")
    time.sleep(0.3)
    err = drain(dave)
    if expect_any_numeric(err, [ERR_INVITEONLYCHAN, 'ERR_INVITEONLYCHAN']):
        print("✅[PASS] Dave blocked on invite-only channel")
    else:
        print("❌[FAIL] Dave joined invite-only channel unexpectedly")

    # +k (password)
    send(alice, f"MODE {channel} +k secret\r\n")
    time.sleep(0.3)
    drain(alice)

    # Carol tries to join with wrong key
    send(carol, f"JOIN {channel} wrongkey\r\n")
    time.sleep(0.3)
    err = drain(carol)
    if expect_any_numeric(err, [ERR_BADCHANNELKEY, 'ERR_BADCHANNELKEY']):
        print("✅[PASS] Wrong key rejected")
    else:
        print("❌[FAIL] Wrong key accepted")

    # +l (limit)
    send(alice, f"MODE {channel} +l 1\r\n")
    time.sleep(0.3)
    drain(alice)

    # Check limit enforcement
    send(dave, f"JOIN {channel} secret\r\n")
    time.sleep(0.3)
    err = drain(dave)
    if expect_any_numeric(err, [ERR_CHANNELISFULL, 'ERR_CHANNELISFULL']):
        print("✅[PASS] Channel limit enforced")
    else:
        print("❌[FAIL] Channel limit not enforced")

    for c in [alice, bob, carol, dave]:
        try:
            c.close()
        except:  # noqa: E722
            pass


def test_join_privmsg_kick_topic():
    log("\n===== Test: JOIN, PRIVMSG, KICK, TOPIC =====")
    alice = connect_client("Alice2", "alice2")
    bob   = connect_client("Bob2",   "bob2")
    carol = connect_client("Carol2", "carol2")
    channel = "#testchannel"

    # Join channel
    for client in [alice, bob, carol]:
        send(client, f"JOIN {channel}\r\n")
        time.sleep(0.3)
        drain(client)

    # Alice sets topic
    send(alice, f"TOPIC {channel} :New Topic\r\n")
    time.sleep(0.3)
    if "New Topic" in drain(alice):
        print("✅[PASS] Topic set successfully")
    else:
        print("❌[FAIL] Topic not updated")

    # Kick Carol (Alice assumed chanop)
    send(alice, f"KICK {channel} Carol2 :No spamming\r\n")
    time.sleep(0.3)
    if "KICK" in drain(carol):
        print("✅[PASS] Carol kicked")
    else:
        print("❌[FAIL] Carol not kicked properly")

    # Carol cannot send message
    send(carol, f"PRIVMSG {channel} :Am I here?\r\n")
    time.sleep(0.3)
    err = drain(carol)
    if expect_any_numeric(err, [ERR_NOTONCHANNEL, 'ERR_NOTONCHANNEL']):
        print("✅[PASS] Carol blocked after kick")
    else:
        print("❌[FAIL] Carol could still message")

    for c in [alice, bob, carol]:
        try:
            c.close()
        except:  # noqa: E722
            pass


def test_partial_message():
    log("\n===== Test: Partial command (nc style) =====")
    client = connect_raw()
    partial = ["PA", "SS pass\r", "\nNICK UserX\r\n", "USER u 0 * :Real\r\n"]
    for part in partial:
        send(client, part)
        time.sleep(0.1)
    time.sleep(0.5)
    drain(client)
    client.close()


def test_pass_missing():
    """Omit PASS entirely; expect either 464 or connection drop at registration."""
    log("\n===== Test: PASS missing =====")
    c = register_client("NoPass", "nopassuser", send_pass=False, drain_welcome=False)
    time.sleep(0.5)
    buf = drain(c)
    if expect_numeric(buf, ERR_PASSWDMISMATCH) or not buf:
        print("✅[PASS] PASS missing detected (mismatch or drop).")
    else:
        print("❌[FAIL] PASS missing not detected.")
    c.close()


def test_pass_wrong():
    """Send wrong PASS; expect 464 (ERR_PASSWDMISMATCH)."""
    log("\n===== Test: PASS wrong =====")
    c = register_client("BadPass", "badpassuser", password="WRONGPASS", drain_welcome=False)
    time.sleep(0.5)
    buf = drain(c)
    if expect_numeric(buf, ERR_PASSWDMISMATCH):
        print("✅[PASS] Wrong PASS rejected (ERR_PASSWDMISMATCH).")
    else:
        print("❌[FAIL] Wrong PASS not properly rejected.")
    c.close()


def test_pass_after_reg():
    """Send PASS *after* completing registration; expect 462 (already registered)."""
    log("\n===== Test: PASS after registration =====")
    c = register_client("LatePass", "latepass")
    send(c, f"PASS somethingelse\r\n")
    time.sleep(0.3)
    buf = drain(c)
    if expect_numeric(buf, ERR_ALREADYREGISTRED):
        print("✅[PASS] PASS after registration rejected (ERR_ALREADYREGISTRED).")
    else:
        print("❌[FAIL] PASS after registration not rejected.")
    c.close()


def test_nick_missing():
    """Send NICK with no parameter: expect 431 (ERR_NONICKNAMEGIVEN) or 461."""
    log("\n===== Test: NICK missing param =====")
    c = connect_raw()
    send(c, f"PASS {PASSWORD}\r\n")
    time.sleep(0.1)
    send(c, "NICK\r\n")
    time.sleep(0.3)
    buf = drain(c)
    # Some servers use 431; if not, treat NEEDMOREPARAMS as acceptable.
    if '431' in buf or expect_numeric(buf, ERR_NEEDMOREPARAMS):
        print("✅[PASS] Missing nick param detected.")
    else:
        print("❌[FAIL] Missing nick param not detected.")
    c.close()


def test_nick_in_use():
    """Two clients choose same nick; second should get 433."""
    log("\n===== Test: NICK already in use =====")
    c1 = register_client("DupNick", "dup1")
    c2 = connect_raw()  # raw so we can control order
    send(c2, f"PASS {PASSWORD}\r\n")
    time.sleep(0.1)
    send(c2, "NICK DupNick\r\n")
    time.sleep(0.3)
    buf = drain(c2)
    if expect_numeric(buf, ERR_NICKNAMEINUSE):
        print("✅[PASS] Duplicate nick rejected (ERR_NICKNAMEINUSE).")
    else:
        print("❌[FAIL] Duplicate nick accepted.")
    c1.close(); c2.close()


def test_nick_invalid_chars():
    """Nick with illegal chars (space, comma, * etc.). Expect 432."""
    log("\n===== Test: NICK invalid characters =====")
    c = connect_raw()
    bad_nick = "bad nick"  # space invalid per RFC
    send(c, f"PASS {PASSWORD}\r\n")
    time.sleep(0.1)
    send(c, f"NICK {bad_nick}\r\n")
    time.sleep(0.3)
    buf = drain(c)
    if expect_numeric(buf, ERR_ERRONEUSNICKNAME):
        print("✅[PASS] Erroneous nick rejected (ERR_ERRONEUSNICKNAME).")
    else:
        print("❌[FAIL] Erroneous nick not rejected.")
    c.close()


def test_user_missing_params():
    """USER requires 4 params: <user> <mode> <unused> :<realname>. Send too few."""
    log("\n===== Test: USER missing params =====")
    c = connect_raw()
    send(c, f"PASS {PASSWORD}\r\n")
    time.sleep(0.1)
    send(c, "NICK UParams\r\n")
    time.sleep(0.1)
    send(c, "USER onlyone\r\n")  # missing mode, unused, realname
    time.sleep(0.3)
    buf = drain(c)
    if expect_numeric(buf, ERR_NEEDMOREPARAMS):
        print("✅[PASS] USER missing params detected (ERR_NEEDMOREPARAMS).")
    else:
        print("❌[FAIL] USER missing params not detected.")
    c.close()


def test_user_already_registered():
    """Send USER twice; second should get 462."""
    log("\n===== Test: USER after registration =====")
    c = register_client("UserTwice", "usertwice")
    send(c, "USER foo 0 * :Bar\r\n")
    time.sleep(0.3)
    buf = drain(c)
    if expect_numeric(buf, ERR_ALREADYREGISTRED):
        print("✅[PASS] Second USER rejected (ERR_ALREADYREGISTRED).")
    else:
        print("❌[FAIL] Second USER not rejected.")
    c.close()


def test_nick_collision_race():
    """Test race condition where two clients try to take same nick simultaneously."""
    log("\n===== Test: Nick collision race =====")
    c1 = connect_raw()
    c2 = connect_raw()
    
    # Send initial commands without waiting
    for c in [c1, c2]:
        send(c, f"PASS {PASSWORD}\r\n")
    
    # Both try to claim same nick
    send(c1, "NICK RaceNick\r\n")
    send(c2, "NICK RaceNick\r\n")
    send(c1, "USER user1 0 * :User One\r\n")
    send(c2, "USER user2 0 * :User Two\r\n")
    
    time.sleep(0.5)
    
    # Drain responses and see who got the nick
    buf1 = drain(c1)
    buf2 = drain(c2)
    
    winner = None
    if "RaceNick" in buf1 and ERR_NICKNAMEINUSE not in buf1:
        winner = c1
        loser = c2
    elif "RaceNick" in buf2 and ERR_NICKNAMEINUSE not in buf2:
        winner = c2
        loser = c1
    
    if winner:
        print(f"✅[PASS] Nick collision resolved (winner: {winner.getsockname()[1]})")
    else:
        print("❌[FAIL] Nick collision not properly resolved")
    
    for c in [c1, c2]:
        try:
            c.close()
        except:
            pass


def test_channel_operator_privileges():
    """Test complex channel operator scenarios."""
    log("\n===== Test: Channel operator privileges =====")
    # Create clients
    alice = connect_client("AliceOp", "aliceop")
    bob = connect_client("BobNorm", "bobnorm")
    carol = connect_client("CarolNorm", "carolnorm")
    channel = "#opertest"
    
    # Alice creates channel and gets op
    send(alice, f"JOIN {channel}\r\n")
    time.sleep(0.2)
    drain(alice)
    
    # Bob joins
    send(bob, f"JOIN {channel}\r\n")
    time.sleep(0.2)
    drain(bob)
    
    # Alice tries to make Bob an operator
    send(alice, f"MODE {channel} +o BobNorm\r\n")
    time.sleep(0.2)
    mode_response = drain(alice)
    
    # Verify Bob got op status
    if "BobNorm" in mode_response and "+o" in mode_response:
        print("✅[PASS] Operator can grant operator status")
    else:
        print("❌[FAIL] Operator could not grant operator status")
    
    # Bob (now op) tries to kick Carol
    send(bob, f"KICK {channel} CarolNorm :testing\r\n")
    time.sleep(0.2)
    kick_response = drain(bob)
    
    if "KICK" in kick_response:
        print("✅[PASS] New operator can kick users")
    else:
        print("❌[FAIL] New operator cannot kick users")
    
    # Carol tries to change topic (should fail)
    send(carol, f"TOPIC {channel} :I shouldn't be able to do this\r\n")
    time.sleep(0.2)
    topic_response = drain(carol)
    
    if "TOPIC" not in topic_response or ERR_NOTONCHANNEL in topic_response:
        print("✅[PASS] Non-operator cannot change topic when not in channel")
    else:
        print("❌[FAIL] Non-operator could change topic")
    
    for c in [alice, bob, carol]:
        try:
            c.close()
        except:
            pass


def test_multiple_channel_operations():
    """Test complex operations across multiple channels with two clients."""
    log("\n===== Test: Multiple channel operations =====")

    clientA = connect_client("MultiChanA", "multichanA")
    clientB = connect_client("MultiChanB", "multichanB")

    channel1 = "#chan1"
    channel2 = "#chan2"
    channel3 = "#chan3"

    # A joins multiple channels
    send(clientA, f"JOIN {channel1},{channel2},{channel3}\r\n")
    time.sleep(0.5)
    join_response = drain(clientA)

    if all(chan in join_response for chan in [channel1, channel2, channel3]):
        print("✅[PASS] Client A joined multiple channels")
    else:
        print("❌[FAIL] Client A failed to join all channels")

    # B joins channel1
    send(clientB, f"JOIN {channel1}\r\n")
    time.sleep(0.3)
    drain(clientB)

    # A sends message to channel1 and channel2
    send(clientA, f"PRIVMSG {channel1},{channel2} :Hello both channels!\r\n")
    time.sleep(0.5)
    responseB = drain(clientB)

    if "Hello both channels!" in responseB:
        print("✅[PASS] Message delivered to channel member")
    else:
        print("❌[FAIL] Message not delivered to channel member")

    # A parts channel2
    send(clientA, f"PART {channel2}\r\n")
    time.sleep(0.3)
    part_response = drain(clientA)

    if f"PART {channel2}" in part_response:
        print("✅[PASS] Successfully parted from channel2")
    else:
        print("❌[FAIL] Could not part from channel2")

    clientA.close()
    clientB.close()



def test_cap_ls_negotiation():
    """Test CAP LS negotiation (modern IRC capability negotiation)."""
    log("\n===== Test: CAP LS negotiation =====")
    client = connect_raw()
    
    # Modern clients start with CAP LS
    send(client, "CAP LS 302\r\n")
    send(client, f"NICK CapTest\r\n")
    send(client, "USER captest 0 * :Cap Test\r\n")
    time.sleep(0.3)
    
    response = drain(client)
    
    # Check if server responds to CAP (even if with empty list)
    if "CAP" in response or "ACK" in response or "NAK" in response:
        print("✅[PASS] Server responded to CAP LS")
    else:
        print("❌[FAIL] Server did not respond to CAP LS")
    
    client.close()



def test_quit_reason_propagation():
    """Test that QUIT reason is propagated to other users."""
    log("\n===== Test: QUIT reason propagation =====")
    alice = connect_client("AliceQ", "aliceq")
    bob = connect_client("BobQ", "bobq")
    channel = "#quittest"
    
    # Both join channel
    send(alice, f"JOIN {channel}\r\n")
    send(bob, f"JOIN {channel}\r\n")
    time.sleep(0.3)
    drain(alice); drain(bob)
    
    # Alice quits with reason
    reason = "Gone to lunch"
    send(alice, f"QUIT :{reason}\r\n")
    time.sleep(0.3)
    
    # Check if Bob sees the quit message
    bob_response = drain(bob)
    if "QUIT" in bob_response and reason in bob_response:
        print("✅[PASS] QUIT reason propagated to other users")
    else:
        print("❌[FAIL] QUIT reason not propagated")
    
    try:
        alice.close()
    except:
        pass
    bob.close()


def test_max_targets_for_privmsg():
    """Test server's handling of maximum targets in PRIVMSG."""
    log("\n===== Test: PRIVMSG max targets =====")

    clientA = connect_client("TargetTestA", "targettestA")
    clientB = connect_client("TargetTestB", "targettestB")

    channel1 = "#target1"
    channel2 = "#target2"
    channel3 = "#target3"

    # Client A joins all channels
    send(clientA, f"JOIN {channel1},{channel2},{channel3}\r\n")
    time.sleep(0.5)
    drain(clientA)

    # Client B joins only channel1
    send(clientB, f"JOIN {channel1}\r\n")
    time.sleep(0.3)
    drain(clientB)

    # Client A tries to send PRIVMSG to multiple targets
    send(clientA, f"PRIVMSG {channel1},{channel2},{channel3} :Hello all\r\n")
    time.sleep(0.5)

    responseA = drain(clientA)  # Check for errors like 407
    responseB = drain(clientB)  # Check if message delivered to channel1

    if "Hello all" in responseB:
        print("✅[PASS] Server delivered multi-target message to at least one recipient")
    elif "407" in responseA or "ERR_TOOMANYTARGETS" in responseA:
        print("✅[PASS] Server rejected multi-target PRIVMSG as expected")
    else:
        print("❌[FAIL] Server did not handle multi-target PRIVMSG correctly")

    clientA.close()
    clientB.close()



def test_ping_pong_handling():
    """Test server's PING/PONG handling and timeout behavior."""
    log("\n===== Test: PING/PONG handling =====")
    client = connect_raw()  # Raw connection to control PING/PONG
    
    # Register but don't respond to PINGs
    send(client, f"PASS {PASSWORD}\r\n")
    send(client, "NICK PingTest\r\n")
    send(client, "USER pingtest 0 * :Ping Test\r\n")
    time.sleep(0.3)
    drain(client)
    
    # Wait for PING (servers typically ping every 60-120 seconds)
    log("Waiting for server PING (up to 10 seconds)...")
    start = time.time()
    while time.time() - start < 10:
        response = recv(client, timeout=1)
        if "PING" in response:
            ping_msg = response.strip()
            print("✅[PASS] Server sent PING")
            # Respond with PONG
            pong_msg = ping_msg.replace("PING", "PONG")
            send(client, f"{pong_msg}\r\n")
            time.sleep(0.3)
            print("✅[PASS] Proper PONG response sent")
            break
    else:
        print("❌[FAIL] Server did not send PING within 10 seconds")
    
    client.close()

# =============================================================
# Master Test Runner (defined after all test functions)
# =============================================================

def run_all():
    # Core existing tests
    test_registration_pass_hi()
    test_modes_extended()
    test_join_privmsg_kick_topic()
    test_partial_message()

    # New PASS tests
    test_pass_missing()
    test_pass_wrong()
    test_pass_after_reg()

    # New NICK tests
    test_nick_missing()
    test_nick_in_use()
    test_nick_invalid_chars()

    # New USER tests
    test_user_missing_params()
    test_user_already_registered()

    # New complex tests
    test_nick_collision_race()
    test_channel_operator_privileges()
    test_multiple_channel_operations()
    test_cap_ls_negotiation()
    test_quit_reason_propagation()
    test_max_targets_for_privmsg()
    test_ping_pong_handling()

    log("\n===== ALL TESTS DONE =====")


if __name__ == "__main__":
    run_all()