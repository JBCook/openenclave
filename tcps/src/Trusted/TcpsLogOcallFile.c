/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */
#include <openenclave/enclave.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */

#ifdef OE_USE_OPTEE
#include "tcps_time_t.h"
#include "tcps_string_t.h"
#include "oeoverintelsgx_t.h"
#include <optee/tcps_string_optee_t.h>
#else
#include "oeoverintelsgx_t.h"
#endif

#include "TcpsLog.h"
#include "TcpsLogOcallFile.h"
#include <stdio.h>
#include "enclavelibc.h"

static
char*
TcpsLogFilenameFormatOcall(
    const char* const Prefix,
    const TCPS_LOG_FILE_TYPE Type,
    const TCPS_IDENTITY_LOG IdentityLog)
{
    static char path[255];
    const char* typeLabel;
    int result;

    switch (Type)
    {
    case TCPS_LOG_FILE_TYPE_LOG:
        typeLabel = "log";
        break;

    case TCPS_LOG_FILE_TYPE_SIG:
        typeLabel = "sig";
        break;

    default:
        return NULL;
    }
    result = snprintf(
        path, 
        sizeof(path), 
        "%s_%s_%s.dat", 
        Prefix, 
        typeLabel, 
        IdentityLog);
    if (result < 0 || result >= (int)sizeof(path))
    {
        return NULL;
    }

    return path;
}

oe_result_t
TcpsLogFileWriteOcall(
    TCPS_LOG_OCALL_OBJECT* Context,
    const uint8_t* const Buffer,
    const size_t BufferSize,
    const TCPS_LOG_FILE_TYPE FileType,
    bool Append,
    const TCPS_IDENTITY_LOG LogIdentityLabel)
{
    oe_buffer256 filenameBuffer;
    oe_buffer4096* content;

Tcps_InitializeStatus(Tcps_Module_Helper_t, "TcpsLogFileWriteOcall");

    Tcps_ReturnErrorIfArgumentNull(Context);
    Tcps_ReturnErrorIfArgumentNull(Buffer);
    Tcps_ReturnErrorIfArgumentNull(LogIdentityLabel);

    unsigned int result = 1;
    const char* filename = TcpsLogFilenameFormatOcall(
        Context->LogPathPrefix, FileType, LogIdentityLabel);
    Tcps_ReturnErrorIfTrue(filename == NULL, OE_FAILURE);

    COPY_BUFFER_FROM_STRING(filenameBuffer, filename);

    content = (oe_buffer4096*)oe_malloc(sizeof(*content));
    Tcps_GotoErrorIfAllocFailed(content);

    COPY_BUFFER(*content, Buffer, BufferSize);

    sgx_status_t sgxResult = ocall_ExportFile(&result, filenameBuffer, Append, *content, BufferSize);

    oe_free(content);

    Tcps_GotoErrorIfTrue(sgxResult != SGX_SUCCESS, OE_FAILURE);
    Tcps_GotoErrorIfTrue(result != 0, OE_FAILURE);

Tcps_ReturnStatusCode;
Tcps_BeginErrorHandling;
Tcps_FinishErrorHandling;
}

oe_result_t
TcpsLogFileWriteEntryOcall(
    TCPS_LOG_OCALL_OBJECT* Context,
    const uint8_t* const Buffer,
    const size_t BufferSize,
    const TCPS_IDENTITY_LOG LogIdentityLabel)
{
    if (Context == NULL ||
        Buffer == NULL ||
        LogIdentityLabel == NULL)
    {
        return OE_INVALID_PARAMETER;
    }

    return TcpsLogFileWriteOcall(
        Context,
        Buffer,
        BufferSize,
        TCPS_LOG_FILE_TYPE_LOG,
        true,
        LogIdentityLabel);
}

oe_result_t
TcpsLogFileReadOcall(
    TCPS_LOG_OCALL_OBJECT* Context,
    uint8_t** const Buffer,
    size_t* const BufferSize,
    const TCPS_LOG_FILE_TYPE FileType,
    const TCPS_IDENTITY_LOG LogIdentityLabel)
{
    oe_buffer256 filenameBuffer;

    if (Buffer == NULL ||
        BufferSize == NULL ||
        Context == NULL ||
        LogIdentityLabel == NULL)
    {
        return OE_INVALID_PARAMETER;
    }

    *Buffer = NULL;
    *BufferSize = 0;

    oe_result_t uStatus = OE_OK;
    GetUntrustedFileSize_Result sizeResult;
    GetUntrustedFileContent_Result contentResult;

    const char* filename = TcpsLogFilenameFormatOcall(
        Context->LogPathPrefix,
        FileType,
        LogIdentityLabel);
    if (!filename)
    {
        return OE_FAILURE;
    }

    COPY_BUFFER_FROM_STRING(filenameBuffer, filename);

    sgx_status_t sgxstatus = ocall_GetUntrustedFileSize(&sizeResult, filenameBuffer);

    Tcps_GotoErrorIfTrue(sgxstatus != SGX_SUCCESS, OE_FAILURE);

    if (sizeResult.status)
    {
        // file not found
        goto Error;
    }

    if (sizeResult.fileSize != 0)
    {
        Tcps_GotoErrorIfTrue(sizeResult.fileSize > sizeof(contentResult.content), OE_FAILURE);

        sgxstatus = ocall_GetUntrustedFileContent(
            &contentResult,
            filenameBuffer,
            sizeResult.fileSize);

        Tcps_GotoErrorIfTrue(sgxstatus != SGX_SUCCESS || contentResult.status, OE_FAILURE);

        *Buffer = oe_malloc(sizeof(sizeResult.fileSize));
        Tcps_GotoErrorIfAllocFailed(*Buffer);
        memcpy(*Buffer, contentResult.content, sizeResult.fileSize);
    }
    *BufferSize = sizeResult.fileSize;

Error:
    if (uStatus != OE_OK)
    {
        if (*Buffer != NULL)
        {
            oe_free(*Buffer);
        }
    }

    return uStatus;
}

oe_result_t
TcpsLogFileClearOcall(
    TCPS_LOG_OCALL_OBJECT* Context,
    const TCPS_IDENTITY_LOG LogIdentityLabel)
{
    oe_result_t status = OE_OK;
    int retVal = 1;

    if (Context == NULL)
    {
        return OE_INVALID_PARAMETER;
    }

    const char* filename = TcpsLogFilenameFormatOcall(
        Context->LogPathPrefix,
        TCPS_LOG_FILE_TYPE_LOG,
        LogIdentityLabel);
    if (!filename)
    {
        return OE_FAILURE;
    }
    oe_buffer256 filenameBuffer;
    COPY_BUFFER_FROM_STRING(filenameBuffer, filename);

    sgx_status_t sgxstatus =
#ifdef OE_USE_OPTEE
        SGX_ERROR_UNEXPECTED;
#else
        ocallTcpsFileDelete(&retVal, filenameBuffer);
#endif

    if (sgxstatus != SGX_SUCCESS || retVal)
    {
        status = OE_FAILURE;
        goto Exit;
    }

Exit:
    return status;
}
