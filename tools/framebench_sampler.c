/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 *
 * Preloaded into openhpl1 by tools/framebench.py (Linux only). With
 * FRAMEBENCH_OUT set it:
 *   - samples the main thread's stack 1000 times a second of wall time,
 *     tagging each sample with the number of frames swapped so far;
 *   - writes the time before and after every SDL_GL_SwapWindow.
 * At exit it writes <out>/samples.bin, <out>/maps.txt (/proc/self/maps,
 * for symbols) and <out>/swaps.txt. Without FRAMEBENCH_OUT it does nothing.
 *
 * Stacks are only taken between SIGUSR1 and SIGUSR2, which framebench.py
 * sends around each measurement, not while maps load. The unwinder can
 * fault on a stack it was not meant to walk (a signal in the middle of
 * libc); such a sample is dropped instead of crashing the game.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define MAX_SAMPLES (1 << 21)
#define DEPTH 48

static void **gpStacks;
static uint32_t *gpLengths, *gpFrames;
static volatile long glSamples;
static pid_t glMainThread;
static volatile uint32_t glSwaps;
static FILE *gpSwapFile;
static int gbDumped;
static volatile sig_atomic_t gbSampling, gbInSample;
static sigjmp_buf gFaultJump;
static struct sigaction gPrevSegv, gPrevBus;

static double Now(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec * 1e-9;
}

static void OnSample(int s, siginfo_t *i, void *c)
{
	(void)s; (void)i; (void)c;
	if(!gbSampling || syscall(SYS_gettid) != glMainThread) return;
	long k = glSamples;
	if(k >= MAX_SAMPLES) return;
	gbInSample = 1;
	if(sigsetjmp(gFaultJump, 1) == 0)
	{
		gpLengths[k] = backtrace(gpStacks + (size_t)k * DEPTH, DEPTH);
		gpFrames[k] = glSwaps;
		glSamples = k + 1;
	}
	gbInSample = 0;
}

static void OnFault(int alSignal, siginfo_t *apInfo, void *apContext)
{
	if(gbInSample) siglongjmp(gFaultJump, 1);
	// Not ours: as if this handler had not been installed
	struct sigaction *pPrev = alSignal == SIGSEGV ? &gPrevSegv : &gPrevBus;
	if(pPrev->sa_flags & SA_SIGINFO)
		pPrev->sa_sigaction(alSignal, apInfo, apContext);
	else if(pPrev->sa_handler != SIG_DFL && pPrev->sa_handler != SIG_IGN)
		pPrev->sa_handler(alSignal);
	else
	{
		signal(alSignal, SIG_DFL);
		raise(alSignal);
	}
}

static void OnToggle(int alSignal)
{
	gbSampling = alSignal == SIGUSR1;
}

static void Dump(void)
{
	const char *pOut = getenv("FRAMEBENCH_OUT");
	if(pOut == NULL || gbDumped) return;
	gbDumped = 1;
	char sPath[1024];
	snprintf(sPath, sizeof sPath, "%s/samples.bin", pOut);
	FILE *pFile = fopen(sPath, "wb");
	if(pFile)
	{
		long n = glSamples;
		for(long k = 0; k < n; k++)
		{
			fwrite(&gpFrames[k], 4, 1, pFile);
			fwrite(&gpLengths[k], 4, 1, pFile);
			fwrite(gpStacks + (size_t)k * DEPTH, sizeof(void *), gpLengths[k], pFile);
		}
		fclose(pFile);
	}
	snprintf(sPath, sizeof sPath, "%s/maps.txt", pOut);
	FILE *pIn = fopen("/proc/self/maps", "r"), *pMaps = fopen(sPath, "w");
	char sLine[1024];
	while(pIn && pMaps && fgets(sLine, sizeof sLine, pIn)) fputs(sLine, pMaps);
	if(pIn) fclose(pIn);
	if(pMaps) fclose(pMaps);
	if(gpSwapFile) fclose(gpSwapFile);
	gpSwapFile = NULL;
}

__attribute__((constructor)) static void Init(void)
{
	const char *pOut = getenv("FRAMEBENCH_OUT");
	if(pOut == NULL) return;
	glMainThread = getpid();
	gpStacks = malloc((size_t)MAX_SAMPLES * DEPTH * sizeof(void *));
	gpLengths = malloc(MAX_SAMPLES * 4);
	gpFrames = malloc(MAX_SAMPLES * 4);
	if(!gpStacks || !gpLengths || !gpFrames) return;
	void *vWarm[4];
	backtrace(vWarm, 4); // loads libgcc before the first signal
	char sPath[1024];
	snprintf(sPath, sizeof sPath, "%s/swaps.txt", pOut);
	gpSwapFile = fopen(sPath, "w");

	struct sigaction action;
	memset(&action, 0, sizeof action);
	action.sa_sigaction = OnSample;
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	sigaction(SIGPROF, &action, NULL);
	action.sa_sigaction = OnFault;
	action.sa_flags = SA_SIGINFO | SA_NODEFER;
	sigaction(SIGSEGV, &action, &gPrevSegv);
	sigaction(SIGBUS, &action, &gPrevBus);
	struct sigaction toggle;
	memset(&toggle, 0, sizeof toggle);
	toggle.sa_handler = OnToggle;
	toggle.sa_flags = SA_RESTART;
	sigaction(SIGUSR1, &toggle, NULL);
	sigaction(SIGUSR2, &toggle, NULL);
	// Wall time, so time blocked in the driver counts as well
	struct sigevent event;
	memset(&event, 0, sizeof event);
	event.sigev_notify = SIGEV_THREAD_ID;
	event.sigev_signo = SIGPROF;
	event._sigev_un._tid = glMainThread;
	timer_t timer;
	if(timer_create(CLOCK_MONOTONIC, &event, &timer) == 0)
	{
		struct itimerspec period = {{0, 1000000}, {0, 1000000}};
		timer_settime(timer, 0, &period, NULL);
	}
	atexit(Dump);
}

bool SDL_GL_SwapWindow(void *apWindow)
{
	static bool (*pReal)(void *);
	if(pReal == NULL) pReal = (bool (*)(void *))dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
	double fBefore = Now();
	bool bResult = pReal(apWindow);
	double fAfter = Now();
	if(gpSwapFile) fprintf(gpSwapFile, "%u %.6f %.6f\n", glSwaps, fBefore, fAfter);
	glSwaps++;
	return bResult;
}

// The game may leave through _exit, which skips atexit
void _exit(int alCode)
{
	static void (*pReal)(int);
	if(pReal == NULL) pReal = (void (*)(int))dlsym(RTLD_NEXT, "_exit");
	Dump();
	pReal(alCode);
	__builtin_unreachable();
}
