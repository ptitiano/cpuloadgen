###############################################################################
#                                                                             #
#                                  CPULOADGEN                                 #
#                                                                             #
#         A Programmable CPU Load Generator, based on Dhrystone loops         #
###############################################################################



ABOUT:
------

CPULOADGEN is a Linux user-space standalone application designed to provide a
quick'n easy way to generate programmable load on CPU core(s).

To generate load, it reuses DHRYSTONE loops, applying on top of it sort of
PWM (Pulse Width Modulation) principle: by generating active (100% load) and
idle (0% load) with programmable duty cycle,
it generates an average CPU load of [1-100]%.


SUPPORT:
--------
CPULOADGEN is a generic application, it can run on any CPU architecture.

Please note however that application development is still under progress,
so currently it can only generates loads on mono or dual-core architectures
(extra cores will not be handled). This will be improved to address any number of
CPU cores in the future.


THIS SOFTWARE IS PROVIDED AS IS, WITH NO SUPPORT OR MAINTENANCE COMMITMENT FROM
TEXAS INSTRUMENTS INCORPORATED.



Build instructions:
-------------------
To only build the output binary file:

	# make cpuloadgen

NB: CROSS_COMPILE variable must be set to point to the correct compiler.
	E.g: (for ARM architecture)

	# export CROSS_COMPILE=arm-none-linux-gnueabi-

	or

	# make CROSS_COMPILE=arm-none-linux-gnueabi- cpuloadgen


To build and install cpuloadgen:

	# make DESTDIR=<YOUR_DIR> install

Where "YOUR_DIR" is a destination directory where cpuloadgen output binary file
will be installed (copied, e.g. ubuntu/android filesystem).

That's it!



Usage:
-----

cpuloadgen [cpu0load] [cpu1load] [duration]
  cpu0load: CPU load to be generated on CPU0 (in %) [1-100]
  cpu1load: CPU load to be generated on CPU1 (in %) [1-100]
  duration: test duration (in seconds) [0-600]

If duration is omitted, then it runs until CTRL+C is pressed.
If duration & cpu1load are omitted, then it generates cpu0load load on any
available CPU until CTRL+C is pressed (no affinity, scheduler runtime decision).
If all options are omitted, then it runs until CTRL+C is pressed with
a 100% CPU Load on each core.
