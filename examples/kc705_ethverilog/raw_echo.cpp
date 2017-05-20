//#include "EthIndication.h"
#include "GeneratedTypes.h"
#include "axiethlite.hpp"
#include "eth.hpp"
#include "raw_echo.h"

int raw_echo(EthIndication *ethIndication) {
  // Initialie the interface
  ethIndication->axi_eth_init(7);
  sleep(3);
  
  tx_packet tx;
  rx_packet rx;
  int i = 1000;
  while (i > 0) {
    ethIndication->receive_packet(&rx);
    
    for (int j = 0; j < 6; j++) {
      tx.dst_mac_addr[j] = rx.src_mac_addr[j];
    }    
    for (int j = 0; j < 6; j++) {
      tx.src_mac_addr[j] = rx.dst_mac_addr[j];
    }
    tx.type_length = rx.type_length;
    int length = tx.type_length;
    if (tx.type_length > 1516) {
      length = 1500;
    }
    for (int j = 0; j < tx.type_length; j++) {
      tx.data[j] = rx.data[j];
      if (verbose)
        printf("  tx->data[%d]: %x\n", j, tx.data[j]);
    }
  
    if (verbose) {

     printf("  tx->src: %x:%x:%x:%x:%x:%x\n",
         tx.src_mac_addr[0], tx.src_mac_addr[1],
         tx.src_mac_addr[2], tx.src_mac_addr[3],
         tx.src_mac_addr[3], tx.src_mac_addr[5]);
     printf("  tx->dst: %x:%x:%x:%x:%x:%x\n",
         tx.dst_mac_addr[0], tx.dst_mac_addr[1],
         tx.dst_mac_addr[2], tx.dst_mac_addr[3],
         tx.dst_mac_addr[3], tx.dst_mac_addr[5]);   
     printf("length: %d\n", length); 
    }


    printf("Recieved packet\n");
    ethIndication->send_packet(&tx, length);
    printf("Echoed packet: len %d\n", tx.type_length);
    i--;
  }
  return 0;
}
