/** ============================================================================
 *  @file   main.c
 *
 *  @path   $(DSPLINK)/gpp/src/samples/ring_io/Linux/
 *
 *  @desc   Linux specific implementation of ring_io sample application's driver.
 *
 *  @ver    1.65.00.02
 *  ============================================================================
 *  Copyright (C) 2002-2009, Texas Instruments Incorporated -
 *  http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  ============================================================================
 */

/*  ----------------------------------- OS Specific Headers           */
#include <stdio.h>
#include <stdlib.h>

/*  ----------------------------------- DSP/BIOS Link                 */
#include <dsplink.h>

/*  ----------------------------------- Application Header            */
#include <ring_io_os.h>
#include <ring_io.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/** ============================================================================
 *  @func   main
 *
 *  @desc   Entry point for the application
 *
 *  @modif  None
 *  ============================================================================
 */
int main(int argc, char ** argv) {
	Char8 * dspExecutable = NULL;
	Char8 * strBufferSize = NULL;
	Char8 * strTotalBytes = NULL;
	Char8 * strProcessorId = NULL;
	Uint8 processorId = 0;

	if ((argc != 3) && (argc != 2)) {
		printf("Usage : %s <absolute path of DSP executable> "
			"<DSP Processor Id>\n"
			"For DSP Processor Id,"
			"\n\t use value of 0  if sample needs to be run on DSP 0 "
			"\n\t use value of 1  if sample needs to be run on DSP 1"
			"\n\t For single DSP configuration this is optional argument\n",
				argv[0]);
	} else {
		dspExecutable = argv[1];
		strBufferSize = "2048";
		strTotalBytes = "128";

		if (argc == 2) {
			strProcessorId = "0";
			processorId = 0;
		} else if (argc == 3) {
			strProcessorId = argv[2];
			processorId = atoi(argv[2]);
		}

		if (processorId < MAX_PROCESSORS) {

			RING_IO_Main(dspExecutable, strBufferSize, strTotalBytes,
					strProcessorId);
		}

	}
	return (0);
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
