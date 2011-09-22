/** ============================================================================
 *  @file   ring_io.c
 *
 *  @path   $(DSPLINK)/gpp/src/samples/ring_io/
 *
 *  @desc   This  application  exchanges data using ring buffers
 *          shared between the GPP and the DSP.
 *          The contents of received buffers are verified against the data
 *          sent to DSP.
 *          There are two RingIO objects in the system. One is created by the
 *          GPP and one by the DSP. The first one is opened by the GPP in writer
 *          mode, and the second in reader mode.
 *          The GPP writes data into the RINGIO1 and reads the scaled data from
 *          RINGIO2. The data in RINGIO1 is processed by a scaling factor on
 *          the DSP-side.
 *          A variable attribute (contains  scale factor and operation (Multiply
 *          or division) sent to the DSP-side attached to the RINGIO1 data
 *          buffer, and is used for all the data buffers acquired till
 *          the next available attribute changes the scaling factor and scale
 *          operation.
 *          The scaling factor used for the  buffers in RINGIO2 written
 *          by the DSP is also associated with these buffers through a variable
 *          size attribute set by the DSP at the start of the buffer,
 *          whose contents are processed with this value.
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

/*  ------------------------ DSP/BIOS Link ----------------------------------*/
#include <dsplink.h>

/*  ------------------------DSP/BIOS LINK API -------------------------------*/
#include <proc.h>
#include <mpcs.h>
#include <pool.h>
#include <ringio.h>
#include <stdio.h>
/*  ------------------------ Application Header------------------------------*/
#include <ring_io.h>
#include <ring_io_os.h>

#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */

/*  ============================================================================
 *  @const   NUM_ARGS
 *
 *  @desc   Number of arguments specified to the DSP application.
 *  ============================================================================
 */
#define NUM_ARGS              4u

/** ============================================================================
 *  @name   SAMPLE_POOL_ID
 *
 *  @desc   ID of the POOL used for the sample.
 *  ============================================================================
 */
#define SAMPLE_POOL_ID        0u

/** ============================================================================
 *  @name   NUM_BUF_SIZES
 *
 *  @desc   Number of buffer pools to be configured for the allocator.
 *  ============================================================================
 */
#define NUM_BUF_SIZES         7u

/** ============================================================================
 *  @const  NUM_BUF_POOL0
 *
 *  @desc   Number of buffers in first buffer pool.
 *  ============================================================================
 */
#define NUM_BUF_POOL0         1u

/** ============================================================================
 *  @const  NUM_BUF_POOL1
 *
 *  @desc   Number of buffers in second buffer pool.
 *  ============================================================================
 */
#define NUM_BUF_POOL1         1u

/** ============================================================================
 *  @const  NUM_BUF_POOL2
 *
 *  @desc   Number of buffers in third buffer pool.
 *  ============================================================================
 */
#define NUM_BUF_POOL2         1u

/** ============================================================================
 *  @const  NUM_BUF_POOL3
 *
 *  @desc   Number of buffers in fourth buffer pool.
 *  ============================================================================
 */
#define NUM_BUF_POOL3         1u

/** ============================================================================
 *  @const  NUM_BUF_POOL4
 *
 *  @desc   Number of buffers in fifth buffer pool.
 *  ============================================================================
 */
#define NUM_BUF_POOL4           4u

/** ============================================================================
 *  @const  NUM_BUF_POOL5
 *
 *  @desc   Number of buffers in fifth buffer pool.
 *  ============================================================================
 */
#define NUM_BUF_POOL5           4u

/** ============================================================================
 *  @const  NUM_BUF_POOL6
 *
 *  @desc   Number of buffers in fifth buffer pool.
 *  ============================================================================
 */
#define NUM_BUF_POOL6           4u

/** ============================================================================
 *  @name   RING_IO_ATTR_BUF_SIZE
 *
 *  @desc   Size of the RingIO Attribute buffer (in bytes).
 *  ============================================================================
 */
#define RING_IO_ATTR_BUF_SIZE   2048u

/*  ============================================================================
 *  @name   XFER_VALUE
 *
 *  @desc   The value used to initialize the output buffer and used for
 *          validation against the input buffer received.
 *  ============================================================================
 */
#define XFER_VALUE              5u

/*  ============================================================================
 *  @name   OP_FACTOR
 *
 *  @desc   The value used by dsp to perform mulification and division on
 *          received data.
 *  ============================================================================
 */
#define OP_FACTOR               2u

/*  ============================================================================
 *  @name   OP_MULTIPLY
 *
 *  @desc   Macro to indicate DSP that multiplication  needs to be performed on
 *          received data with OP_FACTOR.
 *
 *  ============================================================================
 */
#define OP_MULTIPLY             1u

/*  ============================================================================
 *  @name   OP_DIVIDE
 *
 *  @desc   Macro to indicates division  needs to be performed on the
 *          received data with OP_FACTOR by DSP.
 *  ============================================================================
 */
#define OP_DIVIDE               2u

/*  ============================================================================
 *  @const   RINGIO_DATA_START
 *
 *  @desc    Fixed attribute type indicates  start of the data in the RingIO
 *  ============================================================================
 */
#define RINGIO_DATA_START       1u

/*  ============================================================================
 *  @const   NOTIFY_DATA_START
 *
 *  @desc    Notification message  to  DSP.Indicates data transfer start
 *  ============================================================================
 */
#define NOTIFY_DATA_START       2u

/*  ============================================================================
 *  @const   RINGIO_DATA_END
 *
 *  @desc    Fixed attribute type indicates  start of the data in the RingIO
 *  ============================================================================
 */
#define RINGIO_DATA_END         3u

/*  ============================================================================
 *  @const   NOTIFY_DATA_START
 *
 *  @desc     Notification message  to  DSP.Indicates data transfer stop.
 *  ============================================================================
 */
#define NOTIFY_DATA_END         4u



/*  ============================================================================
 *  @const   RINGIO_DSP_END
 *
 *  @desc     Fixed attribute type indicates  end of the dsp 
 *  ============================================================================
 */
#define RINGIO_DSP_END         5u


/*  ============================================================================
 *  @const   NOTIFY_DSP_END
 *
 *  @desc     Notification message  to  DSP.Indicates DSP end
 *  ============================================================================
 */
#define NOTIFY_DSP_END         6u





/*  ============================================================================
 *  @const   RING_IO_WRITER_BUF_SIZE
 *
 *  @desc    Writer task buffer acquire size.
 *  ============================================================================
 */
#define RING_IO_WRITER_BUF_SIZE    1024u

/** ============================================================================
 *  @const  RING_IO_VATTR_SIZE
 *
 *  @desc   size of the RingIO varibale attribute buffer.
 *  ============================================================================
 */
#define RING_IO_VATTR_SIZE      1u

/*  ============================================================================
 *  @name   RING_IO_BufferSize
 *
 *  @desc   Size of the RingIO Data Buffer to be used for data transfer.
 *  ============================================================================
 */
STATIC Uint32 RING_IO_BufferSize;
STATIC Uint32 RING_IO_BufferSize1;
STATIC Uint32 RING_IO_BufferSize2;
STATIC Uint32 RING_IO_BufferSize3;

/** ============================================================================
 *  @const  RingIOWriterName
 *
 *  @desc   Name of the RingIO used by the application in writer mode.
 *  ============================================================================
 */
STATIC Char8 RingIOWriterName1[RINGIO_NAME_MAX_LEN] = "RINGIO1";

STATIC Char8 RingIOWriterName2[RINGIO_NAME_MAX_LEN] = "RINGIO3";

/** ============================================================================
 *  @name   RingIOWriterHandle
 *
 *  @desc   Handle to the RingIO used by the application in writer mode.
 *  ============================================================================
 */
STATIC RingIO_Handle RingIOWriterHandle1 = NULL;
STATIC RingIO_Handle RingIOWriterHandle2 = NULL;

/** ============================================================================
 *  @const  RingIOReaderName
 *
 *  @desc   Name of the RingIO used by the application in reader mode.
 *  ============================================================================
 */
STATIC Char8 RingIOReaderName1[RINGIO_NAME_MAX_LEN] = "RINGIO2";
STATIC Char8 RingIOReaderName2[RINGIO_NAME_MAX_LEN] = "RINGIO4";

/** ============================================================================
 *  @name   RingIOReaderHandle
 *
 *  @desc   Handle to the RingIO used by the application in reader mode.
 *  ============================================================================
 */
STATIC RingIO_Handle RingIOReaderHandle1 = NULL;
STATIC RingIO_Handle RingIOReaderHandle2 = NULL;

/** ============================================================================
 *  @const  RingIOReaderVAttrBuf
 *
 *  @desc   varible that specifies the totol number of bytes to transfer.
 *  ============================================================================
 */
STATIC Uint32 RING_IO_BytesToTransfer1;
STATIC Uint32 RING_IO_BytesToTransfer2;

/** ============================================================================
 *  @const  writerClientInfo
 *
 *  @desc   Writer task information structure.
 *  ============================================================================
 */
RING_IO_ClientInfo writerClientInfo1;
RING_IO_ClientInfo writerClientInfo2;

/** ============================================================================
 *  @const  readerClientInfo
 *
 *  @desc   Reader task information structure.
 *  ============================================================================
 */
RING_IO_ClientInfo readerClientInfo1;
RING_IO_ClientInfo readerClientInfo2;

/** ============================================================================
 *  @const  fReaderStart
 *
 *  @desc   boolean variable to  indicate Gpp reader can start reading.
 *  ============================================================================
 */
STATIC Uint32 fReaderStart1 = FALSE;
STATIC Uint32 fReaderStart2 = FALSE;

/** ============================================================================
 *  @const  fReaderEnd
 *
 *  @desc   boolean variable to  indicate Gpp reader should stop reading.
 *  ============================================================================
 */
STATIC Uint32 fReaderEnd1 = FALSE;
STATIC Uint32 fReaderEnd2 = FALSE;

/** ----------------------------------------------------------------------------
 *  @func   RING_IO_VerifyData
 *
 *  @desc   This function verifies the data-integrity of given message.
 *
 *  @arg    buffer
 *              Pointer to the buffer whose contents are to be validated.
 *  @arg    factor
 *              Expected scaling factor for the buffer contents.
 *  @arg    opcode
 *              Operation(division or multiplication that is performed on
 *              received data from DSP.
 *  @arg    size
 *              Size of the buffer.
 *
 *  @ret    DSP_SOK
 *              Operation successfully completed.
 *          DSP_EFAIL
 *              Contents of the buffer are unexpected.
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ----------------------------------------------------------------------------
 */
STATIC
NORMAL_API
DSP_STATUS
RING_IO_Reader_VerifyData (IN Void * buffer,
		//IN Uint32 factor,
		//IN Uint32 opcode,
		IN Uint32 size);

/** ----------------------------------------------------------------------------
 *  @func   RING_IO_InitBuffer
 *
 *  @desc   This function initializes the specified buffer with valid data.
 *
 *  @arg    buffer
 *              Pointer to the buffer whose contents are to be initialized.
 *  @arg    size
 *              size of the buffer.
 *
 *  @ret    None
 *
 *  @enter  None
 *
 *  @leave  None
 *
 *  @see    None
 *  ----------------------------------------------------------------------------
 */
STATIC
NORMAL_API
Void
RING_IO_InitBuffer (IN Void * buffer, Uint32 size);

/** ----------------------------------------------------------------------------
 *  @func   RING_IO_Writer_Notify
 *
 *  @desc   This function implements the notification callback for the RingIO
 *          opened by the GPP in writer  mode.
 *
 *  @arg    handle
 *              Handle to the RingIO.
 *  @arg    param
 *              Parameter used while registering the notification.
 *  @arg    msg
 *               Message passed along with notification.
 *  @modif  None
 *  ----------------------------------------------------------------------------
 */
STATIC
NORMAL_API
Void
RING_IO_Writer_Notify1 (IN RingIO_Handle handle,
		IN RingIO_NotifyParam param,
		IN RingIO_NotifyMsg msg);

STATIC
NORMAL_API
Void
RING_IO_Writer_Notify2 (IN RingIO_Handle handle,
		IN RingIO_NotifyParam param,
		IN RingIO_NotifyMsg msg);
/** ----------------------------------------------------------------------------
 *  @func   RING_IO_Reader_Notify
 *
 *  @desc   This function implements the notification callback for the RingIO
 *          opened by the GPP in writer  mode.
 *
 *  @arg    handle
 *              Handle to the RingIO.
 *  @arg    param
 *              Parameter used while registering the notification.
 *  @arg    msg
 *               Message passed along with notification.
 *  @modif  None
 *  ----------------------------------------------------------------------------
 */
STATIC
NORMAL_API
Void
RING_IO_Reader_Notify1 ( IN RingIO_Handle handle,
		IN RingIO_NotifyParam param,
		IN RingIO_NotifyMsg msg);

STATIC
NORMAL_API
Void
RING_IO_Reader_Notify2 ( IN RingIO_Handle handle,
		IN RingIO_NotifyParam param,
		IN RingIO_NotifyMsg msg);
/** ============================================================================
 *  @func   RING_IO_Create
 *
 *  @desc   This function allocates and initializes resources used by
 *          this application.
 *
 *  @modif  RING_IO_InpBufs , RING_IO_OutBufs
 *  ============================================================================
 */
