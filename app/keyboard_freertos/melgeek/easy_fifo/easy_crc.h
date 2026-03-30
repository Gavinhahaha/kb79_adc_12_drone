/*
 * easy_crc.h
 *
 *  Created on: 2021-7-21
 *      Author: layne
 */

#ifndef EASY_CRC_H_
#define EASY_CRC_H_

#include <stdint.h>

uint8_t easy_crc8_add(uint8_t crc,uint8_t byte);
uint32_t calc_crc8(const uint8_t *data, uint32_t len);
uint16_t get_crc(uint16_t len, uint8_t *pbuf);
uint16_t crc16(const uint8_t *data, uint32_t len);
uint32_t calculate_crc32(const uint8_t *data, uint32_t length);
uint32_t Crc32(unsigned char *buff, int len);
void char_to_uint32(unsigned char *buf, int len, uint32_t *crc);
uint32_t boot_crc32(uint32_t *ptr, int len);
uint16_t crc16_continue(uint16_t crc, const uint8_t *data, uint32_t len);
#endif /* EASY_CRC_H_ */
