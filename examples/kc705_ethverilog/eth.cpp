#include "EthIndication.h"
#include "EthRequest.h"
#include "GeneratedTypes.h"
#include "eth.hpp"

bool verbose = 0;

class EthIndication: public EthIndicationWrapper {
  private:
    EthRequestProxy ethRequest;
    sem_t sem;
    uint32_t data;
    pthread_mutex_t mu;

  public:
    EthIndication (unsigned int indicationId, unsigned int requestId)
          : EthIndicationWrapper(indicationId),
          ethRequest(requestId)
    {
        sem_init(&sem, 1, 0);
        pthread_mutex_init(&mu, 0);
    }

    // Request wrappers

    void write(uint32_t addr, uint32_t data) {
      pthread_mutex_lock(&mu);
      ethRequest.request(0x0F, addr, data);
      sem_wait(&sem);
      pthread_mutex_unlock(&mu);
    }

    void read(uint32_t addr) {
      pthread_mutex_lock(&mu);
      ethRequest.request(0x00, addr, (uint32_t) 0x00);
      sem_wait(&sem);
      pthread_mutex_unlock(&mu);
    }

    // Indication call back methods
    void response(int write, uint32_t data) {
      printf("Got back: %x, %x\n", write, data);
      sem_post(&sem); 
      this->data = data;
    }

    void resetDone() {
    }
    
};

// class Eth {
//   private:
//     EthIndication *ethIndication;
//     bool verbose;
//
//   public:
//     Eth(EthIndication* ) {
//       verbose = true;
//     }
// }

int main(int argc, char* argv[]) {
  // Eth eth = new Eth();
  EthIndication ethIndication(IfcNames_EthIndicationH2S, IfcNames_EthRequestS2H);
  printf("doing the thing here\n");

  ethIndication.write(32, 100);
  ethIndication.read(32);
}