NORMAL_API
DSP_STATUS
RING_IO_Create (IN Char8 * dspExecutable,
		// IN Char8 * strBufferSize,
		// IN Char8 * strTotalBytes,
		IN Uint8 processorId)
{

	DSP_STATUS status = DSP_SOK;
	Uint32 numArgs = NUM_ARGS;
	Uint32 numBufs [NUM_BUF_SIZES] = {NUM_BUF_POOL0,
		NUM_BUF_POOL1,
		NUM_BUF_POOL2,
		NUM_BUF_POOL3,
		NUM_BUF_POOL4,
		NUM_BUF_POOL5,
		NUM_BUF_POOL6
	};
	Uint32 size [NUM_BUF_SIZES];
	SMAPOOL_Attrs poolAttrs;
	Char8 * args [NUM_ARGS];
	Char8 tempCmdString [NUM_ARGS][11];
	RingIO_Attrs ringIoAttrs;

	//Send and Receive RING data buffer size
	RING_IO_BufferSize = 1024;
	RING_IO_BufferSize1 = 4096;
	RING_IO_BufferSize2 = 2048;
	RING_IO_BufferSize3 = 2048;

	RING_IO_BytesToTransfer1 = 1024;
	RING_IO_BytesToTransfer2 = 2048;

	RING_IO_BufferSize = DSPLINK_ALIGN (RING_IO_BufferSize,
			DSPLINK_BUF_ALIGN);
	RING_IO_BufferSize1 = DSPLINK_ALIGN (RING_IO_BufferSize1,
			DSPLINK_BUF_ALIGN);
	RING_IO_BufferSize2 = DSPLINK_ALIGN (RING_IO_BufferSize2,
			DSPLINK_BUF_ALIGN);
	RING_IO_BufferSize3 = DSPLINK_ALIGN (RING_IO_BufferSize3,
			DSPLINK_BUF_ALIGN);
	RING_IO_BytesToTransfer1 = DSPLINK_ALIGN (RING_IO_BytesToTransfer1,
			DSPLINK_BUF_ALIGN);
	RING_IO_BytesToTransfer2 = DSPLINK_ALIGN (RING_IO_BytesToTransfer2,
			DSPLINK_BUF_ALIGN);

	RING_IO_0Print ("Entered RING_IO_Create ()\n");
	/*
	 *  OS initialization
	 */
	status = RING_IO_OS_init ();
	if (status != DSP_SOK) {
		RING_IO_1Print ("RING_IO_OS_init () failed. Status = [0x%x]\n",
				status);
	}
	/*
	 *  Create and initialize the proc object.
	 */
	status = PROC_setup (NULL);

	/*
	 *  Attach the Dsp with which the transfers have to be done.
	 */
	if (DSP_SUCCEEDED (status)) {
		status = PROC_attach (processorId, NULL);
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("PROC_attach () failed. Status = [0x%x]\n",
					status);
		}
	}
	else {
		RING_IO_1Print ("PROC_setup () failed. Status = [0x%x]\n", status);
	}

	/*
	 *  Open the pool.
	 */
	if (DSP_SUCCEEDED (status)) {

		size [0] = RING_IO_BufferSize;
		size [1] = RING_IO_BufferSize1;
		size [2] = RING_IO_BufferSize2;
		size [3] = RING_IO_BufferSize3;

		size [4] = RING_IO_ATTR_BUF_SIZE;
		size [5] = sizeof (RingIO_ControlStruct);
		size [6] = sizeof (MPCS_ShObj);
		poolAttrs.bufSizes = (Uint32 *) &size;
		poolAttrs.numBuffers = (Uint32 *) &numBufs;
		poolAttrs.numBufPools = NUM_BUF_SIZES;
		poolAttrs.exactMatchReq = TRUE;
		status = POOL_open (POOL_makePoolId(processorId, SAMPLE_POOL_ID), &poolAttrs);
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("POOL_open () failed. Status = [0x%x]\n",
					status);
		}
	}

	/*
	 *  Load the executable on the DSP.
	 */
	if (DSP_SUCCEEDED (status)) {

		/* RingIO send data buffer size*/
		RING_IO_IntToString (size [1], tempCmdString [0]);
		args [0] = tempCmdString [0];

		/* RingIO recv data buffer size*/
		RING_IO_IntToString (size [3], tempCmdString [1]);
		args [1] = tempCmdString [1];

		/* RingIO attr buffer size */
		RING_IO_IntToString (RING_IO_ATTR_BUF_SIZE, tempCmdString [2]);
		args [2] = tempCmdString [2];
		/* RingIO foot buffer size */
		RING_IO_IntToString (0, tempCmdString [3]);
		args [3] = tempCmdString [3];

		{
			status = PROC_load (processorId, dspExecutable, numArgs, args);
		}

		if (DSP_FAILED (status)) {
			RING_IO_1Print ("PROC_load () failed. Status = [0x%x]\n", status);
		}
	}

	//Create the write RingIO for sending 
	if (DSP_SUCCEEDED (status)) {
		ringIoAttrs.transportType = RINGIO_TRANSPORT_GPP_DSP;
		ringIoAttrs.ctrlPoolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
		ringIoAttrs.dataPoolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
		ringIoAttrs.attrPoolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
		ringIoAttrs.lockPoolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
		ringIoAttrs.dataBufSize = size [0];
		ringIoAttrs.footBufSize = 0;
		ringIoAttrs.attrBufSize = RING_IO_ATTR_BUF_SIZE;
#if defined (DSPLINK_LEGACY_SUPPORT)
		status = RingIO_create (RingIOWriterName1, &ringIoAttrs);
#else
		status = RingIO_create (processorId, RingIOWriterName1, &ringIoAttrs);
#endif /* if defined (DSPLINK_LEGACY_SUPPORT) */
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("RingIO_create () failed. Status = [0x%x]\n",
					status);
		}
	}

	//Create the write RingIO for receiving 
	if (DSP_SUCCEEDED (status)) {
		ringIoAttrs.transportType = RINGIO_TRANSPORT_GPP_DSP;
		ringIoAttrs.ctrlPoolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
		ringIoAttrs.dataPoolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
		ringIoAttrs.attrPoolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
		ringIoAttrs.lockPoolId = POOL_makePoolId(processorId, SAMPLE_POOL_ID);
		ringIoAttrs.dataBufSize = size [2];
		ringIoAttrs.footBufSize = 0;
		ringIoAttrs.attrBufSize = RING_IO_ATTR_BUF_SIZE;
#if defined (DSPLINK_LEGACY_SUPPORT)
		status = RingIO_create (RingIOWriterName2, &ringIoAttrs);
#else
		status = RingIO_create (processorId, RingIOWriterName2, &ringIoAttrs);
#endif /* if defined (DSPLINK_LEGACY_SUPPORT) */
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("RingIO_create () failed. Status = [0x%x]\n",
					status);
		}
	}

	/*
	 *  Start execution on DSP.
	 */
	if (DSP_SUCCEEDED (status)) {
		status = PROC_start (processorId);
		if (DSP_FAILED (status)) {
			RING_IO_1Print (" PROC_start () failed. Status = [0x%x]\n",
					status);
		}
	}
	RING_IO_0Print ("Leaving RING_IO_Create ()\n");

	return (status);
}

/** ============================================================================
 *  @func   RING_IO_WriterTsk
 *
 *  @desc   This function implements the writer task  for this sample
 *          application.
 *          The  writer task has the following flow:
 *          1.  This task (GPP RingIO writer) sets the notifier for the RINGIO1
 *              writer with the specific  watermark  value of the buffer size
 *              used for data transfer. The notifier function post the semaphore
 *              passed to it, resulting in unblocking the application which
 *              would be waiting on it.
 *
 *          2.  It inserts an attribute(RINGIO_DATA_START)  in to RINGIO1 to
 *              indicate the start of the data transfer.
 *
 *          3.  It sends  a force notification to unblock RINGIO1 reader(DSP)
 *              and to allow it to  read data from the RingIO.
 *
 *          4.  It sets a variable attribute before acquiring any buffer.
 *              This variable attribute payload contains size, action, factor
 *              fields.
 *              - Size is the size of the received data (in bytes)
 *                that needs to be considered by DSP for processing based on the
 *                action and the factor fields.
 *              - Action tells the DSP what action needs to be taken on the
 *                received data (i.e. multiply or division).
 *              - Factor   holds the other operand used in processing the
 *                received data by DSP.
 *
 *          5.  It acquires and initializes the RINIGIO1 buffer .Then it
 *              releases the buffer.
 *
 *          6.  If buffer is not available,  the application waits on a
 *              semaphore, which will be posted by the notification function
 *              registered for RINIGIO1 writer with watermark equal to the
 *              buffer size.
 *
 *          7.  Steps 4 to 6 are repeated for the number of bytes specified.
 *
 *          8.  After finishing the data transfer, it inserts an attribute
 *              (RINGIO_DATA_END) indicating end of data transmission from GPP.
 *              It also sends a force notification to the RINGIO1 reader (DSP).
 *              This force notification allows DSP to come out of blocked state,
 *              if it is waiting for data notification.
 *              (Because finally we are sending only the attribute and not data).
 *
 *          9.  Finally it   deletes the created semaphore and exits.
 *
 *  @modif  None
 *  ============================================================================
 */
STATIC Uint32 Task_Run = TRUE;

NORMAL_API
Void *
RING_IO_WriterClient1 (IN Void * ptr)
{
	char c;

	DSP_STATUS status = DSP_SOK;
	DSP_STATUS relStatus = DSP_SOK;
	DSP_STATUS tmpStatus = DSP_SOK;
	RingIO_BufPtr bufPtr = NULL;
	Pvoid semPtrWriter = NULL;
	Uint8 i = 0;
	Uint32 bytesTransfered = 0;
	Uint32 attrs [RING_IO_VATTR_SIZE];
	Uint16 type;
	Uint32 acqSize;

	Pvoid semPtrReader = NULL;
	Uint32 param;
	Uint32 vAttrSize = 0;
	Uint32 rcvSize = RING_IO_BufferSize1;
	Uint32 totalRcvbytes = 0;
	Uint8 exitFlag = FALSE;
	DSP_STATUS attrStatus = DSP_SOK;

	////////////////////////////////////////////////////////////////////////////////
	// initial the write task
	////////////////////////////////////////////////////////////////////////////////

	RING_IO_0Print ("Entered RING_IO_WriterClient1 ()\n");

	/*
	 *  Open the RingIO to be used with GPP as the writer.
	 *
	 *  Value of the flags indicates:
	 *     No cache coherence for: Control structure
	 *                             Data buffer
	 *                             Attribute buffer
	 *     Exact size requirement.
	 */
	RingIOWriterHandle1 = RingIO_open (RingIOWriterName1,
			RINGIO_MODE_WRITE,
			(Uint32) (RINGIO_NEED_EXACT_SIZE));
	if (RingIOWriterHandle1 == NULL) {
		status = RINGIO_EFAILURE;
		RING_IO_1Print ("RingIO_open1 () Writer failed. Status = [0x%x]\n",
				status);
	}
	//RING_IO_0Print ("RingIO_open1 () Writer  \n");

	if (DSP_SUCCEEDED (status)) {
		/* Create the semaphore to be used for notification */
		status = RING_IO_CreateSem (&semPtrWriter);
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("RING_IO_CreateSem1 () Writer SEM failed "
					"Status = [0x%x]\n",
					status);
		}
	}

	//RING_IO_0Print ("RING_IO_CreateSem1 () Writer SEM   \n");

	if (DSP_SUCCEEDED (status)) {
		/*
		 *  Set the notification for Writer.
		 */
		do {
			/* Set the notifier for writer for RingIO created by the GPP. */
			status = RingIO_setNotifier (RingIOWriterHandle1,
					RINGIO_NOTIFICATION_ONCE,
					//RING_IO_WRITER_BUF_SIZE,
					RING_IO_BytesToTransfer1,
					&RING_IO_Writer_Notify1,
					(RingIO_NotifyParam) semPtrWriter);
			if (status != RINGIO_SUCCESS) {
				RING_IO_Sleep(10);
			}
		}while (DSP_FAILED (status));

	}

	//RING_IO_0Print (" RingIO_setNotifier1 () Writer SEM   \n");
	////////////////////////////////////////////////////////////////////////////////
	//end  initial the write task
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// initial the read  task
	////////////////////////////////////////////////////////////////////////////////
	RING_IO_0Print ("Entered RING_IO_ReaderClient1 ()\n");

	/*
	 *  Open the RingIO to be used with GPP as the reader.
	 *  Value of the flags indicates:
	 *     No cache coherence for: Control structure
	 *                             Data buffer
	 *                             Attribute buffer
	 *     Exact size requirement false.
	 */
	do {
		RingIOReaderHandle1 = RingIO_open (RingIOReaderName1,
				RINGIO_MODE_READ,
				0);

		//	RING_IO_0Print (" RingIO_open (RingIOReaderName1()ing \n") ;

	}while (RingIOReaderHandle1 == NULL);

