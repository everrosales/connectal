/* Copyright (c) 2014 Quanta Research Cambridge, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "StdDmaIndication.h"
#include "MMUConfigRequestProxy.h"
#include "GeneratedTypes.h" 
#include "NandSimIndicationWrapper.h"
#include "NandSimRequestProxy.h"

static int trace_memory = 1;
extern "C" {
#include "userReference.h"
}

using namespace std;

class NandSimIndication : public NandSimIndicationWrapper
{
public:
  unsigned int rDataCnt;
  virtual void readDone(uint32_t v){
    fprintf(stderr, "NandSim::readDone v=%x\n", v);
    sem_post(&sem);
  }
  virtual void writeDone(uint32_t v){
    fprintf(stderr, "NandSim::writeDone v=%x\n", v);
    sem_post(&sem);
  }
  virtual void eraseDone(uint32_t v){
    fprintf(stderr, "NandSim::eraseDone v=%x\n", v);
    sem_post(&sem);
  }
  virtual void configureNandDone(){
    fprintf(stderr, "NandSim::configureNandDone\n");
    sem_post(&sem);
  }

  NandSimIndication(int id) : NandSimIndicationWrapper(id) {
    sem_init(&sem, 0, 0);
  }
  void wait() {
    fprintf(stderr, "NandSim::wait for semaphore\n");
    sem_wait(&sem);
  }
private:
  sem_t sem;
};

static int sockfd = -1;
#define SOCK_NAME "socket_for_nandsim"
void connect_to_algo_exe(void)
{
  int connect_attempts = 0;

  if (sockfd != -1)
    return;
  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "%s (%s) socket error %s\n",__FUNCTION__, SOCK_NAME, strerror(errno));
    exit(1);
  }

  //fprintf(stderr, "%s (%s) trying to connect...\n",__FUNCTION__, SOCK_NAME);
  struct sockaddr_un local;
  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, SOCK_NAME);
  while (connect(sockfd, (struct sockaddr *)&local, strlen(local.sun_path) + sizeof(local.sun_family)) == -1) {
    if(connect_attempts++ > 100){
      fprintf(stderr,"%s (%s) connect error %s\n",__FUNCTION__, SOCK_NAME, strerror(errno));
      exit(1);
    }
    fprintf(stderr, "%s (%s) retrying connection\n",__FUNCTION__, SOCK_NAME);
    sleep(5);
  }
  fprintf(stderr, "%s (%s) connected\n",__FUNCTION__, SOCK_NAME);
}


void write_to_algo_exe(unsigned int x)
{
  if (send(sockfd, &x, sizeof(x), 0) == -1) {
    fprintf(stderr, "%s send error\n",__FUNCTION__);
    exit(1);
  }
}


int main(int argc, const char **argv)
{

#ifndef BOARD_bluesim
  size_t nandBytes = 1 << 12;
#else
  size_t nandBytes = 1 << 18;
#endif

  fprintf(stderr, "Main::%s %s\n", __DATE__, __TIME__);

  MMUConfigRequestProxy *hostMMUConfigRequest = new MMUConfigRequestProxy(IfcNames_BackingStoreMMUConfigRequest);
  DmaManager *hostDma = new DmaManager(NULL, hostMMUConfigRequest);
  MMUConfigIndication *hostMMUConfigIndication = new MMUConfigIndication(hostDma, IfcNames_BackingStoreMMUConfigIndication);

  NandSimRequestProxy *nandsimRequest = new NandSimRequestProxy(IfcNames_NandSimRequest);
  NandSimIndication *nandsimIndication = new NandSimIndication(IfcNames_NandSimIndication);

  portalExec_start();

  int nandAlloc = portalAlloc(nandBytes);
  fprintf(stderr, "nandAlloc=%d\n", nandAlloc);
  int ref_nandAlloc = hostDma->reference(nandAlloc);
  fprintf(stderr, "ref_nandAlloc=%d\n", ref_nandAlloc);
#ifdef SANITY0
  unsigned int *nandBuffer = (unsigned int*)portalMmap(nandAlloc, nandBytes); 
  fprintf(stderr, "nandBuffer=%p\n", nandBuffer);
  portalDCacheFlushInval(nandAlloc, nandBytes, nandBuffer);
#endif
  fprintf(stderr, "NAND alloc fd=%d ref=%d\n", nandAlloc, ref_nandAlloc);
  nandsimRequest->configureNand(ref_nandAlloc, nandBytes);
  nandsimIndication->wait();

#ifndef ALGO1_NANDSIM
  if (argc == 1) {

    fprintf(stderr, "Main::allocating memory...\n");
    size_t srcBytes = nandBytes>>2;
    int srcAlloc = portalAlloc(srcBytes);
    unsigned int *srcBuffer = (unsigned int *)portalMmap(srcAlloc, srcBytes);
    unsigned int ref_srcAlloc = hostDma->reference(srcAlloc);
    fprintf(stderr, "fd=%d, srcBuffer=%p\n", srcAlloc, srcBuffer);

    /* do tests */
    fprintf(stderr, "chamdoo-test\n");
    unsigned long loop = 0;
    unsigned long match = 0, mismatch = 0;

    while (loop < nandBytes) {

      fprintf(stderr, "Main::starting write ref=%d, len=%08zx (%lu)\n", ref_srcAlloc, srcBytes, loop);
      for (int i = 0; i < srcBytes/sizeof(srcBuffer[0]); i++) {
	srcBuffer[i] = loop+i;
      }
      portalDCacheFlushInval(srcAlloc, srcBytes, srcBuffer);
      nandsimRequest->startWrite(ref_srcAlloc, 0, loop, srcBytes, 16);
      nandsimIndication->wait();
      loop+=srcBytes;
    }
    fprintf(stderr, "Main:: write phase complete\n");

