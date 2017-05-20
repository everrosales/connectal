#ifndef ETH_H
#define ETH_H
#include "axiethlite.hpp"
#include <semaphore.h>
#include <pthread.h>

class EthRequestProxy;
class EthIndication;
// 
 class Eth {
   public:
     Eth();
     ~Eth();
     void reset();
     void read(unsigned long offset, uint8_t *buf);
     void write(unsigned long offset, const uint8_t *buf);
     void axi_eth_init(int phy_id);
     void send_packet(tx_packet *pkt, int len);
     void receive_packet(rx_packet *pkt);
//
   private:
     EthRequestProxy ethRequest;
//     EthIndication *indication;
     void receive_packet_from_pong(rx_packet *pkt);
     void receive_packet_from_ping(rx_packet *pkt);
     sem_t sem;
     uint32_t data;
     pthread_mutex_t mu;  

 };

#endif
