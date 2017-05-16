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

    uint32_t read(uint32_t addr) {
      pthread_mutex_lock(&mu);
      ethRequest.request(0x00, addr, (uint32_t) 0x00);
      sem_wait(&sem);
      pthread_mutex_unlock(&mu);
      return data;
    }

    // Indication call back methods
    void response(int write, uint32_t data) {
      //`printf("Got back: %x, %x\n", write, data);
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
        .dst_mac_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .src_mac_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .type_length = 0x1234
      };
     pkt.data[0] = 0xDE;
     pkt.data[1] = 0xAD;
     pkt.data[2] = 0xBE;
     pkt.data[3] = 0xEF;
     pkt.data[4] = 0xFF;
     sendPacket(&pkt, 22);
   };

   void setTxReady() {
     //read(AXIETH_TCTL);
     uint32_t ctl = AXIETH_TCTL_STATUS;
     write(AXIETH_TCTL, ctl);
   }

   void setTxLen(int len) {
    write(AXIETH_TDL, uint32_t(len));
   }

   void axi_eth_init() {
   
   }


   void sendPacket(tx_packet *pkt, int len) {
      // Basic strategy to write a packet
      // 1) Write destination mac [0 - 5]     len: 6
      // 2) Write source mac      [6 - 11]    len: 6
      // 3) Write data length     [12 - 13]   len: 2
      // 4) Write data
      //uint32_t ptr = 0x0;
      //for (int i=0; i < len / 4; i++) {
      //  // write out every part of the packet up to the length
      //   printf("getting ready to write, addr: %x, data: %x\n", ptr, ((uint32_t *)pkt)[i]);
      //   write(ptr, ((uint32_t*) pkt)[i]);
      //   printf("finished writing\n");
      // }
      uint32_t *fake_pkt = (uint32_t *) pkt;
      write(0x0, fake_pkt[0]);
      //ethRequest.request(0x04, fake_pkt[1]);
      write(0x4, fake_pkt[1]);
      write(0x8, fake_pkt[2]);
      write(0xC, fake_pkt[3]);
      write(0x10, fake_pkt[4]);
      setTxLen(len);
      
      printf("0x0:   %x\n", read(0x0));
      printf("0x4:   %x\n", read(0x4));
      printf("0x8:   %x\n", read(0x8));
      printf("0xC:   %x\n", read(0xC));
      printf("0x10:  %x\n", read(0x10));

      printf("setting ready\n");
      setTxReady();
      printf("set ready\n");
      bool sending = true;
      while(sending) {
        read(AXIETH_TCTL);
        sending = ((data & AXIETH_TCTL_STATUS) == 0x01);

        printf("Looping....\n");
      }
      printf("Sent packet\n");
   }

   void readPacket() {
    // Read the status bit until its set
    // Read the data, clear it
    // set the status bit
    printf("0x1000:   %x\n", read(0x1000));
    printf("0x1004:   %x\n", read(0x1004));
    printf("0x1008:   %x\n", read(0x1008));
    printf("0x100C:   %x\n", read(0x100C));
    printf("0x1010:   %x\n", read(0x1010));
    printf("0x1014:   %x\n", read(0x1014));
    
    printf("0x%x:    %x\n", AXIETH_RCTL, read(AXIETH_RCTL));
    write(AXIETH_RCTL, 0);
    uint32_t present = 0;
    while (!present) {
      present = read(AXIETH_RCTL);
      // printf("... waiting for packet ... \n");
    }

    printf("\n\nSuccess!\n\n");
    


   }

};


int main(int argc, char* argv[]) {
  // Eth eth = new Eth();
  EthIndication ethIndication(IfcNames_EthIndicationH2S, IfcNames_EthRequestS2H);
  printf("doing the thing here\n");
  //mac_address mac;
  uint8_t mac[6] = {0x0a, 0xbc, 0x3b, 0x82, 0x91, 0x14};

  for (int i = 0; i < 100; i++) {
    ethIndication.sendHelloWorldPacket(mac);
  }  

 

  //ethIndication.sendHelloWorldPacket(mac);
  //ethIndication.write(AXIETH_TDL, uint32_t(0xdeadbeef));
 

  //for (int i = 0; i < 100; i++) {
  //  printf("Reading #%d: %x\n", i, ethIndication.read(AXIETH_TCL));
  //}
 
  //ethIndication.sendHelloWorldPacket(mac);
  //uint32_t data = ethIndication.read(AXIETH_TDL);
  //printf("This is : %x\n", data);
  //ethIndication.readPacket();
  
  //printf("Sent the first packet\n");
  //ethIndication.sendHelloWorldPacket(mac);
  //printf("Sent the second packet\n");

  //ethIndication.sendHelloWorldPacket(mac);
  //printf("Sent the third packet\n");

  
//  ethIndication.write(32, 100);
//  ethIndication.read(32);
}
