/*
  Copyright (c) 2015 Andrew McDonnell <bugs@andrewmcdonnell.net>

  This file is part of SentriFarm Radio Relay.

  SentriFarm Radio Relay is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  SentriFarm Radio Relay is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with SentriFarm Radio Relay.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SX1276REG_H__
#define SX1276REG_H__

// The following register naming convention follows SX1276 Datasheet chapter 6
#define SX1276REG_Fifo              0x00
#define SX1276REG_OpMode            0x01
#define SX1276REG_FrfMsb            0x06
#define SX1276REG_FrfMid            0x07
#define SX1276REG_FrfLsb            0x08
#define SX1276REG_PaConfig          0x09
#define SX1276REG_PaRamp            0x0A
#define SX1276REG_Ocp               0x0B
#define SX1276REG_Lna               0x0C
#define SX1276REG_FifoAddrPtr       0x0D
#define SX1276REG_FifoTxBaseAddr    0x0E
#define SX1276REG_FifoRxBaseAddr    0x0F
#define SX1276REG_FifoRxCurrentAddr 0x10
#define SX1276REG_IrqFlagsMask      0x11
#define SX1276REG_IrqFlags          0x12
#define SX1276REG_FifoRxNbBytes     0x13
#define SX1276REG_RxHeaderCntValueMsb  0x14
#define SX1276REG_RxHeaderCntValueLsb  0x15
#define SX1276REG_RxPacketCntValueMsb  0x16
#define SX1276REG_RxPacketCntValueLsb  0x17
#define SX1276REG_ModemStat         0x18
#define SX1276REG_PacketSnr         0x19
#define SX1276REG_PacketRssi        0x1A
#define SX1276REG_Rssi              0x1B
#define SX1276REG_ModemConfig1      0x1D
#define SX1276REG_ModemConfig2      0x1E
#define SX1276REG_SymbTimeoutLsb    0x1F
#define SX1276REG_PreambleMSB       0x20
#define SX1276REG_PreambleLSB       0x21
#define SX1276REG_PayloadLength     0x22
#define SX1276REG_MaxPayloadLength  0x23
#define SX1276REG_FifoRxByteAddrPtr 0x25
#define SX1276REG_DioMapping1       0x40
#define SX1276REG_DioMapping2       0x41
#define SX1276REG_Version           0x42

// Bandwidth settings, for bits 4-7 of ModemConfig1
#define SX1276_LORA_CODING_RATE_4_5 0x01
#define SX1276_LORA_CODING_RATE_4_6 0x02
#define SX1276_LORA_CODING_RATE_4_7 0x03
#define SX1276_LORA_CODING_RATE_4_8 0x04

#define SX1276_LORA_BW_7800   0x0
#define SX1276_LORA_BW_10400  0x1
#define SX1276_LORA_BW_15600  0x2
#define SX1276_LORA_BW_20800  0x3
#define SX1276_LORA_BW_31250  0x4
#define SX1276_LORA_BW_41700  0x5
#define SX1276_LORA_BW_62500  0x6
#define SX1276_LORA_BW_125000  0x7
#define SX1276_LORA_BW_250000  0x8
#define SX1276_LORA_BW_500000  0x9

#endif