//	RING_IO_0Print (" RingIO_open (RingIOReaderName1  ()\n");

	/* Create the semaphore to be used for notification */
	status = RING_IO_CreateSem (&semPtrReader);
	if (DSP_FAILED (status)) {
		RING_IO_1Print ("RING_IO_CreateSem1 () Reader SEM failed "
				"Status = [0x%x]\n",
				status);
	}

	//RING_IO_0Print (" RING_IO_CreateSem1 () Reader SEM  \n");

	if (DSP_SUCCEEDED(status)) {
		do {

			/* Set the notifier for reader for RingIO created by the DSP. */
			/*
			 * Set water mark to zero. and try to acquire the full buffer
			 * and  read what ever is available.
			 */
			status = RingIO_setNotifier (RingIOReaderHandle1,
					RINGIO_NOTIFICATION_ONCE,
					0,
					&RING_IO_Reader_Notify1,
					(RingIO_NotifyParam) semPtrReader);
			if (DSP_FAILED (status)) {
				/*RingIO_setNotifier () Reader failed  */
				RING_IO_Sleep(10);
			}
		}while (DSP_FAILED (status));
	}

	//RING_IO_0Print (" RingIO_setNotifier1 Reader SEM  \n");
	RING_IO_0Print ("End initial the read  task1 \n");

	////////////////////////////////////////////////////////////////////////////////
	//end initial the read  task
	////////////////////////////////////////////////////////////////////////////////


	while(1) {
		RING_IO_0Print ("Enter text. Include a dot ('.') in a sentence to exit: \n");
		c = getchar();
		if(c == '.') {
			Task_Run = FALSE;
			break;
		}

		////////////////////////////////////////////////////////////////////////////////
		//the execute of write task
		////////////////////////////////////////////////////////////////////////////////

		if (DSP_SUCCEEDED (status)) {
			/* Send data transfer attribute (Fixed attribute) to DSP*/
			type = (Uint16) RINGIO_DATA_START;
			status = RingIO_setAttribute(RingIOWriterHandle1,
					0,
					type,
					0);
			if (DSP_FAILED(status)) {
				RING_IO_1Print ("RingIO_setAttribute1 failed to set the  "
						"RINGIO_DATA_START. Status = [0x%x]\n",
						status);
			}
		}

		RING_IO_0Print ("RingIO_setAttribute1 () Writer SEM   \n");

		if (DSP_SUCCEEDED (status)) {
			/* Send Notification  to  the reader (DSP)*/
			RING_IO_0Print ("GPP-->DSP1:Sent Data Transfer Start "
					"Attribute\n");
			do {
				status = RingIO_sendNotify (RingIOWriterHandle1,
						(RingIO_NotifyMsg)NOTIFY_DATA_START);
				if (DSP_FAILED(status)) {
					/* RingIO_sendNotify failed to send notification */
					RING_IO_Sleep(10);
				}
				else {
					RING_IO_0Print ("GPP-->DSP1:Sent Data Transfer Start "
							"Notification \n");
				}
			}while (status != RINGIO_SUCCESS);
		}

		if (DSP_SUCCEEDED (status)) {

			RING_IO_1Print ("Bytes to transfer :%ld \n", RING_IO_BytesToTransfer1);
			RING_IO_1Print ("Data buffer size  :%ld \n", RING_IO_BufferSize);

			while ( (RING_IO_BytesToTransfer1 == 0)
					|| (bytesTransfered < RING_IO_BytesToTransfer1)) {

				/* Update the attrs to send variable attribute to DSP*/
				//attrs [0] = RING_IO_WRITER_BUF_SIZE;
				attrs [0] = RING_IO_BytesToTransfer1;

				/* ----------------------------------------------------------------
				 * Send to DSP.
				 * ----------------------------------------------------------------
				 */
				/* Set the scaling factor variable attribute*/
				status = RingIO_setvAttribute (RingIOWriterHandle1,
						0, /* at the beginning */
						0, /* No type */
						0,
						attrs,
						sizeof (attrs));
				if (DSP_FAILED(status)) {
					/* RingIO_setvAttribute failed */
					RING_IO_Sleep(10);
				}
				else {
					/* Acquire writer bufs and initialize and release them. */
					//acqSize = RING_IO_WRITER_BUF_SIZE;
					acqSize = RING_IO_BytesToTransfer1;
					status = RingIO_acquire (RingIOWriterHandle1,
							&bufPtr ,
							&acqSize);

					/* If acquire success . Write to  ring bufer and then release
					 * the acquired.
					 */
					if ((DSP_SUCCEEDED (status)) && (acqSize > 0)) {
						RING_IO_InitBuffer (bufPtr, acqSize);

						//debug
						Uint8 *ptr8 = (Uint8 *)(bufPtr);
						for (i = 0;i < 5; i++) {
								RING_IO_1Print ("    Send [0x%x]  ", *(ptr8+i));
							}
						RING_IO_0Print ("\n");




						

						if ( (RING_IO_BytesToTransfer1 != 0)
								&& ( (bytesTransfered + acqSize)
										> RING_IO_BytesToTransfer1)) {

							/* we have acquired more buffer than the rest of data
							 * bytes to be transferred */
							if (bytesTransfered != RING_IO_BytesToTransfer1) {

								relStatus = RingIO_release (RingIOWriterHandle1,
										(RING_IO_BytesToTransfer1-
												bytesTransfered));
								if (DSP_FAILED (relStatus)) {
									RING_IO_1Print ("RingIO_release1 () in Writer "
											"task failed relStatus = [0x%x]"
											"\n" , relStatus);
								}
							}

							/* Cancel the  rest of the buffer */
							status = RingIO_cancel (RingIOWriterHandle1);
							if (DSP_FAILED(status)) {
								RING_IO_1Print ("RingIO_cancel1 () in Writer"
										"task failed "
										"status = [0x%x]\n",
										status);
							}
							bytesTransfered = RING_IO_BytesToTransfer1;

						}
						else {

							relStatus = RingIO_release (RingIOWriterHandle1,
									acqSize);
							if (DSP_FAILED (relStatus)) {
								RING_IO_1Print ("RingIO_release1 () in Writer task "
										"failed. relStatus = [0x%x]\n",
										relStatus);
							}
							else {
								bytesTransfered += acqSize;
							}
						}

						/*if ((bytesTransfered % (RING_IO_WRITER_BUF_SIZE * 8u)) == 0)
						 {
						 RING_IO_1Print ("GPP-->DSP1:Bytes Transferred: %lu \n",
						 bytesTransfered);
						 }*/
					}
					else {

						/* Acquired failed, Wait for empty buffer to become
						 * available.
						 */
						status = RING_IO_WaitSem (semPtrWriter);
						if (DSP_FAILED (status)) {
							RING_IO_1Print ("RING_IO_WaitSem1 () Writer SEM failed "
									"Status = [0x%x]\n",
									status);
						}
					}
				}
			}

			RING_IO_1Print ("GPP-->DSP1:Total Bytes Transmitted  %ld \n",
					bytesTransfered);

			bytesTransfered = 0;

			/* Send  End of  data transfer attribute to DSP */
			type = (Uint16) RINGIO_DATA_END;

			do {
				status = RingIO_setAttribute (RingIOWriterHandle1,
						0,
						type,
						0);
				if (DSP_SUCCEEDED(status)) {
					RING_IO_1Print ("RingIO_setAttribute1 succeeded to set the  "
							"RINGIO_DATA_END. Status = [0x%x]\n",
							status);
				}
				//else {
				//	RING_IO_YieldClient ();
				//}
			}while (status != RINGIO_SUCCESS);

			RING_IO_0Print ("GPP-->DSP1:Sent Data Transfer End Attribute\n");

			if (DSP_SUCCEEDED (status)) {

				/* Send Notification  to  the reader (DSP)
				 * This allows DSP  application to come out from blocked state  if
				 * it is waiting for Data buffer and  GPP sent only data end
				 * attribute.
				 */
				status = RingIO_sendNotify (RingIOWriterHandle1,
						(RingIO_NotifyMsg)NOTIFY_DATA_END);
				if (DSP_FAILED(status)) {
					RING_IO_1Print ("RingIO_sendNotify1 failed to send notification "
							"NOTIFY_DATA_END. Status = [0x%x]\n",
							status);
				}
				else {
					RING_IO_0Print ("GPP-->DSP1:Sent Data Transfer End Notification"
							" \n");
					RING_IO_YieldClient ();
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		//end the execute of write task
		////////////////////////////////////////////////////////////////////////////////


		////////////////////////////////////////////////////////////////////////////////
		// the execute of read task
		////////////////////////////////////////////////////////////////////////////////

		if (DSP_SUCCEEDED(status)) {

			/*
			 * Wait for notification from  DSP  about data
			 * transfer
			 */
			status = RING_IO_WaitSem (semPtrReader);
			if (DSP_FAILED (status)) {
				RING_IO_1Print ("RING_IO_WaitSem1 () Reader SEM failed "
						"Status = [0x%x]\n",
						status);
			}
			RING_IO_0Print (" RING_IO_WaitSem1 () Reader SEM  \n");

			if (fReaderStart1 == TRUE) {

				fReaderStart1 = FALSE;

				/* Got  data transfer start notification from DSP*/
				do {

					/* Get the start attribute from dsp */
					status = RingIO_getAttribute (RingIOReaderHandle1,
							&type,
							&param);
					if ( (status == RINGIO_SUCCESS)
							|| (status == RINGIO_SPENDINGATTRIBUTE)) {

						if (type == (Uint16)RINGIO_DATA_START) {
							RING_IO_0Print ("GPP<--DSP1:Received Data Transfer"
									"Start Attribute\n");
							break;
						}
						else {
							RING_IO_1Print ("RingIO_getAttribute1 () Reader failed "
									"Unknown attribute received instead of "
									"RINGIO_DATA_START. Status = [0x%x]\n",
									status);
							RING_IO_Sleep(10);
						}
					}
					else {
						RING_IO_Sleep(10);
					}

				}while ( (status != RINGIO_SUCCESS)
						&& (status != RINGIO_SPENDINGATTRIBUTE));
			}

			/* Now reader  can start reading data from the ringio created
			 * by Dsp as the writer
			 */
			acqSize = RING_IO_BufferSize1;
			while (exitFlag == FALSE) {

				status = RingIO_acquire (RingIOReaderHandle1,
						&bufPtr ,
						&acqSize);

				if ((status == RINGIO_SUCCESS)
						||(acqSize > 0)) {
					/* Got buffer from DSP.*/
					rcvSize -= acqSize;
					totalRcvbytes += acqSize;

					/* Verify the received data */
					if (DSP_SOK != RING_IO_Reader_VerifyData (bufPtr,
									//factor,
									//action,
									acqSize)) {
						RING_IO_1Print (" Data1 verification failed after"
								"%ld bytes received from DSP \n",
								totalRcvbytes);
					}

					/* Release the acquired buffer */
					relStatus = RingIO_release (RingIOReaderHandle1,
							acqSize);
					if (DSP_FAILED (relStatus)) {
						RING_IO_1Print ("RingIO_release1 () in Writer task"
								"failed relStatus = [0x%x]\n",
								relStatus);
					}

					/* Set the acqSize for the next acquire */
					if (rcvSize == 0) {
						/* Reset  the rcvSize to  size of the full buffer  */
						rcvSize = RING_IO_BufferSize1;
						acqSize = RING_IO_BufferSize1;
					}
					else {
						/*Acquire the partial buffer  in next acquire */
						acqSize = rcvSize;
					}

					/*if ((totalRcvbytes % (8192u)) == 0u) {
					 RING_IO_1Print ("GPP<--DSP1:Bytes Received :%lu \n",
					 totalRcvbytes);

					 }*/

				}
				else if ( (status == RINGIO_SPENDINGATTRIBUTE)
						&& (acqSize == 0u)) {

					/* Attribute is pending,Read it */
					attrStatus = RingIO_getAttribute (RingIOReaderHandle1,
							&type,
							&param);
					if ((attrStatus == RINGIO_SUCCESS)
							|| (attrStatus == RINGIO_SPENDINGATTRIBUTE)) {

						if (type == RINGIO_DATA_END) {
							/* End of data transfer from DSP */
							RING_IO_0Print ("GPP<--DSP1:Received Data Transfer"
									"End Attribute \n");
							exitFlag = TRUE;/* Come Out of while loop */
						}
						else {
							RING_IO_1Print ("RingIO_getAttribute () Reader "
									"error,Unknown attribute "
									" received Status = [0x%x]\n",
									attrStatus);
						}
					}
					else if (attrStatus == RINGIO_EVARIABLEATTRIBUTE) {

						vAttrSize = sizeof(attrs);
						attrStatus = RingIO_getvAttribute (RingIOReaderHandle1,
								&type,
								&param,
								attrs,
								&vAttrSize);

						if ((attrStatus == RINGIO_SUCCESS)
								|| (attrStatus == RINGIO_SPENDINGATTRIBUTE)) {

							/* Success in receiving  variable attribute*/
							rcvSize = attrs[0];
							/* Set the  acquire size equal to the
							 * rcvSize
							 */
							acqSize = rcvSize;

							RING_IO_1Print ("!!#RingIO_getAttribute () Reader "
									" received size = [%d]\n",
									rcvSize);

						}
						else if (attrStatus == RINGIO_EVARIABLEATTRIBUTE) {

							RING_IO_1Print ("Error: "
									"buffer is not sufficient to receive"
									"the  variable attribute "
									"Status = [0x%x]\n",
									attrStatus);
						}
						else if (attrStatus == DSP_EINVALIDARG) {

							RING_IO_1Print ("Error: "
									"Invalid args to receive"
									"the  variable attribute "
									"Status = [0x%x]\n",
									attrStatus);

						}
						else if (attrStatus ==RINGIO_EPENDINGDATA) {

							RING_IO_1Print ("Error:RingIO_getvAttribute "
									"Status = [0x%x]\n",
									attrStatus);
						}
						else {
							/* Unable to get  variable attribute
							 * go back to read data.
							 * this may occur due to ringo flush by the DSP
							 * or may be due to  general failure
							 */
						}

					}
					else {
						RING_IO_1Print ("RingIO_getAttribute () Reader error "
								"Status = [0x%x]\n",
								attrStatus);
					}
				}
				else if ( (status == RINGIO_EFAILURE)
						||(status == RINGIO_EBUFEMPTY)) {

					/* Failed to acquire buffer */
					status = RING_IO_WaitSem (semPtrReader);
					if (DSP_FAILED (status)) {
						RING_IO_1Print ("RING_IO_WaitSem () Reader SEM failed "
								"Status = [0x%x]\n",
								status);
					}
				}
				else {
					acqSize = RING_IO_BufferSize1;

				}

				/* Reset the acquired size if it is changed to zero by the
				 * failed acquire call
				 */
				if (acqSize == 0) {
					acqSize = RING_IO_BufferSize1;
				}

			}
		}

		RING_IO_1Print ("GPP<--DSP1:Bytes Received %ld \n",
				totalRcvbytes);

		if (fReaderEnd1 != TRUE) {
			/* If data transfer end notification  not yet received
			 * from DSP ,wait for it.
			 */
			status = RING_IO_WaitSem (semPtrReader);
			if (DSP_FAILED (status)) {
				RING_IO_1Print ("RING_IO_WaitSem1 () Reader SEM failed "
						"Status = [0x%x]\n",
						status);
			}
		}
		//else {
			
		//}
		totalRcvbytes = 0;
		rcvSize = RING_IO_BufferSize1;
		fReaderEnd1 = FALSE;
		exitFlag = FALSE;
		RING_IO_0Print ("End Reader Task1  () \n");

		////////////////////////////////////////////////////////////////////////////////
		//End the execute of read task
		////////////////////////////////////////////////////////////////////////////////
	}




	do {
	tmpStatus = RingIO_sendNotify (RingIOWriterHandle1,
						(RingIO_NotifyMsg)NOTIFY_DSP_END);
	if (DSP_FAILED(tmpStatus)) {
			status = tmpStatus;
			RING_IO_0Print("RingIO_sendNotify (RingIOWriterHandle1)\n");
			RING_IO_Sleep(10);
		} else {
			status = RINGIO_SUCCESS;
		}
	} while (DSP_FAILED(tmpStatus));



	////////////////////////////////////////////////////////////////////////////////
	//close  the write  task
	////////////////////////////////////////////////////////////////////////////////

	/* Delete the semaphore used for notification */
	if (semPtrWriter != NULL) {
		tmpStatus = RING_IO_DeleteSem (semPtrWriter);
		if (DSP_SUCCEEDED (status) && DSP_FAILED (tmpStatus)) {
			status = tmpStatus;
			RING_IO_1Print ("RING_IO_DeleteSem1 () Writer SEM failed "
					"Status = [0x%x]\n",
					status);
		}
	}

	//RING_IO_0Print ("RING_IO_DeleteSem1 () Writer SEM  \n");

	/*
	 *  Close the RingIO to be used with GPP as the writer.
	 */
	if (RingIOWriterHandle1 != NULL) {
		while ( (RingIO_getValidSize(RingIOWriterHandle1) != 0)
				|| (RingIO_getValidAttrSize(RingIOWriterHandle1) != 0)) {
			RING_IO_Sleep(10);
		}
		tmpStatus = RingIO_close (RingIOWriterHandle1);
		if (DSP_FAILED (tmpStatus)) {
			RING_IO_1Print ("RingIO_close1 () Writer failed. Status = [0x%x]\n",
					status);
		}
	}

	RING_IO_0Print ("Leaving RING_IO_WriterClient1 () \n");
	////////////////////////////////////////////////////////////////////////////////
	//End close  the write  task
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	//close  the read  task
	////////////////////////////////////////////////////////////////////////////////

	if (semPtrReader != NULL) {
		tmpStatus = RING_IO_DeleteSem (semPtrReader);
		if (DSP_SUCCEEDED (status) && DSP_FAILED (tmpStatus)) {
			status = tmpStatus;
			RING_IO_1Print ("RING_IO_DeleteSem1 () Reader SEM failed "
					"Status = [0x%x]\n",
					status);
		}
	}
	//RING_IO_0Print (" RING_IO_DeleteSem1 () Reader SEM   \n");

	/*
	 *  Close the RingIO to be used with GPP as the reader.
	 */
	if (RingIOReaderHandle1 != NULL) {
		tmpStatus = RingIO_close (RingIOReaderHandle1);
		if (DSP_FAILED (tmpStatus)) {
			RING_IO_1Print ("RingIO_close1 () Reader failed. Status = [0x%x]\n",
					status);
		}
	}

	RING_IO_0Print ("Leaving RING_IO_ReaderClient1 () \n");

	////////////////////////////////////////////////////////////////////////////////
	//End close  the read  task
	////////////////////////////////////////////////////////////////////////////////


	/* Exit */
	RING_IO_Exit_client(&writerClientInfo1);

	return (NULL);
}

NORMAL_API
Void *
RING_IO_WriterClient2 (IN Void * ptr)
{

	DSP_STATUS status = DSP_SOK;
	DSP_STATUS relStatus = DSP_SOK;
	DSP_STATUS tmpStatus = DSP_SOK;
	RingIO_BufPtr bufPtr = NULL;
	Pvoid semPtrWriter = NULL;
	Uint8 i = 0;
	Uint32 bytesTransfered = 0;
	Uint32 attrs [RING_IO_VATTR_SIZE];
	Uint16 type;
	Uint32 acqSize;

	Pvoid semPtrReader = NULL;
	Uint32 param;
	Uint32 vAttrSize = 0;
	Uint32 rcvSize = RING_IO_BufferSize1;
	Uint32 totalRcvbytes = 0;
	Uint8 exitFlag = FALSE;
	DSP_STATUS attrStatus = DSP_SOK;

	///////////////////////////////////////////////////////////////////////////////

	// initial the write task

	///////////////////////////////////////////////////////////////////////////////

	RING_IO_0Print ("Entered RING_IO_WriterClient2 ()\n");

	/*
	 *  Open the RingIO to be used with GPP as the writer.
	 *
	 *  Value of the flags indicates:
	 *     No cache coherence for: Control structure
	 *                             Data buffer
	 *                             Attribute buffer
	 *     Exact size requirement.
	 */
	RingIOWriterHandle2 = RingIO_open (RingIOWriterName2,
			RINGIO_MODE_WRITE,
			(Uint32) (RINGIO_NEED_EXACT_SIZE));
	if (RingIOWriterHandle2 == NULL) {
		status = RINGIO_EFAILURE;
		RING_IO_1Print ("RingIO_open2 () Writer failed. Status = [0x%x]\n",
				status);
	}

	//RING_IO_0Print ("RingIO_open2 () Writer\n");

	if (DSP_SUCCEEDED (status)) {
		/* Create the semaphore to be used for notification */
		status = RING_IO_CreateSem (&semPtrWriter);
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("RING_IO_CreateSem2 () Writer SEM failed "
					"Status = [0x%x]\n",
					status);
		}
	}

	//RING_IO_0Print ("RING_IO_CreateSem2 () Writer SEM \n");

	if (DSP_SUCCEEDED (status)) {
		/*
		 *  Set the notification for Writer.
		 */
		do {
			/* Set the notifier for writer for RingIO created by the GPP. */
			status = RingIO_setNotifier (RingIOWriterHandle2,
					RINGIO_NOTIFICATION_ONCE,
					RING_IO_WRITER_BUF_SIZE,
					&RING_IO_Writer_Notify2,
					(RingIO_NotifyParam) semPtrWriter);
			if (status != RINGIO_SUCCESS) {
				RING_IO_Sleep(10);
			}
		}while (DSP_FAILED (status));

	}

	//RING_IO_0Print ("RingIO_setNotifier2 () Writer SEM \n");
	///////////////////////////////////////////////////////////////////////////////
	//end  initial the write task
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	// initial the read  task	
	///////////////////////////////////////////////////////////////////////////////

	RING_IO_0Print ("Entered RING_IO_ReaderClient2 ()\n");

	/*
	 *  Open the RingIO to be used with GPP as the reader.
	 *  Value of the flags indicates:
	 *     No cache coherence for: Control structure
	 *                             Data buffer
	 *                             Attribute buffer
	 *     Exact size requirement false.
	 */
	do {
		RingIOReaderHandle2 = RingIO_open (RingIOReaderName2,
				RINGIO_MODE_READ,
				0);
		//RING_IO_0Print (" RingIO_open (RingIOReaderName2)ing  \n") ;

	}while (RingIOReaderHandle2 == NULL);

	//RING_IO_0Print (" RingIO_open (RingIOReaderName2,  \n");

	/* Create the semaphore to be used for notification */
	status = RING_IO_CreateSem (&semPtrReader);
	if (DSP_FAILED (status)) {
		RING_IO_1Print ("RING_IO_CreateSem2 () Reader SEM failed "
				"Status = [0x%x]\n",
				status);
	}
	//RING_IO_0Print (" RING_IO_CreateSem2 () Reader SEM   \n");

	if (DSP_SUCCEEDED(status)) {
		do {

			/* Set the notifier for reader for RingIO created by the DSP. */
			/*
			 * Set water mark to zero. and try to acquire the full buffer
			 * and  read what ever is available.
			 */
			status = RingIO_setNotifier (RingIOReaderHandle2,
					RINGIO_NOTIFICATION_ONCE,
					0,
					&RING_IO_Reader_Notify2,
					(RingIO_NotifyParam) semPtrReader);
			if (DSP_FAILED (status)) {
				/*RingIO_setNotifier () Reader failed  */
				RING_IO_Sleep(10);
			}
		}while (DSP_FAILED (status));
	}

	//RING_IO_0Print (" RingIO_setNotifier (RingIOReaderHandle2 reader \n");
	RING_IO_0Print ("End initial the read  task2 \n");

	///////////////////////////////////////////////////////////////////////////////
	//end initial the read  task
	///////////////////////////////////////////////////////////////////////////////


	while(1) {

		
		RING_IO_Sleep(5000000);
		RING_IO_0Print ("2222 sleep 5s and run \n");
		if(Task_Run == FALSE){
			RING_IO_0Print ("!!! WriteTask2 exit \n");

			break;
		}
				

		///////////////////////////////////////////////////////////////////////////////
		//the execute of write task
		///////////////////////////////////////////////////////////////////////////////


		if (DSP_SUCCEEDED (status)) {
			/* Send data transfer attribute (Fixed attribute) to DSP*/
			type = (Uint16) RINGIO_DATA_START;
			status = RingIO_setAttribute(RingIOWriterHandle2,
					0,
					type,
					0);
			if (DSP_FAILED(status)) {
				RING_IO_1Print ("RingIO_setAttribute2 failed to set the  "
						"RINGIO_DATA_START. Status = [0x%x]\n",
						status);
			}
		}

		RING_IO_0Print ("RingIO_setAttribute2   \n");

		if (DSP_SUCCEEDED (status)) {
			/* Send Notification  to  the reader (DSP)*/
			RING_IO_0Print ("GPP-->DSP2 Sent Data Transfer Start "
					"Attribute\n");
			do {
				status = RingIO_sendNotify (RingIOWriterHandle2,
						(RingIO_NotifyMsg)NOTIFY_DATA_START);
				if (DSP_FAILED(status)) {
					/* RingIO_sendNotify failed to send notification */
					RING_IO_Sleep(10);
				}
				else {
					RING_IO_0Print ("GPP-->DSP2:Sent Data Transfer Start "
							"Notification \n");
				}
			}while (status != RINGIO_SUCCESS);
		}

		if (DSP_SUCCEEDED (status)) {

			RING_IO_1Print ("2Bytes to transfer :%ld \n", RING_IO_BytesToTransfer2);
			RING_IO_1Print ("2Data buffer size  :%ld \n", RING_IO_BufferSize3);

			while ( (RING_IO_BytesToTransfer2 == 0)
					|| (bytesTransfered < RING_IO_BytesToTransfer2)) {

				/* Update the attrs to send variable attribute to DSP*/
				//attrs [0] = RING_IO_WRITER_BUF_SIZE;
				attrs [0] = RING_IO_BytesToTransfer2;

				/* ----------------------------------------------------------------
				 * Send to DSP.
				 * ----------------------------------------------------------------
				 */
				/* Set the scaling factor variable attribute*/
				status = RingIO_setvAttribute (RingIOWriterHandle2,
						0, /* at the beginning */
						0, /* No type */
						0,
						attrs,
						sizeof (attrs));
				if (DSP_FAILED(status)) {
					/* RingIO_setvAttribute failed */
					RING_IO_Sleep(10);
				}
				else {
					/* Acquire writer bufs and initialize and release them. */
					//acqSize = RING_IO_WRITER_BUF_SIZE;
					acqSize = RING_IO_BytesToTransfer2;
					status = RingIO_acquire (RingIOWriterHandle2,
							&bufPtr ,
							&acqSize);

					/* If acquire success . Write to  ring bufer and then release
					 * the acquired.
					 */
					if ((DSP_SUCCEEDED (status)) && (acqSize > 0)) {
						RING_IO_InitBuffer (bufPtr, acqSize);

						//debug
						Uint8 *ptr8 = (Uint8 *)(bufPtr);
						for (i = 0;i < 5; i++) {
								RING_IO_1Print ("    Send [0x%x]  ", *(ptr8+i));
							}
						RING_IO_0Print ("\n");



						

						if ( (RING_IO_BytesToTransfer2 != 0)
								&& ( (bytesTransfered + acqSize)
										> RING_IO_BytesToTransfer2)) {

							/* we have acquired more buffer than the rest of data
							 * bytes to be transferred */
							if (bytesTransfered != RING_IO_BytesToTransfer2) {

								relStatus = RingIO_release (RingIOWriterHandle2,
										(RING_IO_BytesToTransfer2-
												bytesTransfered));
								if (DSP_FAILED (relStatus)) {
									RING_IO_1Print ("RingIO_release2 () in Writer "
											"task failed relStatus = [0x%x]"
											"\n" , relStatus);
								}
							}

							/* Cancel the  rest of the buffer */
							status = RingIO_cancel (RingIOWriterHandle2);
							if (DSP_FAILED(status)) {
								RING_IO_1Print ("RingIO_cancel2 () in Writer"
										"task failed "
										"status = [0x%x]\n",
										status);
							}
							bytesTransfered = RING_IO_BytesToTransfer2;

						}
						else {

							relStatus = RingIO_release (RingIOWriterHandle2,
									acqSize);
							if (DSP_FAILED (relStatus)) {
								RING_IO_1Print ("RingIO_release () in Writer task "
										"failed. relStatus = [0x%x]\n",
										relStatus);
							}
							else {
								bytesTransfered += acqSize;
							}
						}

						/*if ((bytesTransfered % (RING_IO_WRITER_BUF_SIZE * 8u)) == 0)
						 {
						 RING_IO_1Print ("GPP-->DSP2:Bytes Transferred: %lu \n",
						 bytesTransfered);
						 }*/
					}
					else {

						/* Acquired failed, Wait for empty buffer to become
						 * available.
						 */
						status = RING_IO_WaitSem (semPtrWriter);
						if (DSP_FAILED (status)) {
							RING_IO_1Print ("RING_IO_WaitSem () Writer SEM failed "
									"Status = [0x%x]\n",
									status);
						}
					}
				}
			}

			RING_IO_1Print ("GPP-->DSP2: Total Bytes Transmitted  %ld \n",
					bytesTransfered);

			bytesTransfered = 0;

			/* Send  End of  data transfer attribute to DSP */
			type = (Uint16) RINGIO_DATA_END;

			do {
				status = RingIO_setAttribute (RingIOWriterHandle2,
						0,
						type,
						0);
				if (DSP_SUCCEEDED(status)) {
					RING_IO_1Print ("RingIO_setAttribute2 succeeded to set the  "
							"RINGIO_DATA_END. Status = [0x%x]\n",
							status);
				}
				//else {
				//	RING_IO_YieldClient ();
				//}
			}while (status != RINGIO_SUCCESS);

			RING_IO_0Print ("GPP-->DSP2:Sent Data Transfer End Attribute\n");

			if (DSP_SUCCEEDED (status)) {

				/* Send Notification  to  the reader (DSP)
				 * This allows DSP  application to come out from blocked state  if
				 * it is waiting for Data buffer and  GPP sent only data end
				 * attribute.
				 */
				status = RingIO_sendNotify (RingIOWriterHandle2,
						(RingIO_NotifyMsg)NOTIFY_DATA_END);
				if (DSP_FAILED(status)) {
					RING_IO_1Print ("RingIO_sendNotify2 failed to send notification "
							"NOTIFY_DATA_END. Status = [0x%x]\n",
							status);
				}
				else {
					RING_IO_0Print ("GPP-->DSP2:Sent Data Transfer End Notification"
							" \n");
					RING_IO_YieldClient ();
				}
			}
		}

		///////////////////////////////////////////////////////////////////////////////
		//end the execute of write task
		///////////////////////////////////////////////////////////////////////////////


		///////////////////////////////////////////////////////////////////////////////
		// the execute of read task
		///////////////////////////////////////////////////////////////////////////////
		if (DSP_SUCCEEDED(status)) {

			/*
			 * Wait for notification from  DSP  about data
			 * transfer
			 */
			status = RING_IO_WaitSem (semPtrReader);
			if (DSP_FAILED (status)) {
				RING_IO_1Print ("RING_IO_WaitSem2 () Reader SEM failed "
						"Status = [0x%x]\n",
						status);
			}

			RING_IO_0Print (" RING_IO_WaitSem2 () Reader SEM  \n");

			if (fReaderStart2 == TRUE) {

				fReaderStart2 = FALSE;

				/* Got  data transfer start notification from DSP*/
				do {

					/* Get the start attribute from dsp */
					status = RingIO_getAttribute (RingIOReaderHandle2,
							&type,
							&param);
					if ( (status == RINGIO_SUCCESS)
							|| (status == RINGIO_SPENDINGATTRIBUTE)) {

						if (type == (Uint16)RINGIO_DATA_START) {
							RING_IO_0Print ("GPP<--DSP2:Received Data Transfer"
									"Start Attribute\n");
							break;
						}
						else {
							RING_IO_1Print ("RingIO_getAttribute2 () Reader failed  "
									"Unknown attribute received instead of "
									"RINGIO_DATA_START. Status = [0x%x]\n",
									status);
							RING_IO_Sleep(10);
						}
					}
					else {
						RING_IO_Sleep(10);
					}

				}while ( (status != RINGIO_SUCCESS)
						&& (status != RINGIO_SPENDINGATTRIBUTE));
			}

			/* Now reader  can start reading data from the ringio created
			 * by Dsp as the writer
			 */
			acqSize = RING_IO_BufferSize3;
			while (exitFlag == FALSE) {

				status = RingIO_acquire (RingIOReaderHandle2,
						&bufPtr ,
						&acqSize);

				if ((status == RINGIO_SUCCESS)
						||(acqSize > 0)) {
					/* Got buffer from DSP.*/
					rcvSize -= acqSize;
					totalRcvbytes += acqSize;

					/* Verify the received data */
					if (DSP_SOK != RING_IO_Reader_VerifyData (bufPtr,
									//factor,
									//action,
									acqSize)) {
						RING_IO_1Print (" Data verification2 failed after"
								"%ld bytes received from DSP \n",
								totalRcvbytes);
					}

					/* Release the acquired buffer */
					relStatus = RingIO_release (RingIOReaderHandle2,
							acqSize);
					if (DSP_FAILED (relStatus)) {
						RING_IO_1Print ("RingIO_release2 () in Writer task"
								"failed relStatus = [0x%x]\n",
								relStatus);
					}

					/* Set the acqSize for the next acquire */
					if (rcvSize == 0) {
						/* Reset  the rcvSize to  size of the full buffer  */
						rcvSize = RING_IO_BufferSize3;
						acqSize = RING_IO_BufferSize3;
					}
					else {
						/*Acquire the partial buffer  in next acquire */
						acqSize = rcvSize;
					}

					/*if ((totalRcvbytes % (8192u)) == 0u) {
					 RING_IO_1Print ("GPP<--DSP2:Bytes Received :%lu \n",
					 totalRcvbytes);

					 }*/

				}
				else if ( (status == RINGIO_SPENDINGATTRIBUTE)
						&& (acqSize == 0u)) {

					/* Attribute is pending,Read it */
					attrStatus = RingIO_getAttribute (RingIOReaderHandle2,
							&type,
							&param);
					if ((attrStatus == RINGIO_SUCCESS)
							|| (attrStatus == RINGIO_SPENDINGATTRIBUTE)) {

						if (type == RINGIO_DATA_END) {
							/* End of data transfer from DSP */
							RING_IO_0Print ("GPP<--DSP2:Received Data Transfer"
									"End Attribute \n");
							exitFlag = TRUE;/* Come Out of while loop */
						}
						else {
							RING_IO_1Print ("RingIO_getAttribute () Reader "
									"error,Unknown attribute "
									" received Status = [0x%x]\n",
									attrStatus);
						}
					}
					else if (attrStatus == RINGIO_EVARIABLEATTRIBUTE) {

						vAttrSize = sizeof(attrs);
						attrStatus = RingIO_getvAttribute (RingIOReaderHandle2,
								&type,
								&param,
								attrs,
								&vAttrSize);

						if ((attrStatus == RINGIO_SUCCESS)
								|| (attrStatus == RINGIO_SPENDINGATTRIBUTE)) {

							/* Success in receiving  variable attribute*/
							rcvSize = attrs[0];

							/* Set the  acquire size equal to the
							 * rcvSize
							 */
							acqSize = rcvSize;

							RING_IO_1Print ("222RingIO_getAttribute () Reader "
									" received size = [%d]\n",
									rcvSize);

						}
						else if (attrStatus == RINGIO_EVARIABLEATTRIBUTE) {

							RING_IO_1Print ("Error2: "
									"buffer is not sufficient to receive"
									"the  variable attribute "
									"Status = [0x%x]\n",
									attrStatus);
						}
						else if (attrStatus == DSP_EINVALIDARG) {

							RING_IO_1Print ("Error2: "
									"Invalid args to receive"
									"the  variable attribute "
									"Status = [0x%x]\n",
									attrStatus);

						}
						else if (attrStatus ==RINGIO_EPENDINGDATA) {

							RING_IO_1Print ("Erro2r:RingIO_getvAttribute "
									"Status = [0x%x]\n",
									attrStatus);
						}
						else {
							/* Unable to get  variable attribute
							 * go back to read data.
							 * this may occur due to ringo flush by the DSP
							 * or may be due to  general failure
							 */
						}

					}
					else {
						RING_IO_1Print ("RingIO_getAttribute2 () Reader error "
								"Status = [0x%x]\n",
								attrStatus);
					}
				}
				else if ( (status == RINGIO_EFAILURE)
						||(status == RINGIO_EBUFEMPTY)) {

					/* Failed to acquire buffer */
					status = RING_IO_WaitSem (semPtrReader);
					if (DSP_FAILED (status)) {
						RING_IO_1Print ("RING_IO_WaitSem () Reader SEM failed "
								"Status = [0x%x]\n",
								status);
					}
				}
				else {
					acqSize = RING_IO_BufferSize3;

				}

				/* Reset the acquired size if it is changed to zero by the
				 * failed acquire call
				 */
				if (acqSize == 0) {
					acqSize = RING_IO_BufferSize3;
				}

			}
		}

		RING_IO_1Print ("GPP<--DSP2:Bytes Received %ld \n",
				totalRcvbytes);

		if (fReaderEnd2 != TRUE) {
			/* If data transfer end notification  not yet received
			 * from DSP ,wait for it.
			 */
			status = RING_IO_WaitSem (semPtrReader);
			if (DSP_FAILED (status)) {
				RING_IO_1Print ("RING_IO_WaitSem2 () Reader SEM failed "
						"Status = [0x%x]\n",
						status);
			}
		}
		//else {
			
		//}
		totalRcvbytes = 0;
		rcvSize = RING_IO_BufferSize3;
		fReaderEnd2 = FALSE;
		exitFlag = FALSE;

		RING_IO_0Print (" End Reader task2  \n");

	}

	///////////////////////////////////////////////////////////////////////////////
	//close  the write  task	
	///////////////////////////////////////////////////////////////////////////////

	do {
	tmpStatus = RingIO_sendNotify (RingIOWriterHandle2,
						(RingIO_NotifyMsg)NOTIFY_DSP_END);
	if (DSP_FAILED(tmpStatus)) {
			status = tmpStatus;
			RING_IO_0Print("RingIO_sendNotify (RingIOWriterHandle2)\n");
			RING_IO_Sleep(10);
		} else {
			status = RINGIO_SUCCESS;
		}
	} while (DSP_FAILED(tmpStatus));


	/* Delete the semaphore used for notification */
	if (semPtrWriter != NULL) {
		tmpStatus = RING_IO_DeleteSem (semPtrWriter);
		if (DSP_SUCCEEDED (status) && DSP_FAILED (tmpStatus)) {
			status = tmpStatus;
			RING_IO_1Print ("RING_IO_DeleteSem2 () Writer SEM failed "
					"Status = [0x%x]\n",
					status);
		}
	}

	//RING_IO_0Print ("RING_IO_DeleteSem2 () Writer SEM \n");

	/*
	 *  Close the RingIO to be used with GPP as the writer.
	 */
	if (RingIOWriterHandle2 != NULL) {
		while ( (RingIO_getValidSize(RingIOWriterHandle2) != 0)
				|| (RingIO_getValidAttrSize(RingIOWriterHandle2) != 0)) {
			RING_IO_Sleep(10);
		}
		tmpStatus = RingIO_close (RingIOWriterHandle2);
		if (DSP_FAILED (tmpStatus)) {
			RING_IO_1Print ("RingIO_close2 () Writer failed. Status = [0x%x]\n",
					status);
		}
	}

	RING_IO_0Print ("Leaving RING_IO_WriterClient2 () \n");

	///////////////////////////////////////////////////////////////////////////////
	//End close  the write  task	
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	//close  the read  task
	///////////////////////////////////////////////////////////////////////////////

	if (semPtrReader != NULL) {
		tmpStatus = RING_IO_DeleteSem (semPtrReader);
		if (DSP_SUCCEEDED (status) && DSP_FAILED (tmpStatus)) {
			status = tmpStatus;
			RING_IO_1Print ("RING_IO_DeleteSem2 () Reader SEM failed "
					"Status = [0x%x]\n",
					status);
		}
	}

	//RING_IO_0Print ("RING_IO_DeleteSem2 () Reader SEM  \n");

	if (RingIOReaderHandle2 != NULL) {
		tmpStatus = RingIO_close (RingIOReaderHandle2);
		if (DSP_FAILED (tmpStatus)) {
			RING_IO_1Print ("RingIO_close2 () Reader failed. Status = [0x%x]\n",
					status);
		}
	}

	RING_IO_0Print ("Leaving RING_IO_ReaderClient2 () \n");

	///////////////////////////////////////////////////////////////////////////////
	//End close  the read  task	
	///////////////////////////////////////////////////////////////////////////////


	/* Exit */
	RING_IO_Exit_client(&writerClientInfo2);

	return (NULL);
}

/** ============================================================================
 *  @func   RING_IO_ReaderClient
 *
 *  @desc   This function implements the Reader task  for this sample
 *          application.
 *          The  Reader task has the following flow:
 *
 *          1.  This task (GPP RingIO reader) sets the notifier for the reader
 *              (RINGIO2) with the specific watermark value of zero.
 *              Pointer to a semaphore is passed   to the notifier function.
 *              The notifier function post the semaphore passed to it,
 *              resulting in unblocking the reader task which would be waiting
 *              on it.
 *
 *          2.  It waits on semaphore to receive a start notification from
 *              the DSP.
 *          3.  After receiving notification from the RINGIO2 writer (i.e. DSP),
 *              it tries to get the start attribute (RINGIO_DATA_START).If the
 *              start attribute is received, reader task starts reading data.
 *
 *          4.  It acquires data buffer in read mode from the RINGO2 and
 *              verifies the contents based on the variable attribute received
 *              prior to this acquire call.This task always tries to acquire
 *              the full buffer and gets what is available in the RINGIO2.
 *              If nothing is available, it waits on a semaphore for
 *              notification.
 *
 *          5.  Step 4 is performed repeatedly until it receives end of data
 *              transfer attribute (RINIGIO_DATA_END) from DSP.
 *
 *          6.  It deletes the created semaphore and exits.
 *
 *
 *  @modif  None
 *  ============================================================================
 */
NORMAL_API
Void *
RING_IO_ReaderClient1 (IN Void * ptr)
{

	DSP_STATUS status = DSP_SOK;
	DSP_STATUS relStatus = DSP_SOK;
	DSP_STATUS tmpStatus = DSP_SOK;
	DSP_STATUS attrStatus = DSP_SOK;
	RingIO_BufPtr bufPtr = NULL;
	Pvoid semPtrReader = NULL;
	Uint32 vAttrSize = 0;
	Uint32 rcvSize = RING_IO_BufferSize1;
	Uint32 totalRcvbytes = 0;
	Uint8 exitFlag = FALSE;
	Uint32 factor = 0;
	Uint32 action = 0;
	Uint16 type;
	Uint32 param;
	Uint32 acqSize;
	Uint32 attrs [RING_IO_VATTR_SIZE];

	RING_IO_0Print ("Entered RING_IO_ReaderClient1 ()\n");

	/*
	 *  Open the RingIO to be used with GPP as the reader.
	 *  Value of the flags indicates:
	 *     No cache coherence for: Control structure
	 *                             Data buffer
	 *                             Attribute buffer
	 *     Exact size requirement false.
	 */
	do {
		RingIOReaderHandle1 = RingIO_open (RingIOReaderName1,
				RINGIO_MODE_READ,
				0);

		//	RING_IO_0Print (" RingIO_open (RingIOReaderName1()ing \n") ;

	}while (RingIOReaderHandle1 == NULL);

	RING_IO_0Print (" RingIO_open (RingIOReaderName1  ()\n");

	/* Create the semaphore to be used for notification */
	status = RING_IO_CreateSem (&semPtrReader);
	if (DSP_FAILED (status)) {
		RING_IO_1Print ("RING_IO_CreateSem1 () Reader SEM failed "
				"Status = [0x%x]\n",
				status);
	}

	RING_IO_0Print (" RING_IO_CreateSem1 () Reader SEM  \n");

	if (DSP_SUCCEEDED(status)) {
		do {

			/* Set the notifier for reader for RingIO created by the DSP. */
			/*
			 * Set water mark to zero. and try to acquire the full buffer
			 * and  read what ever is available.
			 */
			status = RingIO_setNotifier (RingIOReaderHandle1,
					RINGIO_NOTIFICATION_ONCE,
					0,
					&RING_IO_Reader_Notify1,
					(RingIO_NotifyParam) semPtrReader);
			if (DSP_FAILED (status)) {
				/*RingIO_setNotifier () Reader failed  */
				RING_IO_Sleep(10);
			}
		}while (DSP_FAILED (status));
	}

	RING_IO_0Print (" RingIO_setNotifier1 Reader SEM  \n");

	if (DSP_SUCCEEDED(status)) {

		/*
		 * Wait for notification from  DSP  about data
		 * transfer
		 */
		status = RING_IO_WaitSem (semPtrReader);
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("RING_IO_WaitSem1 () Reader SEM failed "
					"Status = [0x%x]\n",
					status);
		}
		RING_IO_0Print (" RING_IO_WaitSem1 () Reader SEM  \n");

		if (fReaderStart1 == TRUE) {

			/* Got  data transfer start notification from DSP*/
			do {

				/* Get the start attribute from dsp */
				status = RingIO_getAttribute (RingIOReaderHandle1,
						&type,
						&param);
				if ( (status == RINGIO_SUCCESS)
						|| (status == RINGIO_SPENDINGATTRIBUTE)) {

					if (type == (Uint16)RINGIO_DATA_START) {
						RING_IO_0Print ("GPP<--DSP1:Received Data Transfer"
								"Start Attribute\n");
						break;
					}
					else {
						RING_IO_1Print ("RingIO_getAttribute1 () Reader failed  "
								"Unknown attribute received instead of "
								"RINGIO_DATA_START. Status = [0x%x]\n",
								status);
						RING_IO_Sleep(10);
					}
				}
				else {
					RING_IO_Sleep(10);
				}

			}while ( (status != RINGIO_SUCCESS)
					&& (status != RINGIO_SPENDINGATTRIBUTE));
		}

		/* Now reader  can start reading data from the ringio created
		 * by Dsp as the writer
		 */
		acqSize = RING_IO_BufferSize1;
		while (exitFlag == FALSE) {

			status = RingIO_acquire (RingIOReaderHandle1,
					&bufPtr ,
					&acqSize);

			if ((status == RINGIO_SUCCESS)
					||(acqSize > 0)) {
				/* Got buffer from DSP.*/
				rcvSize -= acqSize;
				totalRcvbytes += acqSize;

				/* Verify the received data */
				if (DSP_SOK != RING_IO_Reader_VerifyData (bufPtr,
								//factor,
								//action,
								acqSize)) {
					RING_IO_1Print (" Data1 verification failed after"
							"%ld bytes received from DSP \n",
							totalRcvbytes);
				}

				/* Release the acquired buffer */
				relStatus = RingIO_release (RingIOReaderHandle1,
						acqSize);
				if (DSP_FAILED (relStatus)) {
					RING_IO_1Print ("RingIO_release1 () in Writer task"
							"failed relStatus = [0x%x]\n",
							relStatus);
				}

				/* Set the acqSize for the next acquire */
				if (rcvSize == 0) {
					/* Reset  the rcvSize to  size of the full buffer  */
					rcvSize = RING_IO_BufferSize1;
					acqSize = RING_IO_BufferSize1;
				}
				else {
					/*Acquire the partial buffer  in next acquire */
					acqSize = rcvSize;
				}

				if ((totalRcvbytes % (8192u)) == 0u) {
					RING_IO_1Print ("GPP<--DSP1:Bytes Received :%lu \n",
							totalRcvbytes);

				}

			}
			else if ( (status == RINGIO_SPENDINGATTRIBUTE)
					&& (acqSize == 0u)) {

				/* Attribute is pending,Read it */
				attrStatus = RingIO_getAttribute (RingIOReaderHandle1,
						&type,
						&param);
				if ((attrStatus == RINGIO_SUCCESS)
						|| (attrStatus == RINGIO_SPENDINGATTRIBUTE)) {

					if (type == RINGIO_DATA_END) {
						/* End of data transfer from DSP */
						RING_IO_0Print ("GPP<--DSP1:Received Data Transfer"
								"End Attribute \n");
						exitFlag = TRUE;/* Come Out of while loop */
					}
					else {
						RING_IO_1Print ("RingIO_getAttribute () Reader "
								"error,Unknown attribute "
								" received Status = [0x%x]\n",
								attrStatus);
					}
				}
				else if (attrStatus == RINGIO_EVARIABLEATTRIBUTE) {

					vAttrSize = sizeof(attrs);
					attrStatus = RingIO_getvAttribute (RingIOReaderHandle1,
							&type,
							&param,
							attrs,
							&vAttrSize);

					if ((attrStatus == RINGIO_SUCCESS)
							|| (attrStatus == RINGIO_SPENDINGATTRIBUTE)) {

						/* Success in receiving  variable attribute*/
						rcvSize = attrs[0];
						/* Set the  acquire size equal to the
						 * rcvSize
						 */
						acqSize = rcvSize;
					}
					else if (attrStatus == RINGIO_EVARIABLEATTRIBUTE) {

						RING_IO_1Print ("Error: "
								"buffer is not sufficient to receive"
								"the  variable attribute "
								"Status = [0x%x]\n",
								attrStatus);
					}
					else if (attrStatus == DSP_EINVALIDARG) {

						RING_IO_1Print ("Error: "
								"Invalid args to receive"
								"the  variable attribute "
								"Status = [0x%x]\n",
								attrStatus);

					}
					else if (attrStatus ==RINGIO_EPENDINGDATA) {

						RING_IO_1Print ("Error:RingIO_getvAttribute "
								"Status = [0x%x]\n",
								attrStatus);
					}
					else {
						/* Unable to get  variable attribute
						 * go back to read data.
						 * this may occur due to ringo flush by the DSP
						 * or may be due to  general failure
						 */
					}

				}
				else {
					RING_IO_1Print ("RingIO_getAttribute () Reader error "
							"Status = [0x%x]\n",
							attrStatus);
				}
			}
			else if ( (status == RINGIO_EFAILURE)
					||(status == RINGIO_EBUFEMPTY)) {

				/* Failed to acquire buffer */
				status = RING_IO_WaitSem (semPtrReader);
				if (DSP_FAILED (status)) {
					RING_IO_1Print ("RING_IO_WaitSem () Reader SEM failed "
							"Status = [0x%x]\n",
							status);
				}
			}
			else {
				acqSize = RING_IO_BufferSize1;

			}

			/* Reset the acquired size if it is changed to zero by the
			 * failed acquire call
			 */
			if (acqSize == 0) {
				acqSize = RING_IO_BufferSize1;
			}

		}
	}

	RING_IO_1Print ("GPP<--DSP1:Bytes Received %ld \n",
			totalRcvbytes);

	if (fReaderEnd1 != TRUE) {
		/* If data transfer end notification  not yet received
		 * from DSP ,wait for it.
		 */
		status = RING_IO_WaitSem (semPtrReader);
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("RING_IO_WaitSem1 () Reader SEM failed "
					"Status = [0x%x]\n",
					status);
		}
	}

	if (fReaderEnd1 == TRUE) {
		RING_IO_0Print ("GPP<--DSP1:Received Data Transfer End Notification"
				" \n");
		if (semPtrReader != NULL) {
			tmpStatus = RING_IO_DeleteSem (semPtrReader);
			if (DSP_SUCCEEDED (status) && DSP_FAILED (tmpStatus)) {
				status = tmpStatus;
				RING_IO_1Print ("RING_IO_DeleteSem1 () Reader SEM failed "
						"Status = [0x%x]\n",
						status);
			}
		}
	}

	RING_IO_0Print (" RING_IO_DeleteSem1 () Reader SEM   \n");

	/*
	 *  Close the RingIO to be used with GPP as the reader.
	 */
	if (RingIOReaderHandle1 != NULL) {
		tmpStatus = RingIO_close (RingIOReaderHandle1);
		if (DSP_FAILED (tmpStatus)) {
			RING_IO_1Print ("RingIO_close1 () Reader failed. Status = [0x%x]\n",
					status);
		}
	}

	RING_IO_0Print ("Leaving RING_IO_ReaderClient1 () \n");

	/* Exit  */
	RING_IO_Exit_client(&readerClientInfo1);

	return (NULL);
}

