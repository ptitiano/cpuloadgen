/*
 *
 * @Component			CPULOADGEN
 * @Filename			cpuloadgen.c
 * @Description			Programmable CPU Load Generator,
 *					based on Dhrystone loops
 * @Author			Patrick Titiano (p-titiano@ti.com)
 * @Date			2010
 * @Copyright			Texas Instruments Incorporated
 *
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "dhry.h"
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#define __USE_GNU
#include <sched.h>
#include <signal.h>


#define CPULOADGEN_REVISION ((const char *) "0.93")


/* #define DEBUG */
#ifdef DEBUG
#define dprintf(format, ...)	 printf(format, ## __VA_ARGS__)
#else
#define dprintf(format, ...)
#endif


#ifndef ROPT
#define REG
#else
#define REG register
#endif

/* Global Variables: */

Rec_Pointer Ptr_Glob, Next_Ptr_Glob;
int Int_Glob;
Boolean Bool_Glob;
char Ch_1_Glob, Ch_2_Glob;
int Arr_1_Glob [50];
int Arr_2_Glob [50][50];

char Reg_Define[] = "Register option selected.";

Enumeration     Func_1();
/* forward declaration necessary since Enumeration may not simply be int */

extern char *builddate;
extern double dtime();


pid_t pid, pid_child;

void dhryStone(unsigned int iterations);
void loadgen(unsigned int cpu, unsigned int load,
	unsigned int duration, unsigned short int need_cpu_affinity);


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		usage
 * @BRIEF		Display list of supported commands.
 * @DESCRIPTION		Display list of supported commands.
 *//*------------------------------------------------------------------------ */
void usage(void)
{
	printf("Usage:\n\n");
	printf(
		"cpuloadgen [<cpu0load>] [<cpu1load>] [<duration>]\n");
	printf(
		"  <cpu0load>: CPU load to be generated on CPU0 (in %%) [1-100]\n");
	printf(
		"  <cpu1load>: CPU load to be generated on CPU1 (in %%) [1-100]\n");
	printf(
		"  <duration>: test duration (in seconds) [0-600]\n\n");
	printf(
		"If <duration> is omitted, then it runs until CTRL+C is pressed.\n\n");
	printf(
		"If <duration> & <cpu1load> are omitted, then it generates <cpu0load> load on any available CPU until CTRL+C is pressed (no affinity, scheduler runtime decision).\n\n");
	printf(
		"If all options are omitted, then it runs until CTRL+C is pressed with a 100%% CPU Load on each core.\n\n");
}


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		sigterm_handler
 * @BRIEF		parent SIGTERM callback function.
 *			Send SIGTERM signal to child process.
 * @DESCRIPTION		parent SIGTERM callback function.
 *			Send SIGTERM signal to child process.
 *//*------------------------------------------------------------------------ */
