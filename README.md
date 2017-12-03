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
	# cpuloadgen [<cpu[n]=load>] [<duration=time>]

Load is a percentage which may be any integer value between 1 and 100.

Duration time unit is seconds. If duration is omitted, generate load(s) until
CTRL+C is pressed.

Arguments may be provided in any order.

If no argument is given, generate 100% load on all online CPU cores
indefinitely.

E.g.:
Generate 100% load on all online CPU cores until CTRL+C is pressed:

	# cpuloadgen

Generate 100% load on all online CPU cores during 10 seconds:

	# cpuloadgen duration=10

Generate 50% load on CPU1 and 100% load on CPU3 during 5 seconds:

	# cpuloadgen cpu3=100 cpu1=50 duration=5