NORMAL_API
Void *
RING_IO_ReaderClient2 (IN Void * ptr)
{

	DSP_STATUS status = DSP_SOK;
	DSP_STATUS relStatus = DSP_SOK;
	DSP_STATUS tmpStatus = DSP_SOK;
	DSP_STATUS attrStatus = DSP_SOK;
	RingIO_BufPtr bufPtr = NULL;
	Pvoid semPtrReader = NULL;
	Uint32 vAttrSize = 0;
	Uint32 rcvSize = RING_IO_BufferSize3;
	Uint32 totalRcvbytes = 0;
	Uint8 exitFlag = FALSE;
	Uint32 factor = 0;
	Uint32 action = 0;
	Uint16 type;
	Uint32 param;
	Uint32 acqSize;
	Uint32 attrs [RING_IO_VATTR_SIZE];

	RING_IO_0Print ("Entered RING_IO_ReaderClient2 ()\n");

	/*
	 *  Open the RingIO to be used with GPP as the reader.
	 *  Value of the flags indicates:
	 *     No cache coherence for: Control structure
	 *                             Data buffer
	 *                             Attribute buffer
	 *     Exact size requirement false.
	 */
	do {
		RingIOReaderHandle2 = RingIO_open (RingIOReaderName2,
				RINGIO_MODE_READ,
				0);
		//RING_IO_0Print (" RingIO_open (RingIOReaderName2)ing  \n") ;

	}while (RingIOReaderHandle2 == NULL);

	RING_IO_0Print (" RingIO_open (RingIOReaderName2,  \n");

	/* Create the semaphore to be used for notification */
	status = RING_IO_CreateSem (&semPtrReader);
	if (DSP_FAILED (status)) {
		RING_IO_1Print ("RING_IO_CreateSem2 () Reader SEM failed "
				"Status = [0x%x]\n",
				status);
	}
	RING_IO_0Print (" RING_IO_CreateSem2 () Reader SEM   \n");

	if (DSP_SUCCEEDED(status)) {
		do {

			/* Set the notifier for reader for RingIO created by the DSP. */
			/*
			 * Set water mark to zero. and try to acquire the full buffer
			 * and  read what ever is available.
			 */
			status = RingIO_setNotifier (RingIOReaderHandle2,
					RINGIO_NOTIFICATION_ONCE,
					0,
					&RING_IO_Reader_Notify2,
					(RingIO_NotifyParam) semPtrReader);
			if (DSP_FAILED (status)) {
				/*RingIO_setNotifier () Reader failed  */
				RING_IO_Sleep(10);
			}
		}while (DSP_FAILED (status));
	}

	RING_IO_0Print (" RingIO_setNotifier (RingIOReaderHandle2 reader \n");

	if (DSP_SUCCEEDED(status)) {

		/*
		 * Wait for notification from  DSP  about data
		 * transfer
		 */
		status = RING_IO_WaitSem (semPtrReader);
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("RING_IO_WaitSem2 () Reader SEM failed "
					"Status = [0x%x]\n",
					status);
		}

		RING_IO_0Print (" RING_IO_WaitSem2 () Reader SEM  \n");

		if (fReaderStart2 == TRUE) {

			/* Got  data transfer start notification from DSP*/
			do {

				/* Get the start attribute from dsp */
				status = RingIO_getAttribute (RingIOReaderHandle2,
						&type,
						&param);
				if ( (status == RINGIO_SUCCESS)
						|| (status == RINGIO_SPENDINGATTRIBUTE)) {

					if (type == (Uint16)RINGIO_DATA_START) {
						RING_IO_0Print ("GPP<--DSP2:Received Data Transfer"
								"Start Attribute\n");
						break;
					}
					else {
						RING_IO_1Print ("RingIO_getAttribute2 () Reader failed  "
								"Unknown attribute received instead of "
								"RINGIO_DATA_START. Status = [0x%x]\n",
								status);
						RING_IO_Sleep(10);
					}
				}
				else {
					RING_IO_Sleep(10);
				}

			}while ( (status != RINGIO_SUCCESS)
					&& (status != RINGIO_SPENDINGATTRIBUTE));
		}

		/* Now reader  can start reading data from the ringio created
		 * by Dsp as the writer
		 */
		acqSize = RING_IO_BufferSize3;
		while (exitFlag == FALSE) {

			status = RingIO_acquire (RingIOReaderHandle2,
					&bufPtr ,
					&acqSize);

			if ((status == RINGIO_SUCCESS)
					||(acqSize > 0)) {
				/* Got buffer from DSP.*/
				rcvSize -= acqSize;
				totalRcvbytes += acqSize;

				/* Verify the received data */
				if (DSP_SOK != RING_IO_Reader_VerifyData (bufPtr,
								//factor,
								//action,
								acqSize)) {
					RING_IO_1Print (" Data verification2 failed after"
							"%ld bytes received from DSP \n",
							totalRcvbytes);
				}

				/* Release the acquired buffer */
				relStatus = RingIO_release (RingIOReaderHandle2,
						acqSize);
				if (DSP_FAILED (relStatus)) {
					RING_IO_1Print ("RingIO_release2 () in Writer task"
							"failed relStatus = [0x%x]\n",
							relStatus);
				}

				/* Set the acqSize for the next acquire */
				if (rcvSize == 0) {
					/* Reset  the rcvSize to  size of the full buffer  */
					rcvSize = RING_IO_BufferSize3;
					acqSize = RING_IO_BufferSize3;
				}
				else {
					/*Acquire the partial buffer  in next acquire */
					acqSize = rcvSize;
				}

				if ((totalRcvbytes % (8192u)) == 0u) {
					RING_IO_1Print ("GPP<--DSP2:Bytes Received :%lu \n",
							totalRcvbytes);

				}

			}
			else if ( (status == RINGIO_SPENDINGATTRIBUTE)
					&& (acqSize == 0u)) {

				/* Attribute is pending,Read it */
				attrStatus = RingIO_getAttribute (RingIOReaderHandle2,
						&type,
						&param);
				if ((attrStatus == RINGIO_SUCCESS)
						|| (attrStatus == RINGIO_SPENDINGATTRIBUTE)) {

					if (type == RINGIO_DATA_END) {
						/* End of data transfer from DSP */
						RING_IO_0Print ("GPP<--DSP2:Received Data Transfer"
								"End Attribute \n");
						exitFlag = TRUE;/* Come Out of while loop */
					}
					else {
						RING_IO_1Print ("RingIO_getAttribute () Reader "
								"error,Unknown attribute "
								" received Status = [0x%x]\n",
								attrStatus);
					}
				}
				else if (attrStatus == RINGIO_EVARIABLEATTRIBUTE) {

					vAttrSize = sizeof(attrs);
					attrStatus = RingIO_getvAttribute (RingIOReaderHandle2,
							&type,
							&param,
							attrs,
							&vAttrSize);

					if ((attrStatus == RINGIO_SUCCESS)
							|| (attrStatus == RINGIO_SPENDINGATTRIBUTE)) {

						/* Success in receiving  variable attribute*/
						rcvSize = attrs[0];

						/* Set the  acquire size equal to the
						 * rcvSize
						 */
						acqSize = rcvSize;
					}
					else if (attrStatus == RINGIO_EVARIABLEATTRIBUTE) {

						RING_IO_1Print ("Error2: "
								"buffer is not sufficient to receive"
								"the  variable attribute "
								"Status = [0x%x]\n",
								attrStatus);
					}
					else if (attrStatus == DSP_EINVALIDARG) {

						RING_IO_1Print ("Error2: "
								"Invalid args to receive"
								"the  variable attribute "
								"Status = [0x%x]\n",
								attrStatus);

					}
					else if (attrStatus ==RINGIO_EPENDINGDATA) {

						RING_IO_1Print ("Erro2r:RingIO_getvAttribute "
								"Status = [0x%x]\n",
								attrStatus);
					}
					else {
						/* Unable to get  variable attribute
						 * go back to read data.
						 * this may occur due to ringo flush by the DSP
						 * or may be due to  general failure
						 */
					}

				}
				else {
					RING_IO_1Print ("RingIO_getAttribute2 () Reader error "
							"Status = [0x%x]\n",
							attrStatus);
				}
			}
			else if ( (status == RINGIO_EFAILURE)
					||(status == RINGIO_EBUFEMPTY)) {

				/* Failed to acquire buffer */
				status = RING_IO_WaitSem (semPtrReader);
				if (DSP_FAILED (status)) {
					RING_IO_1Print ("RING_IO_WaitSem () Reader SEM failed "
							"Status = [0x%x]\n",
							status);
				}
			}
			else {
				acqSize = RING_IO_BufferSize3;

			}

			/* Reset the acquired size if it is changed to zero by the
			 * failed acquire call
			 */
			if (acqSize == 0) {
				acqSize = RING_IO_BufferSize3;
			}

		}
	}

	RING_IO_1Print ("GPP<--DSP2:Bytes Received %ld \n",
			totalRcvbytes);

	if (fReaderEnd2 != TRUE) {
		/* If data transfer end notification  not yet received
		 * from DSP ,wait for it.
		 */
		status = RING_IO_WaitSem (semPtrReader);
		if (DSP_FAILED (status)) {
			RING_IO_1Print ("RING_IO_WaitSem2 () Reader SEM failed "
					"Status = [0x%x]\n",
					status);
		}
	}
	RING_IO_0Print (" RING_IO_WaitSem2 () Reader SEM  \n");

	if (fReaderEnd2 == TRUE) {
		RING_IO_0Print ("GPP<--DSP2:Received Data Transfer End Notification"
				" \n");
		if (semPtrReader != NULL) {
			tmpStatus = RING_IO_DeleteSem (semPtrReader);
			if (DSP_SUCCEEDED (status) && DSP_FAILED (tmpStatus)) {
				status = tmpStatus;
				RING_IO_1Print ("RING_IO_DeleteSem2 () Reader SEM failed "
						"Status = [0x%x]\n",
						status);
			}
		}
	}

	RING_IO_0Print ("RING_IO_DeleteSem2 () Reader SEM  \n");

	/*
	 *  Close the RingIO to be used with GPP as the reader.
	 */
	if (RingIOReaderHandle2 != NULL) {
		tmpStatus = RingIO_close (RingIOReaderHandle2);
		if (DSP_FAILED (tmpStatus)) {
			RING_IO_1Print ("RingIO_close2 () Reader failed. Status = [0x%x]\n",
					status);
		}
	}

	RING_IO_0Print ("Leaving RING_IO_ReaderClient2 () \n");

	/* Exit  */
	RING_IO_Exit_client(&readerClientInfo2);

	return (NULL);
}

