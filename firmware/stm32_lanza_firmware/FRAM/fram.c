/*
 * fram.c
 *
 *  Created on: Dec 17, 2025
 *      Author: pzaragoza
 */

#include "fram.h"

static uint16_t crc16_ccitt_false(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static uint8_t metaIsValid(MetaFrame_t *meta) {
    if (meta->commit != FRAM_META_COMMIT_VALUE) return 0;
    if (meta->write_idx >= FRAM_DATA_SLOTS) return 0;
    if (meta->count > FRAM_DATA_SLOTS) return 0;

    return (meta->crc == crc16_ccitt_false((const uint8_t *)meta, offsetof(MetaFrame_t, crc)));
}

static uint8_t frameIsValid(const DataFrame_t *frame) {
    if (frame->commit != FRAM_FRAME_COMMIT_VALUE) return 0;

    return (frame->crc == crc16_ccitt_false((const uint8_t*)frame, offsetof(DataFrame_t, crc)));
}

static inline uint8_t seqIsNewer(uint8_t a, uint8_t b) {
	return (uint8_t)((uint8_t)(a - b) < 128);
}

static HAL_StatusTypeDef metaClear(MB85RS256B_t *fram, uint16_t addr) {
    MetaFrame_t meta = {0};

    return MB85RS256B_Write(fram, addr, (const uint8_t*)&meta, sizeof(meta));
}

static HAL_StatusTypeDef metaWriteSafe(MB85RS256B_t *fram, uint16_t addr, MetaFrame_t *meta) {
	meta->commit = 0;
	meta->crc = crc16_ccitt_false((const uint8_t*)meta, offsetof(MetaFrame_t, crc));

    HAL_StatusTypeDef status = MB85RS256B_Write(fram, addr, (const uint8_t*)meta, sizeof(*meta));
    if (status != HAL_OK) return status;

    uint8_t c = FRAM_META_COMMIT_VALUE;
    return MB85RS256B_Write(fram, (uint16_t)(addr + offsetof(MetaFrame_t, commit)), &c, 1);
}

static HAL_StatusTypeDef dataWriteSafe(MB85RS256B_t *fram, uint16_t addr, DataSample_t *data) {
	DataFrame_t frame = {0};
	frame.data = *data;
	frame.commit = 0;

	frame.crc = crc16_ccitt_false((const uint8_t*)&frame, offsetof(DataFrame_t, crc));

	HAL_StatusTypeDef status = MB85RS256B_Write(fram, addr, (const uint8_t*)&frame, sizeof(frame));
	if (status != HAL_OK) return status;

	uint8_t c = FRAM_FRAME_COMMIT_VALUE;
	return MB85RS256B_Write(fram, (uint16_t)(addr + offsetof(DataFrame_t, commit)), &c, 1);
}

static inline uint16_t dataSlotAddr(uint16_t slot_idx) {
    return (uint16_t)(FRAM_DATA_START + slot_idx * FRAM_SLOT_SIZE);
}

HAL_StatusTypeDef FRAM_SelfTest(MB85RS256B_t *fram) {
    uint8_t w[8] = { 0x5A, 0xA5, 0xC3, 0x3C, 0x00, 0xFF, 0x12, 0x34 };
    uint8_t r[8];

    HAL_StatusTypeDef status;

    status = MB85RS256B_Write(fram, FRAM_TEST_START, w, 8);
    if (status != HAL_OK) return status;

    status = MB85RS256B_Read(fram, FRAM_TEST_START, r, 8);
    if (status != HAL_OK) return status;

    if (memcmp(w, r, 8) != 0) return HAL_ERROR;

    return HAL_OK;
}


HAL_StatusTypeDef FRAM_Init(FramRing_t *mem) {
	HAL_StatusTypeDef status;

	// Inicializar FRAM
	status = MB85RS256B_Init(mem->fram);
	if (status != HAL_OK) return status;

	status = FRAM_SelfTest(mem->fram);
	if (status != HAL_OK) return status;

	MetaFrame_t metaA, metaB, best;

	// Leer ambas cabeceras meta
	status = MB85RS256B_Read(mem->fram, FRAM_META_A_START, (uint8_t *)&metaA, sizeof(metaA));
	if (status != HAL_OK) return status;
	status = MB85RS256B_Read(mem->fram, FRAM_META_B_START, (uint8_t *)&metaB, sizeof(metaB));
	if (status != HAL_OK) return status;

	uint8_t validA = metaIsValid(&metaA);
	uint8_t validB = metaIsValid(&metaB);

	// Meta A y Meta B validas: se guarda la más nueva
	if (validA && validB) {
		best = (seqIsNewer(metaA.seq, metaB.seq)) ? metaA : metaB;
		//printf("Both meta valid\r\n");
	}
	// Meta A o Meta B validas: se guarda la valida
	else if (validA || validB) {
		best = (validA) ? metaA : metaB;
		//printf("Only meta %c valid\r\n", (validA) ? 'A' : 'B');
	}
	// Meta A y Meta B invalidas: se inicializa
	else {
		//printf("None meta valid\r\n");

		best = (MetaFrame_t){0};
		best.write_idx = 0;
		best.count = 0;
		best.seq = 0;

		status = metaWriteSafe(mem->fram, FRAM_META_A_START, &best);
		if (status != HAL_OK) return status;

		status = metaClear(mem->fram, FRAM_META_B_START);
		if (status != HAL_OK) return status;
	}

	// Guardar contexto
	mem->write_idx = best.write_idx;
	mem->count = best.count;
	mem->seq = best.seq;

	return HAL_OK;
}

HAL_StatusTypeDef FRAM_SaveData(FramRing_t *mem, DataSample_t *data) {
	HAL_StatusTypeDef status;

	uint16_t addr = dataSlotAddr(mem->write_idx);
	//printf("Write Slot: %u (0x%04X)\r\n", mem->write_idx, addr);

	status = dataWriteSafe(mem->fram, addr, data);
	if (status != HAL_OK) return status;

	mem->write_idx = (uint16_t)((mem->write_idx + 1) % FRAM_DATA_SLOTS);
	mem->count = (mem->count < FRAM_DATA_SLOTS) ? mem->count + 1 : FRAM_DATA_SLOTS;

	//printf("Write Count: %u\r\n", mem->count);

	mem->seq++;

	MetaFrame_t meta = {0};
	meta.write_idx = mem->write_idx;
	meta.count = mem->count;
	meta.seq = mem->seq;

	uint16_t meta_addr = (meta.seq & 1) ? FRAM_META_B_START : FRAM_META_A_START;
	//printf("Write Meta %c\r\n", (meta_addr == FRAM_META_A_START) ? 'A':'B');
	return metaWriteSafe(mem->fram, meta_addr, &meta);
}

HAL_StatusTypeDef FRAM_GetSlot(FramRing_t *mem, uint16_t slot, DataSample_t *data, uint8_t *valid) {
	HAL_StatusTypeDef status;

	uint16_t addr = dataSlotAddr(slot);
	//printf("Read Slot: %u (0x%04X)\r\n", slot, addr);

	DataFrame_t frame;
	status = MB85RS256B_Read(mem->fram, addr, (uint8_t *)&frame, sizeof(frame));
	if (status != HAL_OK) return status;

	*valid = frameIsValid(&frame);
	//printf("Read Valid: %u\r\n", *valid);

	*data = frame.data;
	return HAL_OK;
}

HAL_StatusTypeDef FRAM_WriteData(FramRing_t *mem, uint16_t addr, DataSample_t *data) {
	HAL_StatusTypeDef status = dataWriteSafe(mem->fram, addr, data);

	return status;
}

HAL_StatusTypeDef FRAM_WriteDeviceInfo(FramRing_t *mem, DeviceFrame_t *dev_info) {
	HAL_StatusTypeDef status = MB85RS256B_Write(mem->fram, FRAM_DEVICE_START, (uint8_t *)dev_info, sizeof(*dev_info));

	return status;
}

HAL_StatusTypeDef FRAM_Reset(FramRing_t *mem) {
	HAL_StatusTypeDef status;

	const uint8_t chunk[32] = {0};

	for (uint16_t slot = 0; slot < FRAM_DATA_SLOTS; ++slot) {
		uint16_t addr = dataSlotAddr(slot);

		status = MB85RS256B_Write(mem->fram, addr, chunk, sizeof(chunk));
		if (status != HAL_OK) return status;
	}

	MetaFrame_t meta = {0};
	meta.write_idx = 0;
	meta.count = 0;
	meta.seq = 0;

	status = metaWriteSafe(mem->fram, FRAM_META_A_START, &meta);
	if (status != HAL_OK) return status;

	status = metaClear(mem->fram, FRAM_META_B_START);
	if (status != HAL_OK) return status;

	mem->write_idx = meta.write_idx;
	mem->count = meta.count;
	mem->seq = meta.seq;

	return HAL_OK;
}

HAL_StatusTypeDef FRAM_EraseAll(FramRing_t *mem) {
	HAL_StatusTypeDef status;

	const uint8_t chunk[32] = {0};

	for (uint16_t addr = 0; addr < MB85RS256B_SIZE; addr += sizeof(chunk)) {
		status = MB85RS256B_Write(mem->fram, addr, chunk, sizeof(chunk));
		if (status != HAL_OK) return status;
	}

	mem->write_idx = 0;
	mem->count = 0;
	mem->seq = 0;

	return HAL_OK;
}
