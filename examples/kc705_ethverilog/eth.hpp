#ifndef ETH_H
#define ETH_H
#include <semaphore.h>
#include <pthread.h>

class EthRequestProxy;
class EthIndication;
// 
 class Eth {
   public:
     Eth();
//     ~Eth();
//     // void reset();
     void read(unsigned long offset, uint8_t *buf);
     void write(unsigned long offset, const uint8_t *buf);
//
   private:
     EthRequestProxy ethRequest;
//     EthIndication *indication;
     sem_t sem;
     uint32_t data;
     pthread_mutex_t mu;  

 };

#endif