/** ============================================================================
 *  @func   RING_IO_Delete
 *
 *  @desc   This function releases resources allocated earlier by call to
 *          RING_IO_Create ().
 *          During cleanup, the allocated resources are being freed
 *          unconditionally. Actual applications may require stricter check
 *          against return values for robustness.
 *
 *  @modif  None
 *  ============================================================================
 */
NORMAL_API
Void RING_IO_Delete(Uint8 processorId) {
	DSP_STATUS status = DSP_SOK;
	DSP_STATUS tmpStatus = DSP_SOK;

	RING_IO_0Print("Entered RING_IO_Delete ()\n");





/*	if (DSP_FAILED(status)) {
			
			RING_IO_Sleep(10);
		}
		else {
			RING_IO_0Print ("GPP-->DSP2:Sent Data Transfer Start "
							"Notification \n");
	}*/

	

	/*
	 *  Delete the sending RingIO to be used with GPP as the writer.
	 */
	do {
#if defined (DSPLINK_LEGACY_SUPPORT)
		tmpStatus = RingIO_delete (RingIOWriterName1);
#else
		tmpStatus = RingIO_delete(processorId, RingIOWriterName1);
#endif /* if defined (DSPLINK_LEGACY_SUPPORT) */

		if (DSP_FAILED(tmpStatus)) {
			status = tmpStatus;
			RING_IO_0Print("RingIO_delete (RingIOWriterName1)\n");
			RING_IO_Sleep(10);
		} else {
			status = RINGIO_SUCCESS;
		}
	} while (DSP_FAILED(tmpStatus));

	/*
	 *  Delete the receiving RingIO to be used with GPP as the writer.
	 */
	do {
#if defined (DSPLINK_LEGACY_SUPPORT)
		tmpStatus = RingIO_delete (RingIOWriterName2);
#else
		tmpStatus = RingIO_delete(processorId, RingIOWriterName2);
#endif /* if defined (DSPLINK_LEGACY_SUPPORT) */

		if (DSP_FAILED(tmpStatus)) {
			status = tmpStatus;
			RING_IO_Sleep(10);
		} else {
			status = RINGIO_SUCCESS;
		}
	} while (DSP_FAILED(tmpStatus));

	/*
	 *  Stop execution on DSP.
	 */
	tmpStatus = PROC_stop(processorId);
	if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
		status = tmpStatus;
		RING_IO_1Print("PROC_stop () failed. Status = [0x%x]\n", status);
	}

	/*
	 *  Close the pool
	 */
	tmpStatus = POOL_close(POOL_makePoolId(processorId, SAMPLE_POOL_ID));
	if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
		status = tmpStatus;
		RING_IO_1Print("POOL_close () failed. Status = [0x%x]\n", status);
	}

	/*
	 *  Detach from the processor
	 */
	tmpStatus = PROC_detach(processorId);
	if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
		status = tmpStatus;
		RING_IO_1Print("PROC_detach () failed. Status = [0x%x]\n", status);
	}

	/*
	 *  Destroy the PROC object.
	 */
	tmpStatus = PROC_destroy();
	if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
		status = tmpStatus;
		RING_IO_1Print("PROC_destroy () failed. Status = [0x%x]\n", status);
	}

	/*
	 *  OS Finalization
	 */
	tmpStatus = RING_IO_OS_exit();
	if (DSP_SUCCEEDED(status) && DSP_FAILED(tmpStatus)) {
		status = tmpStatus;
		RING_IO_1Print("RING_IO_OS_exit () failed. Status = [0x%x]\n", status);
	}

	RING_IO_0Print("Leaving RING_IO_Delete ()\n");
}