void sigterm_handler(void)
{
	dprintf("%s(): sending SIGTERM signal to child process (PID %d)\n",
		__func__, pid_child);
	kill(pid_child, SIGTERM);
}


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		main
 * @BRIEF		main entry point
 * @RETURNS		0 on success
 *			-1 in case of incorrect argument
 *			-2 in case of failure to fork
 * @param[in, out]	argc: shell input argument number
 * @param[in, out]	argv: shell input argument(s)
 * @DESCRIPTION		main entry point
 *//*------------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
	unsigned int cpu0load, cpu1load, duration;
	unsigned short int need_cpu_affinity;
	int ret;


	printf("CPULOADGEN (REV %s built %s)\n\n",
		CPULOADGEN_REVISION, builddate);

	if ((argc == 1) || (argc > 4)) {
		usage();
		return -1;
	}

	need_cpu_affinity = 1;
	cpu0load = 100;
	cpu1load = 100;
	duration = 0;

	if (argc == 4) {
		ret = sscanf(argv[1], "%d", &cpu0load);
		if (ret != 1) {
			usage();
			return -1;
		}
		ret = sscanf(argv[2], "%d", &cpu1load);
		if (ret != 1) {
			usage();
			return -1;
		}
		ret = sscanf(argv[3], "%d", &duration);
		if (ret != 1) {
			usage();
			return -1;
		}
	} else if (argc == 3) {
		ret = sscanf(argv[1], "%d", &cpu0load);
		if (ret != 1) {
			usage();
			return -1;
		}
		ret = sscanf(argv[2], "%d", &cpu1load);
		if (ret != 1) {
			usage();
			return -1;
		}
	} else if (argc == 2) {
		dprintf("%s(): argc == 2\n", __func__);
		ret = sscanf(argv[1], "%d", &cpu0load);
		if (ret != 1) {
			usage();
			return -1;
		}
		dprintf("in case of need_cpu_affinity = 0\n", __func__);
		need_cpu_affinity = 0;
		cpu1load = 0;
	}

	if (duration > 600) {
		printf("Incorrect duration (%d)!\n\n", duration);
		usage();
		return -1;
	}
	if (cpu0load > 100) {
		printf("Incorrect CPU0 Load (%d)!\n\n", cpu0load);
		usage();
		return -1;
	}
	if (cpu1load > 100) {
		printf("Incorrect CPU1 Load (%d)!\n\n", cpu1load);
		usage();
		return -1;
	}
	if ((cpu0load == 0) && (cpu1load == 0)) {
		printf("Nothing to do!\n");
		return -1;
	}

	if (need_cpu_affinity == 1) {
		printf("CPU0 Load to generate: %d%%\n", cpu0load);
		printf("CPU1 Load to generate: %d%%\n", cpu1load);
	} else {
		printf("CPU Load to generate: %d%%\n", cpu0load);
	}

	if (duration != 0)
		printf("Duration: %ds\n\n", duration);
	else
		printf("Press CTRL+C to end\n\n");

	if ((cpu0load != 0) && (cpu1load != 0)) {
		/* need to create 2 processes, 1 per CPU core */
		pid = fork();
		if (pid < 0) {
			/* failed to fork */
			fprintf(stderr, "%s(): Failed to fork!\n (%d)",
				__func__, pid);
			return -2;
		} else if (pid == 0) {
			/* Child Process => CPU1 task */
			dprintf("%s(): this is the child process (PID=%d)\n",
				__func__, getpid());
			/* Generate requested load on CPU1 */
			loadgen(1, cpu1load, duration, need_cpu_affinity);
		} else {
			/* Parent Process => CPU0 task */
			pid_child = pid;
			dprintf(
				"%s(): this is the parent process (PID = %d). Child process PID is %d\n",
				__func__, getpid(), pid_child);
			/*
			 * Register signal handler in order to be able to
			 * kill child process if user kills parent process
			 */
			signal(SIGTERM, (sighandler_t) sigterm_handler);
			/* Generate requested load on CPU0 */
			loadgen(0, cpu0load, duration, need_cpu_affinity);
			/*
			 * Parent process completes,
			 * kill child process before returning
			 */
			kill(pid_child, SIGTERM);
		}
	} else if (cpu1load != 0) {
		loadgen(1, cpu1load, duration, need_cpu_affinity);
	} else { /* (cpu0load != 0) */
		loadgen(0, cpu0load, duration, need_cpu_affinity);
	}

	return 0;
}


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		loadgen
 * @BRIEF		Programmable CPU load generator
 * @RETURNS		0 on success
 *			OMAPCONF_ERR_CPU
 *			OMAPCONF_ERR_ARG
 *			OMAPCONF_ERR_REG_ACCESS
 * @param[in]		cpu: target CPU core ID (loaded CPU core)
 * @param[in]		load: load to generate on that CPU ([1-100])
 * @param[in]		duration: how long this CPU core shall be loaded
 *				(in seconds)
 * @param[in]		need_cpu_affinity: flag to be set when affinity shall be
 *				used.
 * @DESCRIPTION		Programmable CPU load generator. Use Dhrystone loops
 *			to generate load, and apply PWM (Pulse Width Modulation)
 *			principle on it to make average CPU load vary between
 *			0 and 100%
 *//*------------------------------------------------------------------------ */
