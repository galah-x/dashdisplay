#!/usr/bin/perl
#    -*- Mode: cperl     -*-
# emacs automagically updates the timestamp field on save
# my $ver =  'udp server to test dashdisplay  Time-stamp: "2025-06-20 16:08:25 john"';

# use as udp_tester.pl --ip <display_ip_adddr>

# this is a udp message generator to exercise/test my dash display monitor
# for a batriumwatchmon BMS.This replaces the Batrium for display testing.
# It runs each of the display fields through about a dozen canned values. 

# As there is no batrium, I run this app on a machine on my local wifi.
# Use the serial port on the dash display to change the password and SSID on
# the dashdisplay to also connect to local same wifi. 

# most works. Except the float value for shunt current. Seems float formats
# are more varied. I have yet to confirm the batrium and dashdisplay work
# together on the float.
# I used an intel laptop and the float formats definitely differ.

use strict;
use warnings;

my $destmc_ip;
my $inc=1;
use Getopt::Long;
&GetOptions (
    'ip=s', \$destmc_ip,
    'i=n', \$inc
    );

use IO::Socket;
my $handle = IO::Socket::INET->new(Proto => 'udp') 
    or die "socket: $@";     # yes, it uses $@ here


my @min_cell_v = (3.0, 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.001);
my @max_cell_v = (3.4, 3.5, 3.6, 3.777, 3.888, 3.909, 4.012, 4.1, 4.2);
my @pack_v = (50.0, 51.11, 52.23, 53.34, 54.45, 55.56, 56.67, 57.7, 58.08, 59.9, 60.0, 61.1, 62.2, 63.3,
		  64.4, 65.5, 66.6);

my @min_cell_t = (-10, 0, 1, 2, 10, 20, 30, 40, 50, 100);
my @max_cell_t = (-5, 1, 2, 10, 20, 30, 40, 50, 60, 120);
#my @pack_i = (1, 10, 40, 70, 100, 150, 0.0, 1.1, 2.222);
my @pack_i = (0x0e26e3c0, 0x0efec3c0, 0x0fd6a4c0, 0x1050a5c0);
my @soc = (1, 10,5, 50.5, 99);



my $msgnum = 1;
my $msgnum3233 = 1;

my $ipaddr   = inet_aton($destmc_ip);
my $portaddr = sockaddr_in(18542, $ipaddr);


my $MSG;

while  (1) {
    if (!(($msgnum % 10) == 0))
      {
	$MSG = next_msg(0x5732);
	send($handle, $MSG, 0, $portaddr) == length($MSG)
	  or die "cannot send to $ipaddr:18452: $!";
      }
    else 
      {
	$MSG = next_msg(0x3233);
	send($handle, $MSG, 0, $portaddr) == length($MSG)
	  or die "cannot send to $ipaddr:18452: $!";
      }
    sleep 2;
  }


sub next_min_cell_v
{
    my $values = @min_cell_v;
    my $this_index = $msgnum % $values;
    my $this = $min_cell_v[$this_index];
    my $ret = int($this*1000);
    printf "min_cell_v = $ret\n"; 
    return $ret;
}
sub next_max_cell_v
{
    my $values = @max_cell_v;
    my $this_index = $msgnum % $values;
    my $this = $max_cell_v[$this_index];
    my $ret = int($this*1000);
    printf "max_cell_v = $ret\n"; 
    return $ret;
  }
sub next_min_cell_t
{
    my $values = @min_cell_t;
    my $this_index = $msgnum % $values;
    my $this = $min_cell_t[$this_index];
    my $ret = int($this+40);
    printf "min_cell_t = $ret\n"; 
    return $ret;
  }
sub next_max_cell_t
{
    my $values = @max_cell_t;
    my $this_index = $msgnum3233 % $values;
    my $this = $max_cell_t[$this_index];
    my $ret = int($this+40);
    printf "max_cell_t = $ret\n"; 
    return $ret;
  }
sub next_pack_v
{
    my $values = @pack_v;
    my $this_index = $msgnum % $values;
    my $this = $pack_v[$this_index];
    my $ret = int($this*100);
    printf "pack_v = $ret\n"; 
    return $ret;
  }
sub next_pack_i
{
    my $values = @pack_i;
    my $this_index = $msgnum % $values;
    my $this = $pack_i[$this_index];
    printf "pack_i = %8x\n", $this;   # the last term is the first off the call stack.
    return ($this>>24) & 0xff, ($this>>16) & 0xff, ($this>>8) & 0xff, $this & 0xff   ; 
}
sub next_soc
{
    my $values = @soc;
    my $this_index = $msgnum % $values;
    my $this = $soc[$this_index];
    my $ret =  int($this*2+10);
    printf "soc = $ret\n";
    return $ret;
}
# /*
# 0  A
# 1  v
# 3  x5  0's
# 8   A8 
# 16  A2
# 18  A2
# 20  A4
# 24  A2
# 26  A
# 27  A
# 28  A
# 29  A
# 30  A
# 31  v 
# 33  v
# 35  A2
# 37  C
# 38  A
# 39  A
# 40  A
# 41  C
# 42  v
# 44  f
# 48  A
# 49  A",
# */




sub next_msg {
    my $type = shift;
    my $msg;
    if ($type == 0x5732) {#    t op        T  S
	$msg = pack("Avx5A8A2A2A4A2AAAAAvvA2CAAACvCCCCAA",
		    ':',    # 0 header
		    0x5732, # 1 msg type
		          # 3 get to offset 8
		    1, # 8 system code
		    2, # 16 firmware version
		    3, # 18 device time
		    4, # 20 system op status
		    5, # 24 system auth mode
		    6, # 26 critical bat ok state
		    7, # 27 critical power rate state
		    8, # 28 discharge power rate state
		    9, # 29 heat on state
		    10, # 30 cool on state
		    next_min_cell_v(), # 31 min cell V
		    next_max_cell_v(), # 33 min cell V
                    0, # 35 avg cell v
		    next_min_cell_t(), # 37
		    0, # 38 num active cellmons
		    0, # 39 cmu port rx
		    0, # 40 cmu poller mode
		    next_soc(), # 41
		    next_pack_v(), # 42
		    next_pack_i(), #44
		    0,  # 48 shunt status
		    0  # 49 shunt rx ticks
		   );
	printf "5732 msg len = %d\n", length($msg); 
      }
    elsif ($type == 0x3233) {#    t op        T  S
      
	$msg = pack("AvA16CA37",
		    ':',    # header
		    0x3233, # msg type
		    0,      # get to offset 19
		    next_max_cell_t(), # 19
		    0  # last is o/s 56
		   );
	if ($inc) {
	  $msgnum3233++;
	}
      }
    if ($inc) {
      $msgnum++;
    }
    return $msg;
  }
 