/** ============================================================================
 *  @func   RING_IO_Main
 *
 *  @desc   Entry point for the application
 *
 *  @modif  None
 *  ============================================================================
 */
NORMAL_API
Void
RING_IO_Main (IN Char8 * dspExecutable,
		IN Char8 * strBufferSize,
		IN Char8 * strBytesToTransfer,
		IN Char8 * strProcessorId)
{
	DSP_STATUS status = DSP_SOK;
	Uint8 processorId = 0;

	RING_IO_0Print ("========== Sample Application : RING_IO ==========\n");

	if ( (dspExecutable != NULL)) {
		/*
		 *  Validate the buffer size  specified.
		 */
		/*        RING_IO_BufferSize = DSPLINK_ALIGN (RING_IO_Atoi (strBufferSize),
		 DSPLINK_BUF_ALIGN) ;

		 RING_IO_BytesToTransfer1 = RING_IO_Atoi (strBytesToTransfer) ;
		 processorId             = RING_IO_Atoi (strProcessorId) ;
		 if (RING_IO_BytesToTransfer1 > 0u) {
		 // Make RING_IO_BytesToTransfer  multiple of DSPLINK_BUF_ALIGN
		 RING_IO_BytesToTransfer1 = DSPLINK_ALIGN(RING_IO_BytesToTransfer1,
		 DSPLINK_BUF_ALIGN) ;
		 }

		 //RING_IO_1Print ("Bytes to transfer :%ld \n", RING_IO_BytesToTransfer1) ;
		 //RING_IO_1Print ("Data buffer size  :%ld \n", RING_IO_BufferSize) ;

		 if (RING_IO_BufferSize < RING_IO_WRITER_BUF_SIZE) {
		 RING_IO_1Print ("NOTE:<argc = 2>Data buffer should be  atleast "
		 "%ld \n",
		 RING_IO_WRITER_BUF_SIZE) ;
		 status =   DSP_ESIZE ;

		 }*/

		if (processorId >= MAX_DSPS) {
			RING_IO_1Print ("==Error: Invalid processor id %d specified"
					" ==\n", processorId);
			status = DSP_EINVALIDARG;
		}

		if (status == DSP_SOK) {
			/*
			 *  Specify the dsp executable file name and the buffer size for
			 *  RING_IO creation phase.
			 */
			status = RING_IO_Create (dspExecutable,
					//strBufferSize,
					// strBytesToTransfer,
					processorId);

			if (DSP_SUCCEEDED (status)) {
				writerClientInfo1.processorId = processorId;
				status = RING_IO_Create_client(&writerClientInfo1,
						(Pvoid)RING_IO_WriterClient1, NULL);
				if (DSP_SUCCEEDED (status)) {
					writerClientInfo2.processorId = processorId;
					status = RING_IO_Create_client(&writerClientInfo2,
							(Pvoid)RING_IO_WriterClient2, NULL);
					if (status != DSP_SOK) {
#ifdef RING_IO_MULTIPROCESS
						RING_IO_0Print ("ERROR! Failed to create Reader Client "
								"Process in RING_IO application\n");

#else
						RING_IO_0Print ("ERROR! Failed to create Reader Task  "
								"RING_IO application\n");
#endif
					}

					/*	 if (status == DSP_SOK) {
					 readerClientInfo1.processorId = processorId ;
					 status = RING_IO_Create_client(&readerClientInfo1,
					 (Pvoid)RING_IO_ReaderClient1,
					 NULL) ;

					 if (status == DSP_SOK) {
					 readerClientInfo2.processorId = processorId ;
					 status =  RING_IO_Create_client(&readerClientInfo2,
					 (Pvoid)RING_IO_ReaderClient2,
					 NULL) ;

					 if (status != DSP_SOK) {
					 #ifdef RING_IO_MULTIPROCESS
					 RING_IO_0Print ("ERROR! Failed to create Reader Client "
					 "Process in RING_IO application\n") ;

					 #else
					 RING_IO_0Print ("ERROR! Failed to create Reader Task  "
					 "RING_IO application\n") ;
					 #endif
					 }
					 }
					 else{
					 #ifdef RING_IO_MULTIPROCESS
					 RING_IO_0Print ("ERROR! Failed to create Writer Client "
					 "Process in RING_IO application\n") ;

					 #else
					 RING_IO_0Print ("ERROR! Failed to create Writer Task  "
					 "RING_IO application\n") ;
					 #endif
					 }

					 }
					 else{
					 #ifdef RING_IO_MULTIPROCESS
					 RING_IO_0Print ("ERROR! Failed to create Writer Client "
					 "Process in RING_IO application\n") ;

					 #else
					 RING_IO_0Print ("ERROR! Failed to create Writer Task  "
					 "RING_IO application\n") ;
					 #endif
					 }*/
				}
				else {
#ifdef RING_IO_MULTIPROCESS
					RING_IO_0Print ("ERROR! Failed to create Writer Client "
							"Process in RING_IO application\n");

#else
					RING_IO_0Print ("ERROR! Failed to create Writer Task  "
							"RING_IO application\n");
#endif
				}
			}

			/*
			 if (DSP_SUCCEEDED (status)) {
			 writerClientInfo.processorId = processorId ;
			 status = RING_IO_Create_client(&writerClientInfo,
			 (Pvoid)RING_IO_WriterClient, NULL) ;
			 if (status == DSP_SOK) {
			 readerClientInfo.processorId = processorId ;
			 status = RING_IO_Create_client(&readerClientInfo,
			 (Pvoid)RING_IO_ReaderClient,
			 NULL) ;
			 if (status != DSP_SOK) {
			 #ifdef RING_IO_MULTIPROCESS
			 RING_IO_0Print ("ERROR! Failed to create Reader Client "
			 "Process in RING_IO application\n") ;

			 #else
			 RING_IO_0Print ("ERROR! Failed to create Reader Task  "
			 "RING_IO application\n") ;
			 #endif
			 }

			 }
			 else {
			 #ifdef RING_IO_MULTIPROCESS
			 RING_IO_0Print ("ERROR! Failed to create Writer Client "
			 "Process in RING_IO application\n") ;

			 #else
			 RING_IO_0Print ("ERROR! Failed to create Writer Task  "
			 "RING_IO application\n") ;
			 #endif
			 }
			 }
			 */

			if (DSP_SUCCEEDED (status)) {
				/* Wait for the threads/process to  terminate*/
				RING_IO_Join_client (&writerClientInfo1);
				RING_IO_Join_client (&writerClientInfo2);
				//   RING_IO_Join_client (&readerClientInfo1) ;
				//   RING_IO_Join_client (&readerClientInfo2);

			}

			/*
			 *  Perform cleanup operation.
			 */
			RING_IO_Delete (processorId);
		}
	}
	else {
		status = DSP_EINVALIDARG;
		RING_IO_0Print ("ERROR! Invalid arguments specified for  "
				"RING_IO application\n");
	}

	RING_IO_0Print ("====================================================\n");
}

