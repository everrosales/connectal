#include "EthIndication.h"
#include "EthRequest.h"
#include "GeneratedTypes.h"
#include "eth.hpp"
#include "axiethlite.hpp"

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

    void reset() {
      ethRequest.reset();
      sem_wait(&sem);
    } 

    void resetDone() {
      sem_post(&sem);
    }
    

   void sendHelloWorldPacket(uint8_t *mac) {
     tx_packet pkt = {
        .dst_mac_addr = {0x14, 0x91, 0x82, 0x3b, 0xbc, 0x0a},
        .src_mac_addr = {0x00, 0x00, 0x5E, 0x00, 0xFA, 0xCE},
        .type_length = 32
      };
     pkt.data[0] = 0xDE;
     pkt.data[1] = 0xAD;
     pkt.data[2] = 0xBE;
     pkt.data[3] = 0xEF;
     sendPacket(&pkt, 18);
   };

   void setTxReady() {
     read(AXIETH_TCTL);
     uint32_t ctl = data | AXIETH_TCTL_STATUS;
     write(AXIETH_TCTL, ctl);
   }

   void sendPacket(tx_packet *pkt, int len) {
      // Basic strategy to write a packet
      // 1) Write destination mac [0 - 5]     len: 6
      // 2) Write source mac      [6 - 11]    len: 6
      // 3) Write data length     [12 - 13]   len: 2
      // 4) Write data
      uint32_t ptr = 0x0;
      for (int i=0; i < len / 4; i++) {
        // write out every part of the packet up to the length
         printf("getting ready to write, addr: %x, data: %x\n", ptr, ((uint32_t *)pkt)[i]);
         write(ptr, ((uint32_t*) pkt)[i]);
         printf("finished writing\n");
      }
      printf("setting ready\n");
      setTxReady();
      printf("set ready\n");
      bool sent = false;
      while(sent) {
        read(AXIETH_TCTL);
        sent = (data & AXIETH_TCTL_STATUS);
        printf("Looping....\n");
      }
      printf("Sent packet\n");
   }

   void readPacket() {
   }

};


int main(int argc, char* argv[]) {
  // Eth eth = new Eth();
  EthIndication ethIndication(IfcNames_EthIndicationH2S, IfcNames_EthRequestS2H);
  printf("doing the thing here\n");
  //mac_address mac;
  uint8_t mac[6] = {0x14, 0x91, 0x82, 0x3b, 0xbc, 0x0a};
  ethIndication.sendHelloWorldPacket(mac);
  

  printf("Sent the first packet\n");
  ethIndication.sendHelloWorldPacket(mac);
  printf("Sent the second packet\n");

  ethIndication.sendHelloWorldPacket(mac);
  printf("Sent the third packet\n");

  
//  ethIndication.write(32, 100);
//  ethIndication.read(32);
}