void loadgen(unsigned int cpu, unsigned int load,
	unsigned int duration, unsigned short int need_cpu_affinity)
{
	double dhrystone_start_time, dhrystone_end_time;
	double idle_time_us;
	double loadgen_start_time_us, active_time_us;
	double total_time_us;
	struct timeval tv_cpuloadgen_start, tv_cpuloadgen;
	struct timeval tv_idle_start, tv_idle_stop;
	struct timezone tz;
	double time_us;
	unsigned long mask;
	unsigned int len = sizeof(mask);
	cpu_set_t set;

	if (need_cpu_affinity == 1) {
		CPU_ZERO(&set);
		if (cpu == 0)
			CPU_SET(0, &set);
		else
			CPU_SET(1, &set);
		sched_setaffinity(0, len, &set);
		printf("Generating Load on CPU%d...\n", cpu);
	} else {
		printf("Generating Load ...\n");
	}
	gettimeofday(&tv_cpuloadgen_start, &tz);
	loadgen_start_time_us = ((double) tv_cpuloadgen_start.tv_sec
		+ ((double) tv_cpuloadgen_start.tv_usec * 1.0e-6));
	dprintf("%s(): CPU%d start time: %fus\n", __func__,
		cpu, loadgen_start_time_us);

	if (load != 100) {
		while (1) {
			/* Generate load (100%) */
			dhrystone_start_time = dtime();
			dhryStone(200000);
			dhrystone_end_time = dtime();
			active_time_us =
				(dhrystone_end_time - dhrystone_start_time) * 1.0e6;
			dprintf("%s(): CPU%d running time: %dus\n", __func__,
				cpu, (unsigned int) active_time_us);

			/* Compute needed idle time */
			idle_time_us = total_time_us - active_time_us;
			dprintf("%s(): CPU%d computed idle_time_us = %dus\n",
				__func__, cpu, (unsigned int) idle_time_us);
			total_time_us =
				active_time_us * (100.0 / (double) (load + 1));
			dprintf("%s(): CPU%d total time: %dus\n", __func__, cpu,
				(unsigned int) total_time_us);

			/* Generate idle time */
			#ifdef DEBUG
			gettimeofday(&tv_idle_start, &tz);
			#endif
			usleep((unsigned int) (idle_time_us));
			#ifdef DEBUG
			gettimeofday(&tv_idle_stop, &tz);
			idle_time_us = 1.0e6 * (
				((double) tv_idle_stop.tv_sec +
				((double) tv_idle_stop.tv_usec * 1.0e-6))
				- ((double) tv_idle_start.tv_sec +
				((double) tv_idle_start.tv_usec * 1.0e-6)));
			dprintf("%s(): CPU%d effective idle time: %dus\n",
				__func__, cpu, (unsigned int) idle_time_us);
			dprintf("%s(): CPU%d effective CPU Load: %d%%\n",
				__func__, cpu,
				(unsigned int) (100.0 * (active_time_us /
				(active_time_us + idle_time_us))));
			#endif
			gettimeofday(&tv_cpuloadgen, &tz);
			time_us = ((double) tv_cpuloadgen.tv_sec
				+ ((double) tv_cpuloadgen.tv_usec * 1.0e-6));
			dprintf("%s(): CPU%d elapsed time: %fs\n",
				__func__, cpu,
				time_us - loadgen_start_time_us);
			if ((duration != 0)
				&& (time_us - loadgen_start_time_us) >= duration)
				break;
		}
	} else {
		while (1) {
			dhryStone(1000000);
			gettimeofday(&tv_cpuloadgen, &tz);
			time_us = ((double) tv_cpuloadgen.tv_sec
				+ ((double) tv_cpuloadgen.tv_usec * 1.0e-6));
			dprintf("%s(): CPU%d elapsed time: %fs\n", __func__,
				cpu, time_us - loadgen_start_time_us);
			if ((duration != 0)
				&& (time_us - loadgen_start_time_us) >= duration)
				break;
		}
	}

	printf("Load Generation on CPU%d stopped.\n", cpu);
}