/** ----------------------------------------------------------------------------
 *  @func   RING_IO_Reader_VerifyData
 *
 *  @desc   This function verifies the data-integrity of given buffer.
 *
 *  @modif  None
 *  ----------------------------------------------------------------------------
 */
STATIC
NORMAL_API
DSP_STATUS
RING_IO_Reader_VerifyData (IN Void * buffer,
		//IN Uint32 factor,
		//IN Uint32 action,
		IN Uint32 size)
{
	DSP_STATUS status = DSP_SOK;
	Char8 * ptr8 = (Char8 *) (buffer);
	//Uint16 *   ptr16  = (Uint16 *) (buffer) ;
	Int16 i;
	for (i = 0;
			DSP_SUCCEEDED (status) && (i < 20 );
			i++) {

		RING_IO_1Print ("    Received [0x%x]  ", *ptr8);

	}

	RING_IO_0Print ("\n");

	/*
	 *  Verify the data
	 */

	/*
	 for (i = 0 ;
	 DSP_SUCCEEDED (status) && (i < size / DSP_MAUSIZE) ;
	 i++) {

	 if (DSP_MAUSIZE == 1) {
	 if (action == OP_MULTIPLY) {
	 if (*ptr8 != (Char8) (XFER_VALUE * factor)) {
	 RING_IO_0Print ("ERROR! Data integrity check failed\n") ;
	 RING_IO_1Print ("    Expected [0x%x]\n",
	 (XFER_VALUE * factor)) ;
	 RING_IO_1Print ("    Received [0x%x]\n", *ptr8) ;
	 status = DSP_EFAIL ;
	 }
	 }
	 else if (action ==  OP_DIVIDE) { */
	/* OP_DIVIDE */
	/*           if (*ptr8 != (Char8) (XFER_VALUE / factor)) {
	 RING_IO_0Print ("ERROR! Data integrity check failed\n") ;
	 RING_IO_1Print ("    Expected [0x%x]\n",
	 (XFER_VALUE / factor)) ;
	 RING_IO_1Print ("    Received [0x%x]\n", *ptr8) ;
	 status = DSP_EFAIL ;
	 }

	 }
	 else {
	 RING_IO_0Print ("ERROR! Data integrity check failed\n") ;
	 }

	 ptr8++ ;
	 }
	 else if (DSP_MAUSIZE == 2) {
	 if (action == OP_MULTIPLY) {
	 if (*ptr16 != (Uint16) (XFER_VALUE * factor)) {
	 RING_IO_0Print ("ERROR! Data integrity check failed\n") ;
	 RING_IO_1Print ("    Expected [0x%x]\n",
	 (XFER_VALUE * factor)) ;
	 RING_IO_1Print ("    Received [0x%x]\n", *ptr16) ;
	 status = DSP_EFAIL ;
	 }
	 }
	 else if (action ==  OP_DIVIDE){
	 if (*ptr16 != (Uint16) (XFER_VALUE /factor)) {
	 RING_IO_0Print ("ERROR! Data integrity check failed\n") ;
	 RING_IO_1Print ("    Expected [0x%x]\n",
	 (XFER_VALUE /factor)) ;
	 RING_IO_1Print ("    Received [0x%x]\n", *ptr16) ;
	 status = DSP_EFAIL ;
	 }
	 }
	 else {
	 RING_IO_0Print ("ERROR! Data integrity check failed\n") ;
	 }
	 ptr16++ ;
	 }
	 }*/

	return (status);
}

