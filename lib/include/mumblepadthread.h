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

#ifndef __MUMBLEPADTHREAD_H
#define __MUMBLEPADTHREAD_H

#include "mumrenderer.h"
#include <thread>
#include "signal.h"


typedef enum EMumJobState {
    MUM_JOB_STATE_NONE = -1,
    MUM_JOB_STATE_DONE = 0,
    MUM_JOB_STATE_ASSIGNED = 1,
    MUM_JOB_STATE_WORKING = 2,
} EMumJobState;

typedef enum EMumJobType {
    MUM_JOB_TYPE_ENCRYPT = 0,
    MUM_JOB_TYPE_DECRYPT = 1,
} EMumJobType;

typedef struct TMumJob
{
    EMumJobState state;
    EMumJobType type;
    uint8_t *src;
    uint8_t *dst;
    uint32_t length;
    uint32_t outlength;
    uint16_t seqNum;
} TMumRenderJob;


class CMumblepadThread : public CMumRenderer {
public:
    CMumblepadThread(TMumInfo *mumInfo, uint32_t id, CSignal * serverSignal);
    ~CMumblepadThread();
    virtual void EncryptDiffuse(uint32_t round);
    virtual void EncryptConfuse(uint32_t round);
    virtual void DecryptConfuse(uint32_t round);
    virtual void DecryptDiffuse(uint32_t round);
    virtual void EncryptUpload(uint8_t *data);
    virtual void EncryptDownload(uint8_t *data);
    virtual void DecryptUpload(uint8_t *data);
    virtual void DecryptDownload(uint8_t *data);
    virtual void InitKey();
    uint8_t mPingPongBlock[2][MUM_MAX_BLOCK_SIZE];
    uint32_t mId;
    TMumJob mJob;
    std::thread * mThreadHandle;
    CSignal * mWorkerSignal;
    CSignal * mServerSignal;
    //uint32_t mThreadID;
    bool mRunning;
    uint32_t mEncryptLength;
    uint32_t mDecryptLength;
    void Run();
    // void Start();
    void Stop();
};




#endif