#ifdef SANITY1
    // see what was written to the backing store...
    int mmc = 0;
    loop = 0;
    int j = 0;
    while (loop < nandBytes) {
      for (int i = 0; i < srcBytes/sizeof(srcBuffer[0]); i++){
	if (nandBuffer[j] != loop+i){
	  mmc++;
	  fprintf(stderr, "Main::sanity failed %d %d\n", i, nandBuffer[j]);
	}
	j++;
      }
      loop += srcBytes;
    }
    fprintf(stderr, "Main::sanity complete %d\n", mmc);
#endif

    loop = 0;
    while (loop < nandBytes) {
      fprintf(stderr, "Main::starting read %08zx (%lu)\n", srcBytes, loop);

      for (int i = 0; i < srcBytes/sizeof(srcBuffer[0]); i++) {
	srcBuffer[i] = 5;
      }

      portalDCacheFlushInval(srcAlloc, srcBytes, srcBuffer);
      nandsimRequest->startRead(ref_srcAlloc, 0, loop, srcBytes, 16);
      nandsimIndication->wait();
      
      for (int i = 0; i < srcBytes/sizeof(srcBuffer[0]); i++) {
	if (srcBuffer[i] != loop+i) {
	  fprintf(stderr, "Main::mismatch [%08ld] != [%08d] (%d,%zu)\n", loop+i, srcBuffer[i], i, srcBytes/sizeof(srcBuffer[0]));
	  mismatch++;
	} else {
	  match++;
	}
      }
      
      loop+=srcBytes;
    }
    /* end */
    
    uint64_t beats_r = hostDma->show_mem_stats(ChannelType_Read);
    uint64_t beats_w = hostDma->show_mem_stats(ChannelType_Write);

    fprintf(stderr, "Main::Summary: match=%lu mismatch:%lu (%lu) (%f percent)\n", match, mismatch, match+mismatch, (float)mismatch/(float)(match+mismatch)*100.0);
    fprintf(stderr, "(%"PRIx64", %"PRIx64")\n", beats_r, beats_w);
    
    return (mismatch > 0);
  } else
#endif
  {

    // else we were invoked by alg1_nandsim
    const char *filename = "../haystack.txt";
    fprintf(stderr, "Main::opening %s\n", filename);
    // open up the text file and read it into an allocated memory buffer
    int dataFile = open(filename, O_RDONLY);
    uint32_t data_len = lseek(dataFile, 0, SEEK_END);
    lseek(dataFile, 0, SEEK_SET);
    int dataAlloc = portalAlloc(data_len);
    int ref_dataAlloc = hostDma->reference(dataAlloc);
    char *data = (char *)portalMmap(dataAlloc, data_len);
    if(read(dataFile, data, data_len) != data_len) {
      fprintf(stderr, "error reading %s %d\n", filename, (int)data_len);
      exit(-1);
    }

    // write the contents of data into "flash" memory
    portalDCacheFlushInval(ref_dataAlloc, data_len, data);
    nandsimRequest->startWrite(ref_dataAlloc, 0, 0, data_len, 16);
    nandsimIndication->wait();

#ifdef SANITY2
    // see what was written to the backing store...
    for(int i = 0; i < data_len; i++)
      fprintf(stderr, "%c", ((char*)nandBuffer)[i]);
    fprintf(stderr, "\n");
#endif

    fprintf(stderr, "Main::connecting to algo_exe...\n");
    connect_to_algo_exe();
    fprintf(stderr, "Main::connected to algo_exe\n");

    // send the offset and length (in nandsim) of the text
    write_to_algo_exe(0);
    write_to_algo_exe(data_len);
    printf("[%s:%d] sleep, waiting for search\n", __FUNCTION__, __LINE__);
    sleep(200);
    printf("[%s:%d] now closing down\n", __FUNCTION__, __LINE__);
  }
}
