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

/* MDIO Address Register and masks */
#define AXIETH_MDIO_ADDR   0x00007E4  // MDIO address register
#define AXIETH_MDIO_WR     0x00007E8  // MDIO write data register
#define AXIETH_MDIO_RD     0x00007EC  // MDIO read data register
#define AXIETH_MDIO_CTRL   0x00007F0  // MDIO control register

#define AXIETH_MDIO_REGADDR_MASK  0x0000001F  // Register address mask
#define AXIETH_MDIO_PHYADDR_MASK  0x000003E0  // Phy address mask
#define AXIETH_MDIO_PHYADDR_SHIFT 5          
#define AXIETH_MDIO_OP_MASK       0x00000400  // RD/WR operation mask

#define AXIETH_MDIOWR_WRDATA_MASK 0x0000FFFF  // Data to be Written mask
#define AXIETH_MDIORD_RDDATA_MASK 0x0000FFFF  // Data to be Read mask

#define AXIETH_MDIOCTRL_STATUS_MASK  0x00000001  // MDIO status mask
#define AXIETH_MDIOCTRL_EN_MASK      0x00000008  // MDIO enable

// MII control stuff
#define MII_BMCR              0x00
#define MII_CTRL1000          0x09
#define MII_STAT1000          0x0a
#define MII_ADVERTISE         0x04

/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

#define ADVERTISE_FULL    (ADVERTISE_100FULL | ADVERTISE_10FULL | \
          ADVERTISE_CSMA)
#define ADVERTISE_ALL   (ADVERTISE_10HALF | ADVERTISE_10FULL | \
          ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Basic mode control register. */
#define BMCR_RESV           0x003f  /* Unused...                   */
#define BMCR_SPEED1000      0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST           0x0080  /* Collision test              */
#define BMCR_FULLDPLX       0x0100  /* Full duplex                 */
#define BMCR_ANRESTART      0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE        0x0400  /* Isolate data paths from MII */
#define BMCR_PDOWN          0x0800  /* Enable low power state      */
#define BMCR_ANENABLE       0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100       0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK       0x4000  /* TXD loopback bits           */
#define BMCR_RESET          0x8000  /* Reset to default state      */
#define BMCR_SPEED10        0x0000  /* Select 10Mbps               */


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
  uint8_t dst_mac_addr[6];
  uint8_t src_mac_addr[6];
  uint16_t type_length;
  uint8_t data[AXIETH_RX_PKT_LEN];
} __attribute__ ((packed));

#endif
