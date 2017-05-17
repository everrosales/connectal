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
      this->data = data;
      sem_post(&sem);
    }

    void reset() {
      ethRequest.reset();
      sem_wait(&sem);
    } 

    void resetDone() {
      sem_post(&sem);
    }
    
   /*
    * Ethernet convinence methods
    *
    * sendHelloWorldPacket
    *   Used for 
    */
   void sendHelloWorldPacket(uint8_t *mac) {
     tx_packet pkt = {
        .dst_mac_addr = {mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]},
        .src_mac_addr = {0x00, 0x00, 0x5E, 0x00, 0xFA, 0xCE},
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
     uint32_t ctl = AXIETH_TCTL_STATUS;
     write(AXIETH_TCTL, ctl);
   }

   void setTxLen(int len) {
    write(AXIETH_TDL, uint32_t(len));
   }

   void axi_eth_init(int phy_id) {
     // Disable interrupts 
     write(AXIETH_MDIO_CTRL, AXIETH_MDIOCTRL_EN_MASK);
    
      //  Dont advertise 1000BASE_T Full/Half duplex speeds
     if (axi_eth_mdio_write(7, MII_CTRL1000, 0) < 0) {
       printf("Failed writing miictrl100\n");
     }

     //  Advertise only 10 and 100mbps full/half duplex speeds
     if (axi_eth_mdio_write(7, MII_ADVERTISE, ADVERTISE_ALL | ADVERTISE_CSMA) < 0) {
       printf("Failed to advertise\n");
     }
  
     // Restart auto negotiation
     uint32_t bmcr = axi_eth_mdio_read(7, MII_BMCR);
     bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);
     axi_eth_mdio_write(7, MII_BMCR, bmcr);
   
     // Spin until init mdio signal sent
     if (axi_eth_mdio_wait() < 0)
       printf("[WARN]: MDIO write timeout\n");


     // TODO: Wait until renegotiation has completed
  }


   //
   // MDIO functions
   //
   int axi_eth_mdio_wait() {
     int timeout = 50000;
     while (read(AXIETH_MDIO_CTRL) & AXIETH_MDIOCTRL_STATUS_MASK) {
       if (timeout-- < 0) {
         printf("Timeout exceeded\n");
         return -1;
       }
     }

     return 0;
   }   

   int axi_eth_mdio_read(int phy_id, int reg) {
     if (axi_eth_mdio_wait()) {
       // Err
       printf("FAILED READ\n");
       return -1;  
     }

     /* Write the PHY address, register number and set hte OP bit in the
      * MDIO address register. Set the Status bit in the MDIO Control
      * register to start a MDIO read transaction
      */ 
     uint32_t ctrl_reg = read(AXIETH_MDIO_CTRL);
     write(AXIETH_MDIO_ADDR, AXIETH_MDIO_OP_MASK | ((phy_id << AXIETH_MDIO_PHYADDR_SHIFT) | reg));
     write(AXIETH_MDIO_CTRL, ctrl_reg | AXIETH_MDIOCTRL_STATUS_MASK);

     if (axi_eth_mdio_wait()) {
       printf("ALSO FAILED READ\n");
       return -1;
     }

     uint32_t rdata = read(AXIETH_MDIO_RD);
     return rdata;
   }


   int axi_eth_mdio_write(int phy_id, int reg, uint16_t val) {
     if (axi_eth_mdio_wait()) {
       printf("Failed MDIO write wait\n");
       return -1;
     }    

     uint32_t ctrl_reg = read(AXIETH_MDIO_CTRL);
     write(AXIETH_MDIO_ADDR, (~AXIETH_MDIO_OP_MASK) & ((phy_id << AXIETH_MDIO_PHYADDR_SHIFT) | reg));
     write(AXIETH_MDIO_WR, val);
     write(AXIETH_MDIO_CTRL, ctrl_reg | AXIETH_MDIOCTRL_STATUS_MASK);
     return 0; 
   }


   void sendPacket(tx_packet *pkt, int len) {
      // Basic strategy to write a packet
      // 1) Write destination mac [0 - 5]     len: 6
      // 2) Write source mac      [6 - 11]    len: 6
      // 3) Write data length     [12 - 13]   len: 2
      // 4) Write data
      
      // Write packet to relevant buffer
      uint32_t ptr = 0x0;
      for (int i=0; i < len / 4; i++) {
        // write out every part of the packet up to the length
        //printf("getting ready to write, addr: %x, data: %x\n", ptr + (4*i), ((uint32_t *)pkt)[i]);
        write(ptr + (4*i), ((uint32_t*) pkt)[i]);
      }
      // Set the length of the packet as specified by the IP core
      setTxLen(len);
      
      // Signal Transmit ready
      setTxReady();
      
      // Spin until transfer is ready for another packet
      bool sending = true;
      while(sending) {
        read(AXIETH_TCTL);
        sending = ((data & AXIETH_TCTL_STATUS) == 0x01);
      }
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
    }

    printf("\n\nSuccess!\n\n");
   }
};


int main(int argc, char* argv[]) {
  EthIndication ethIndication(IfcNames_EthIndicationH2S, IfcNames_EthRequestS2H);
  
  ethIndication.axi_eth_init(7);
  printf("finished setting the flags and stuff\n");

  uint8_t mac[6] = {0x14, 0x91, 0x82, 0x3b, 0xbc, 0x0a};

  sleep(5);

  for (int i = 0; i < 100; i++) {
    ethIndication.sendHelloWorldPacket(mac);
    //`sleep(1);
  }  

 
}
