#!/bin/bash
set -e
cd /mnt/c/Users/Ron/.grok/worktrees/ms-sdr-linux
pkill -x ms-sdr 2>/dev/null || true
sleep 0.5
./ms-sdr > /tmp/ms-sdr-cat-test.out 2>&1 &
MSPID=$!
echo "pid=$MSPID"
for i in $(seq 1 40); do
  if grep -q "Kenwood CAT PTY ready" /tmp/ms-sdr-cat-test.out 2>/dev/null; then
    break
  fi
  if ! kill -0 "$MSPID" 2>/dev/null; then
    echo "ms-sdr died early"
    cat /tmp/ms-sdr-cat-test.out
    exit 1
  fi
  sleep 0.25
done
grep -E "PTY|Point digital|MessageBox" /tmp/ms-sdr-cat-test.out || true
ls -la "$HOME/ms-sdr-cat"

perl -e '
use strict;
use IO::Handle;
my $path = "$ENV{HOME}/ms-sdr-cat";
open(my $fh, "+<", $path) or die "open $path: $!";
$fh->autoflush(1);
select(undef, undef, undef, 0.3);
print $fh "ID;";
print "sent ID;\n";
my $rin = "";
vec($rin, fileno($fh), 1) = 1;
my $buf = "";
my $end = time() + 3;
while (time() < $end) {
  my $nfound = select(my $rout = $rin, undef, undef, 0.5);
  if ($nfound && vec($rout, fileno($fh), 1)) {
    my $chunk = "";
    my $n = sysread($fh, $chunk, 64);
    last if !defined $n || $n == 0;
    $buf .= $chunk;
    print "got [$chunk]\n";
    last if index($buf, ";") >= 0;
  }
}
close $fh;
print "reply: [$buf]\n";
if ($buf =~ /ID019;/ || ($buf =~ /ID/ && $buf =~ /;/)) {
  print "CAT_OK\n";
  exit 0;
}
print "CAT_FAIL\n";
exit 2;
'
EC=$?
echo "perl_ec=$EC"
grep -E "Error Reading|open_comm_port -> OK|Comms_port_thread. Started|NORMAL EXIT" "$HOME/ms-sdr.log" | tail -20 || true
kill "$MSPID" 2>/dev/null || true
wait "$MSPID" 2>/dev/null || true
exit $EC
