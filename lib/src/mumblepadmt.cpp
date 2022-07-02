//
// MIT License
//
// Copyright (c) 2022 Kyle Granger
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "mumblepadmt.h"
#include "malloc.h"
#include "string.h"
#include "assert.h"
#include "stdio.h"



CMumblepadMt::CMumblepadMt(TMumInfo *mumInfo, uint32_t numThreads) : CMumRenderer(mumInfo)
{
    mMumInfo = mumInfo;
    mNumThreads = numThreads;
    mStarted = false;
    for (int i = 0; i < MUM_MAX_THREADS; i++)
        mThreads[i] = nullptr;
    mServerSignal = new CSignal();
    for (uint32_t i = 0; i < mNumThreads; i++)
        mThreads[i] = new CMumblepadThread(mMumInfo, i + 1, mServerSignal);
    //Start();
    //Sleep(100);
}

CMumblepadMt::~CMumblepadMt()
{
    Stop();
    for (uint32_t i = 0; i < mNumThreads; i++)
        delete mThreads[i];
    delete mServerSignal;
    //CloseHandle(mServerSignal);
}


// void CMumblepadMt::Start()
// {
//     mStarted = true;
//     for (uint32_t i = 0; i < mNumThreads; i++)
//         mThreads[i]->Start();
// }

void CMumblepadMt::Stop()
{
    for (uint32_t i = 0; i < mNumThreads; i++)
        mThreads[i]->Stop();
    //Sleep(100);
    //mStarted = false;
}

EMumError CMumblepadMt::EncryptBlock(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t seqnum)
{
    if (mNumThreads == 0 || mThreads[0] == nullptr)
        return MUM_ERROR_MTRENDERER_NO_THREADS;
    return mThreads[0]->EncryptBlock(src, dst, length, seqnum);
}

EMumError CMumblepadMt::DecryptBlock(uint8_t *src, uint8_t *dst, uint32_t *length, uint32_t *seqnum)
{
    if (mNumThreads == 0 || mThreads[0] == nullptr)
        return MUM_ERROR_MTRENDERER_NO_THREADS;
    return mThreads[0]->DecryptBlock(src, dst, length, seqnum);
}


EMumError CMumblepadMt::Encrypt(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength, uint16_t seqNum)
{
    uint32_t plaintextSize, encryptedSize;

    *outlength = 0;
    for (uint32_t i = 0; i < mNumThreads; i++)
        mThreads[i]->mEncryptLength = 0;

    uint32_t blocksPerJob = MUM_MAX_BYTES_PER_JOB / mMumInfo->plaintextBlockSize;
    while (length > 0)
    {
        TMumJob job;

        // Prepare a job
        //
        if (length >= mMumInfo->plaintextBlockSize * blocksPerJob)
            plaintextSize = mMumInfo->plaintextBlockSize * blocksPerJob;
        else
            plaintextSize = length;
        encryptedSize = plaintextSize * mMumInfo->encryptedBlockSize / mMumInfo->plaintextBlockSize;

        length -= plaintextSize;
        job.state = MUM_JOB_STATE_NONE;
        job.type = MUM_JOB_TYPE_ENCRYPT;
        job.src = src;
        job.dst = dst;
        job.length = plaintextSize;
        job.seqNum = seqNum;

        // Update pointers;
        src += plaintextSize;
        dst += encryptedSize;
        seqNum += (uint16_t) blocksPerJob;

        // Hand off the job
        bool assigned = false;
        while (!assigned)
        {
            for (uint32_t i = 0; i < mNumThreads; i++)
            {
                if (mThreads[i]->mJob.state == MUM_JOB_STATE_DONE)
                {
                    mThreads[i]->mJob = job;
                    mThreads[i]->mJob.state = MUM_JOB_STATE_ASSIGNED;
                    mThreads[i]->mWorkerSignal->DoSignal();
                    assigned = true;
                    break;
                }
            }
            if (!assigned)
            {
                mServerSignal->WaitForSignal();
            }
            else
                break;
        }
    }

    while (true)
    {
        bool working = false;
        for (uint32_t i = 0; i < mNumThreads; i++)
        {
            if (mThreads[i]->mJob.state != MUM_JOB_STATE_DONE)
                working = true;
        }
        if (!working)
            break;
    }
    for (uint32_t i = 0; i < mNumThreads; i++)
        *outlength += mThreads[i]->mEncryptLength;
    return MUM_ERROR_OK;
}

EMumError CMumblepadMt::Decrypt(uint8_t *src, uint8_t *dst, uint32_t length, uint32_t *outlength)
{
    uint32_t plaintextSize, encryptedSize;

    for (uint32_t i = 0; i < mNumThreads; i++)
        mThreads[i]->mDecryptLength = 0;

    *outlength = 0;
    uint32_t blocksPerJob = MUM_MAX_BYTES_PER_JOB / mMumInfo->encryptedBlockSize;
    while (length > 0)
    {
        TMumJob job;

        // Prepare a job
        //
        if (length >= mMumInfo->encryptedBlockSize * blocksPerJob)
            encryptedSize = mMumInfo->encryptedBlockSize * blocksPerJob;
        else
            encryptedSize = length;
        plaintextSize = encryptedSize * mMumInfo->plaintextBlockSize / mMumInfo->encryptedBlockSize;

        length -= encryptedSize;
        job.state = MUM_JOB_STATE_NONE;
        job.type = MUM_JOB_TYPE_DECRYPT;
        job.src = src;
        job.dst = dst;
        job.length = encryptedSize;
        job.seqNum = 0;

        // Update pointers;
        src += encryptedSize;
        dst += plaintextSize;

        // Hand off the job
        bool assigned = false;
        while (!assigned)
        {
            for (uint32_t i = 0; i < mNumThreads; i++)
            {
                if (mThreads[i]->mJob.state == MUM_JOB_STATE_DONE)
                {
                    mThreads[i]->mJob = job;
                    mThreads[i]->mJob.state = MUM_JOB_STATE_ASSIGNED;
                    mThreads[i]->mWorkerSignal->DoSignal();
                    assigned = true;
                    break;
                }
            }
            if (!assigned)
            {
                mServerSignal->WaitForSignal();
            }
            else
                break;
        }
    }

    while (true)
    {
        bool working = false;
        for (uint32_t i = 0; i < mNumThreads; i++)
        {
            if (mThreads[i]->mJob.state != MUM_JOB_STATE_DONE)
                working = true;
        }
        if (!working)
            break;
    }
    for (uint32_t i = 0; i < mNumThreads; i++)
        *outlength += mThreads[i]->mDecryptLength;
    return MUM_ERROR_OK;
}


void CMumblepadMt::EncryptUpload(uint8_t *data)
{
}

void CMumblepadMt::DecryptUpload(uint8_t *data)
{
}

void CMumblepadMt::EncryptDiffuse(uint32_t round)
{
}

void CMumblepadMt::EncryptConfuse(uint32_t round)
{
}

void CMumblepadMt::DecryptConfuse(uint32_t round)
{
}

void CMumblepadMt::DecryptDiffuse(uint32_t round)
{
}

void CMumblepadMt::EncryptDownload(uint8_t *data)
{
}

void CMumblepadMt::DecryptDownload(uint8_t *data)
{
}


