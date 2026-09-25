#!/bin/bash
# Smoke-test sdrcore-recv UDP listen on :9000 and keep-alive (0xF4)
set -e
cd /mnt/c/Users/Ron/.grok/worktrees/SDRcore-recv
pkill -x sdrcore-recv 2>/dev/null || true
sleep 0.3
./sdrcore-recv > /tmp/sdrcore-recv.out 2>&1 &
PID=$!
echo "sdrcore-recv pid=$PID"
for i in $(seq 1 40); do
  if grep -q "UDP Thread. Running" "$HOME/sdrcore-recv.log" 2>/dev/null; then
    break
  fi
  if ! kill -0 "$PID" 2>/dev/null; then
    echo "died early"
    cat /tmp/sdrcore-recv.out
    tail -40 "$HOME/sdrcore-recv.log" 2>/dev/null || true
    exit 1
  fi
  sleep 0.25
done
echo "==== log (UDP setup) ===="
grep -E "Setup_UDP|UDP Thread|listen|PortAudio|NO_USB" "$HOME/sdrcore-recv.log" | tail -30
echo "==== sockets ===="
ss -ulnp 2>/dev/null | grep -E '9000|8888' || netstat -ulnp 2>/dev/null | grep -E '9000|8888' || true
echo "==== send keep-alive 0xF4 x3 ===="
# CMD_SET_KEEP_ALIVE = 0xF4, payload byte 1
perl -e '
use IO::Socket::INET;
my $s = IO::Socket::INET->new(PeerAddr=>"127.0.0.1", PeerPort=>9000, Proto=>"udp")
  or die $!;
for (1..3) {
  $s->send(pack("CC", 0xF4, 1));
  print "sent KA $_\n";
  select(undef,undef,undef,0.2);
}
'
sleep 0.5
echo "==== KA log lines ===="
grep -E "KEEP_ALIVE|keep_alive|count=" "$HOME/sdrcore-recv.log" | tail -10 || true
if grep -q "CMD_SET_KEEP_ALIVE" "$HOME/sdrcore-recv.log"; then
  echo "UDP_OK"
  EC=0
else
  echo "UDP_FAIL — no keep-alive seen in log"
  EC=1
fi
kill "$PID" 2>/dev/null || true
wait "$PID" 2>/dev/null || true
exit $EC
