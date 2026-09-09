#!/bin/bash
set -e
cd /mnt/c/Users/Ron/.grok/worktrees/SDRcore-trans-linux
pkill -x sdrcore-trans 2>/dev/null || true
sleep 0.3
rm -f "$HOME/sdrcore-trans.log"
./sdrcore-trans > /tmp/sdrcore-trans.out 2>&1 &
PID=$!
echo "pid=$PID"
for i in $(seq 1 40); do
  if grep -q "UDP Thread. Waiting for data\|listen :9200\|SDRCORE_NO_USB" "$HOME/sdrcore-trans.log" 2>/dev/null; then
    break
  fi
  if ! kill -0 "$PID" 2>/dev/null; then
    echo "died early"
    cat /tmp/sdrcore-trans.out
    tail -40 "$HOME/sdrcore-trans.log" 2>/dev/null || true
    exit 1
  fi
  sleep 0.25
done
echo "==== log ===="
grep -E "Setup_UDP|UDP Thread|listen|NO_USB" "$HOME/sdrcore-trans.log" | tail -20
ss -ulnp 2>/dev/null | grep 9200 || true
perl -e '
use IO::Socket::INET;
my $s = IO::Socket::INET->new(PeerAddr=>"127.0.0.1", PeerPort=>9200, Proto=>"udp") or die $!;
for (1..3) { $s->send(pack("CC", 0xF4, 1)); print "sent KA $_\n"; select(undef,undef,undef,0.2); }
'
sleep 0.4
grep KEEP_ALIVE "$HOME/sdrcore-trans.log" | tail -5 || true
if grep -q "CMD_SET_KEEP_ALIVE" "$HOME/sdrcore-trans.log"; then
  echo "UDP_OK"
  EC=0
else
  echo "UDP_FAIL"
  EC=1
fi
kill "$PID" 2>/dev/null || true
wait "$PID" 2>/dev/null || true
exit $EC