void dhryStone(unsigned int iterations)
{
	One_Fifty Int_1_Loc;
	REG One_Fifty Int_2_Loc;
	One_Fifty Int_3_Loc;
	REG char Ch_Index;
	Enumeration Enum_Loc;
	Str_30 Str_1_Loc;
	Str_30 Str_2_Loc;
	REG int Run_Index;
	REG int Number_Of_Runs;

	FILE *Ap;
	unsigned int mrcData;

	Next_Ptr_Glob = (Rec_Pointer) malloc(sizeof(Rec_Type));
	Ptr_Glob = (Rec_Pointer) malloc(sizeof (Rec_Type));

	Ptr_Glob->Ptr_Comp = Next_Ptr_Glob;
	Ptr_Glob->Discr = Ident_1;
	Ptr_Glob->variant.var_1.Enum_Comp = Ident_3;
	Ptr_Glob->variant.var_1.Int_Comp = 40;
	strcpy (Ptr_Glob->variant.var_1.Str_Comp,
	"DHRYSTONE PROGRAM, SOME STRING");
	strcpy (Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");

	Arr_2_Glob [8][7] = 10;
    /* Was missing in published program. Without this statement,
    Arr_2_Glob [8][7] would have an undefined value.
    Warning: With 16-Bit processors and Number_Of_Runs > 32000,
    overflow may occur for this array element. */

    Number_Of_Runs = iterations;
	for (Run_Index = 1; Run_Index <= Number_Of_Runs; ++Run_Index) {
		Proc_5();
		Proc_4();
		/* Ch_1_Glob == 'A', Ch_2_Glob == 'B', Bool_Glob == true */
		Int_1_Loc = 2;
		Int_2_Loc = 3;
		strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");
		Enum_Loc = Ident_2;
		Bool_Glob = ! Func_2 (Str_1_Loc, Str_2_Loc);
		/* Bool_Glob == 1 */
		while (Int_1_Loc < Int_2_Loc) { /* loop body executed once */
			Int_3_Loc = 5 * Int_1_Loc - Int_2_Loc;
			/* Int_3_Loc == 7 */
			Proc_7 (Int_1_Loc, Int_2_Loc, &Int_3_Loc);
			/* Int_3_Loc == 7 */
			Int_1_Loc += 1;
		}
		/* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
		Proc_8 (Arr_1_Glob, Arr_2_Glob, Int_1_Loc, Int_3_Loc);
		/* Int_Glob == 5 */
		Proc_1 (Ptr_Glob);
		for (Ch_Index = 'A'; Ch_Index <= Ch_2_Glob; ++Ch_Index) {
		/* loop body executed twice */
			if (Enum_Loc == Func_1 (Ch_Index, 'C')) {
				/* then, not executed */
				Proc_6 (Ident_1, &Enum_Loc);
				strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 3'RD STRING");
				Int_2_Loc = Run_Index;
				Int_Glob = Run_Index;
			}
		}
		/* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
		Int_2_Loc = Int_2_Loc * Int_1_Loc;
		Int_1_Loc = Int_2_Loc / Int_3_Loc;
		Int_2_Loc = 7 * (Int_2_Loc - Int_3_Loc) - Int_1_Loc;
		/* Int_1_Loc == 1, Int_2_Loc == 13, Int_3_Loc == 7 */
		Proc_2 (&Int_1_Loc);
		/* Int_1_Loc == 5 */
	}
}


Proc_1 (Ptr_Val_Par)
/******************/

REG Rec_Pointer Ptr_Val_Par;
    /* executed once */
{
  REG Rec_Pointer Next_Record = Ptr_Val_Par->Ptr_Comp;
                                        /* == Ptr_Glob_Next */
  /* Local variable, initialized with Ptr_Val_Par->Ptr_Comp,    */
  /* corresponds to "rename" in Ada, "with" in Pascal           */

  structassign (*Ptr_Val_Par->Ptr_Comp, *Ptr_Glob);
  Ptr_Val_Par->variant.var_1.Int_Comp = 5;
  Next_Record->variant.var_1.Int_Comp
        = Ptr_Val_Par->variant.var_1.Int_Comp;
  Next_Record->Ptr_Comp = Ptr_Val_Par->Ptr_Comp;
  Proc_3 (&Next_Record->Ptr_Comp);
    /* Ptr_Val_Par->Ptr_Comp->Ptr_Comp
                        == Ptr_Glob->Ptr_Comp */
  if (Next_Record->Discr == Ident_1)
    /* then, executed */
  {
    Next_Record->variant.var_1.Int_Comp = 6;
    Proc_6 (Ptr_Val_Par->variant.var_1.Enum_Comp,
           &Next_Record->variant.var_1.Enum_Comp);
    Next_Record->Ptr_Comp = Ptr_Glob->Ptr_Comp;
    Proc_7 (Next_Record->variant.var_1.Int_Comp, 10,
           &Next_Record->variant.var_1.Int_Comp);
  }
  else /* not executed */
    structassign (*Ptr_Val_Par, *Ptr_Val_Par->Ptr_Comp);
} /* Proc_1 */

Proc_2 (Int_Par_Ref)
/******************/
    /* executed once */
    /* *Int_Par_Ref == 1, becomes 4 */

One_Fifty   *Int_Par_Ref;
{
  One_Fifty  Int_Loc;
  Enumeration   Enum_Loc;

  Int_Loc = *Int_Par_Ref + 10;
  do /* executed once */
    if (Ch_1_Glob == 'A')
      /* then, executed */
    {
      Int_Loc -= 1;
      *Int_Par_Ref = Int_Loc - Int_Glob;
      Enum_Loc = Ident_1;
    } /* if */
  while (Enum_Loc != Ident_1); /* true */
} /* Proc_2 */


Proc_3 (Ptr_Ref_Par)
/******************/
    /* executed once */
    /* Ptr_Ref_Par becomes Ptr_Glob */

Rec_Pointer *Ptr_Ref_Par;

{
  if (Ptr_Glob != Null)
    /* then, executed */
    *Ptr_Ref_Par = Ptr_Glob->Ptr_Comp;
  Proc_7 (10, Int_Glob, &Ptr_Glob->variant.var_1.Int_Comp);
} /* Proc_3 */


Proc_4 () /* without parameters */
/*******/
    /* executed once */
{
  Boolean Bool_Loc;

  Bool_Loc = Ch_1_Glob == 'A';
  Bool_Glob = Bool_Loc | Bool_Glob;
  Ch_2_Glob = 'B';
} /* Proc_4 */


Proc_5 () /* without parameters */
/*******/
    /* executed once */
{
  Ch_1_Glob = 'A';
  Bool_Glob = false;
} /* Proc_5 */
