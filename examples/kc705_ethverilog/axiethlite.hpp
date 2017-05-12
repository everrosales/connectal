#ifndef AXIETHLITE_H
#define AXIETHLITE_H

#define AXIETH_GIE    0x000007F8  // Global interrupt register
#define AXIETH_GIE_EN 0x80000000  // Enable gie

#define AXIETH_TDL       0x00007F4  // TX length for ping buffer
#define AXIETH_FRAME_MSB 0x000FF00  // Mask for frame MSB
#define AXIETH_FRAME_LSB 0x00000FF  // Mask for frame LSB
#define AXIETH_FRAME     0x000FFFF  // Mask for frame length

#define AXIETH_TCTL        0x00007FC  // TX control register
#define AXIETH_TCTL_LP_EN  0x0000010  // Loop back enable
#define AXIETH_TCTL_IE     0x0000008  // TX interrupt enable
#define AXIETH_TCTL_MAC_P  0x0000002  // TX program AXI Mac Address
#define AXIETH_TCTL_STATUS 0x0000001  // TX status 

#define AXIETH_TX_PKT_LEN       1500  // TX Packet length

#define AXIETH_RCTL        0x00017FC  // RX control register
#define AXIETH_RCTL_IE     0x0000008  // RX interrupt enable
#define AXIETH_RCTL_STATUS 0x0000001  // RX status

#define AXIETH_RX_PKT_LEN       1500  // RX Packet length

#define AXIETH_MAC_ADDR_LEN        6  // MAC address length

uint8_t mac_default[6] = {0x00, 0x00, 0x5E, 0x00, 0xFA, 0xCE};

struct mac_address {
  uint8_t buf[AXIETH_MAC_ADDR_LEN];
} __attribute__((packed));

struct tx_packet {
  uint8_t dst_mac_addr[6];
  uint8_t src_mac_addr[6];
  uint16_t type_length;
  uint8_t data[AXIETH_RX_PKT_LEN];
} __attribute__ ((packed));

struct rx_packet {
  uint8_t buf[AXIETH_RX_PKT_LEN];
} __attribute__ ((packed));

#endif
