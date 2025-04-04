# CPUHOG

"When a hog proudly stomps on the cores of your processor."


`cpuhog` is a very simple command that do active loops on the CPU cores indicated on command line.

This may seems silly, but it is useful when experimenting with scheduling, realtime, core isolation...


A number of options may be used to configure the behaviour of the hog:

- `-c <list>` or `--core <list>` indicates a list of core to run loop. By default all cores are used. The comma-separated list may contain intervals. Ex: `-c 1,3,6-8,11`
- `-d <seconds> or `--duration <seconds>` limit the execution time of the loop. By default the duration is 60 seconds.
- `f <prio>` or `--fifo <prio>` run the loops under the FIFO realtime scheduling. Be careful, this may freeze your system.
- `r <prio>` or `--rr <prio>` run the loops under the RR (Round Robin) realtime scheduling. Be careful, this may freeze your system.
- `-y` or `--yes` answer `yes` to all questions (to validate use of realtime scheduling for example).