/** ----------------------------------------------------------------------------
 *  @func   RING_IO_InitBuffer
 *
 *  @desc   This function initializes the specified buffer with valid data.
 *  @filed  buffer
 *              Pointer to the buffer.
 *  @filed  size
 *              Size of the  buffer.
 *  @modif  None
 *  ----------------------------------------------------------------------------
 */
STATIC
NORMAL_API
Void
RING_IO_InitBuffer (IN Void * buffer, Uint32 size)
{
	Uint8 * ptr8 = (Uint8 *) (buffer);
	Uint16 * ptr16 = (Uint16 *) (buffer);
	Int16 i;

	if (buffer != NULL) {
		for (i = 0; i < (size ); i++) {

			*ptr8 = XFER_VALUE;
			ptr8++;

		}
	}
}

/** ----------------------------------------------------------------------------
 *  @func   RING_IO_Writer_Notify
 *
 *  @desc   This function implements the notification callback for the RingIO
 *          opened by the GPP in writer  mode.
 *
 *  @modif  None
 *  ----------------------------------------------------------------------------
 */
STATIC
NORMAL_API
Void
RING_IO_Writer_Notify1(IN RingIO_Handle handle,
		IN RingIO_NotifyParam param,
		IN RingIO_NotifyMsg msg)
{
	DSP_STATUS status = DSP_SOK;

	/* Post the semaphore. */
	status = RING_IO_PostSem ((Pvoid) param);
	if (DSP_FAILED (status)) {
		RING_IO_1Print ("RING_IO_PostSem () failed. Status = [0x%x]\n",
				status);
	}
	RING_IO_0Print (" RING_IO_Writer_Notify1  Scuccess \n");

}

STATIC
NORMAL_API
Void
RING_IO_Writer_Notify2(IN RingIO_Handle handle,
		IN RingIO_NotifyParam param,
		IN RingIO_NotifyMsg msg)
{
	DSP_STATUS status = DSP_SOK;

	/* Post the semaphore. */
	status = RING_IO_PostSem ((Pvoid) param);
	if (DSP_FAILED (status)) {
		RING_IO_1Print ("RING_IO_PostSem () failed. Status = [0x%x]\n",
				status);
	}
	RING_IO_0Print (" RING_IO_Writer_Notify2  Scuccess \n");

}

/** ----------------------------------------------------------------------------
 *  @func   RING_IO_Reader_Notify
 *
 *  @desc   This function implements the notification callback for the RingIO
 *          opened by the GPP in  reader mode.
 *
 *  @modif  None
 *  ----------------------------------------------------------------------------
 */
STATIC
NORMAL_API
Void
RING_IO_Reader_Notify1 (IN RingIO_Handle handle,
		IN RingIO_NotifyParam param,
		IN RingIO_NotifyMsg msg)
{
	DSP_STATUS status = DSP_SOK;

	switch(msg) {
		case NOTIFY_DATA_START:
		fReaderStart1 = TRUE;
		RING_IO_0Print (" RING_IO_Reader_Notify1 Start Scuccess \n");
		break;

		case NOTIFY_DATA_END:
		fReaderEnd1 = TRUE;
		RING_IO_0Print (" RING_IO_Reader_Notify1 End Scuccess \n");
		break;

		default:
		break;
	}

	/* Post the semaphore. */
	status = RING_IO_PostSem ((Pvoid) param);
	if (DSP_FAILED (status)) {
		RING_IO_1Print ("RING_IO_PostSem () failed. Status = [0x%x]\n",
				status);
	}
}

STATIC
NORMAL_API
Void
RING_IO_Reader_Notify2 (IN RingIO_Handle handle,
		IN RingIO_NotifyParam param,
		IN RingIO_NotifyMsg msg)
{
	DSP_STATUS status = DSP_SOK;
	RING_IO_1Print ("###RING_IO_Reader_Notify2. (msg) = %d\n",
				msg);

	switch(msg) {
		case NOTIFY_DATA_START:
		fReaderStart2 = TRUE;
		RING_IO_0Print (" RING_IO_Reader_Notify2 Start Scuccess \n");
		/* Post the semaphore. */
		status = RING_IO_PostSem ((Pvoid) param);
		break;

		case NOTIFY_DATA_END:
		fReaderEnd2 = TRUE;
		RING_IO_0Print (" RING_IO_Reader_Notify2 End Scuccess \n");
		/* Post the semaphore. */
		status = RING_IO_PostSem ((Pvoid) param);
		break;

		default:
		break;
	}

	
	if (DSP_FAILED (status)) {
		RING_IO_1Print ("RING_IO_PostSem () failed. Status = [0x%x]\n",
				status);
	}
}

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
