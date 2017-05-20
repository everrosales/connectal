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
    bool rx_is_ping;

  public:
    EthIndication (unsigned int indicationId, unsigned int requestId)
          : EthIndicationWrapper(indicationId),
          ethRequest(requestId)
    {
        sem_init(&sem, 1, 0);
        pthread_mutex_init(&mu, 0);
        rx_is_ping = true;
    }

    // Request wrappers
    void write(uint32_t addr, uint32_t data) {
      pthread_mutex_lock(&mu);
      ethRequest.request(0xFF, addr, data);
      sem_wait(&sem);
      pthread_mutex_unlock(&mu);
    }

    void write(uint32_t addr, uint32_t data, uint8_t wen) {
      pthread_mutex_lock(&mu);
      ethRequest.request(wen, addr, data);
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
     send_packet(&pkt, 22);
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

     write(AXIETH_TCTL, 0);
     write(AXIETH_RCTL, 0);
     // TODO: Wait until renegotiation has completed
  }


   /**
    * =================================
    * MDIO functions
    * ================================+
    */
   /**
    * Spin until you can send another mdio message
    * or timesout if doesnt succeed after timtout loops
    */
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


   void send_packet(tx_packet *pkt, int len) {
     // Basic strategy to write a packet
     // 1) Write destination mac [0 - 5]     len: 6
     // 2) Write source mac      [6 - 11]    len: 
     // 3) Write data length     [12 - 13]   len: 2
     // 4) Write data
       
     // NOTE: len is the length of the data
     // First copy over the dst / src mac addresses
        
     bool sending=true;
     while(sending) {
       uint32_t txctrl = read(AXIETH_TCTL);
       sending = ((txctrl & AXIETH_TCTL_STATUS) == 0x01);
     }

     // Write dst mac
     uint32_t mac_first = 0;
     mac_first |= pkt->dst_mac_addr[0];
     mac_first |= pkt->dst_mac_addr[1] << 8;
     mac_first |= pkt->dst_mac_addr[2] << 16;
     mac_first |= pkt->dst_mac_addr[3] << 24;

     uint32_t mac_second = 0;
     mac_second |= pkt->dst_mac_addr[4];
     mac_second |= pkt->dst_mac_addr[5] << 8;
     mac_second |= pkt->src_mac_addr[0] << 16;
     mac_second |= pkt->src_mac_addr[1] << 24;

     uint32_t mac_third = 0;
     mac_third |= pkt->src_mac_addr[2];
     mac_third |= pkt->src_mac_addr[3] << 8;
     mac_third |= pkt->src_mac_addr[4] << 16;
     mac_third |= pkt->src_mac_addr[5] << 24;

     write(0x00, mac_first);
     write(0x04, mac_second);
     write(0x08, mac_third);
 
     uint32_t type_length = 0;
     type_length |= pkt->type_length;
     type_length |= ((uint32_t) pkt->data[0]) << 16;
     type_length |= ((uint32_t) pkt->data[1]) << 24;
     write(0x0C, type_length);


     // Write packet to relevant buffer
     // TODO: This is likely to produce an odd error
     //       if len = 1500
     uint32_t ptr = 0x010;
     for (int i=0; i < (len - 2); i = i + 4) {
       // write out every part of the packet up to the length
       uint32_t tmp_data = 0x0;
       tmp_data |= pkt->data[i + 2];
       tmp_data |= ((uint32_t) pkt->data[i + 3]) << 8;
       tmp_data |= ((uint32_t) pkt->data[i + 4]) << 16;
       tmp_data |= ((uint32_t) pkt->data[i + 5]) << 24;
       write(ptr + i, tmp_data);
     }
     // Set the length of the packet as specified by the IP core
     setTxLen(len);
      
     // Signal Transmit ready
     setTxReady();
    
   }

  void receive_packet_from_pong(rx_packet *pkt) {
    // TODO: fill rx_packet with receive data
    // Read the status bit until its set
    // Read the data, clear it
     
    // spin until you recieve something
    printf("Waiting until recieved packet\n");
    uint32_t present = 0;
    while (!present) {
      present = read(AXIETH_RCTL_PONG) & AXIETH_RCTL_STATUS;
    }
    printf("Made it past this loop\n");
    
    // Copy over bytes 0 - 1512
    uint32_t macs[3];
    macs[0] = read(0x1800);
    macs[1] = read(0x1804);
    macs[2] = read(0x1808);

    uint8_t *byte_macs = (uint8_t *) macs;  
  
    for (int i=0; i < 6; i++) {
      pkt->dst_mac_addr[i] = byte_macs[i];
      pkt->src_mac_addr[i] = byte_macs[6+i];  
    }
    if (verbose) {
      printf("pkt->dst_mac_addr: %x:%x:%x:%x:%x:%x\n", 
             pkt->dst_mac_addr[0], pkt->dst_mac_addr[1],
             pkt->dst_mac_addr[2], pkt->dst_mac_addr[3],
             pkt->dst_mac_addr[4], pkt->dst_mac_addr[5]);
      printf("pkt->src_mac_addr: %x:%x:%x:%x:%x:%x\n",
             pkt->src_mac_addr[0], pkt->src_mac_addr[1],
             pkt->src_mac_addr[2], pkt->src_mac_addr[3],
             pkt->src_mac_addr[4], pkt->src_mac_addr[5]);
    }
    uint32_t size_and_data = read(0x180C);
    pkt->type_length = ((uint16_t *) &size_and_data)[0];
    
    if (verbose)
      printf("pkt->type_length: %d\n", pkt->type_length);
 
    if (AXIETH_RX_PKT_LEN < 2) 
      printf("AXIETH_RX_PKT_LEN must be > 2\n");


    printf("Beginning data read\n");
    pkt->data[0] = ((uint8_t *) &size_and_data)[3];
    pkt->data[1] = ((uint8_t *) &size_and_data)[4];


    int starting_offset = 2;
    for (int i = 0; i < (AXIETH_RX_PKT_LEN - starting_offset); i = i + 4) {
      uint32_t tmp_data = read(0x1810 + i);
      uint8_t *tmp_data_arr = (uint8_t *) &tmp_data;
      pkt->data[i + starting_offset] = tmp_data_arr[0];
      pkt->data[i + starting_offset + 1] = tmp_data_arr[1];
      pkt->data[i + starting_offset + 2] = tmp_data_arr[2];
      pkt->data[i + starting_offset + 3] = tmp_data_arr[3];
    }
    
    // Set the rx status bit back to 0 so that you can continue reading
    write(AXIETH_RCTL_PONG, 0);
   }

 void receive_packet_from_ping(rx_packet *pkt) {
    // TODO: fill rx_packet with receive data
    // Read the status bit until its set
    // Read the data, clear it
     
    // spin until you recieve something
    printf("Waiting until recieved packet\n");
    uint32_t present = 0;
    while (!present) {
      present = read(AXIETH_RCTL) & 0x01;
    }
    printf("Made it past this loop\n");
    
    // Copy over bytes 0 - 1512
    uint32_t macs[3];
    macs[0] = read(0x1000);
    macs[1] = read(0x1004);
    macs[2] = read(0x1008);

    uint8_t *byte_macs = (uint8_t *) macs;  
  
    for (int i=0; i < 6; i++) {
      pkt->dst_mac_addr[i] = byte_macs[i];
      pkt->src_mac_addr[i] = byte_macs[6+i];  
    }
    if (verbose) {
      printf("pkt->dst_mac_addr: %x:%x:%x:%x:%x:%x\n", 
             pkt->dst_mac_addr[0], pkt->dst_mac_addr[1],
             pkt->dst_mac_addr[2], pkt->dst_mac_addr[3],
             pkt->dst_mac_addr[4], pkt->dst_mac_addr[5]);
      printf("pkt->src_mac_addr: %x:%x:%x:%x:%x:%x\n",
             pkt->src_mac_addr[0], pkt->src_mac_addr[1],
             pkt->src_mac_addr[2], pkt->src_mac_addr[3],
             pkt->src_mac_addr[4], pkt->src_mac_addr[5]);
    }
    uint32_t size_and_data = read(0x100C);
    pkt->type_length = ((uint16_t *) &size_and_data)[0];
    
    if (verbose)
      printf("pkt->type_length: %d\n", pkt->type_length);
 
    if (AXIETH_RX_PKT_LEN < 2) 
      printf("AXIETH_RX_PKT_LEN must be > 2\n");


    printf("Beginning data read\n");
    pkt->data[0] = ((uint8_t *) &size_and_data)[3];
    pkt->data[1] = ((uint8_t *) &size_and_data)[4];


    int starting_offset = 2;
    for (int i = 0; i < (AXIETH_RX_PKT_LEN - starting_offset); i = i + 4) {
      uint32_t tmp_data = read(0x1010 + i);
      uint8_t *tmp_data_arr = (uint8_t *) &tmp_data;
      pkt->data[i + starting_offset] = tmp_data_arr[0];
      pkt->data[i + starting_offset + 1] = tmp_data_arr[1];
      pkt->data[i + starting_offset + 2] = tmp_data_arr[2];
      pkt->data[i + starting_offset + 3] = tmp_data_arr[3];
    }
    
    // Set the rx status bit back to 0 so that you can continue reading
    write(AXIETH_RCTL, 0);
   }

  void receive_packet(rx_packet *pkt) {
    if (rx_is_ping) {
      receive_packet_from_ping(pkt);
    } else {
      receive_packet_from_pong(pkt);
    }
    rx_is_ping = !rx_is_ping;
  }  
  
};


#include "raw_echo.cpp"

int main(int argc, char* argv[]) {
  EthIndication ethIndication(IfcNames_EthIndicationH2S, IfcNames_EthRequestS2H);
 
  raw_echo(&ethIndication); 
}
