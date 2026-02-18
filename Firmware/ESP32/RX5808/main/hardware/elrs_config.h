#ifndef ELRS_CONFIG_H
#define ELRS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Load ELRS UID from NVS
 * @param uid Buffer to store 6-byte UID
 * @return true if UID was loaded successfully, false if not found or error
 */
bool elrs_config_load_uid(uint8_t uid[6]);

/**
 * @brief Save ELRS UID to NVS
 * @param uid 6-byte UID to save
 * @return true if saved successfully, false on error
 */
bool elrs_config_save_uid(const uint8_t uid[6]);

/**
 * @brief Clear ELRS UID from NVS (unbind)
 * @return true if cleared successfully, false on error
 */
bool elrs_config_clear_uid(void);

/**
 * @brief Check if a valid UID is stored
 * @return true if UID exists and is not all zeros, false otherwise
 */
bool elrs_config_has_uid(void);

/**
 * @brief Save TX sender MAC address to NVS
 * @param tx_mac 6-byte TX MAC address to save
 * @return true if saved successfully, false on error
 */
bool elrs_config_save_tx_mac(const uint8_t tx_mac[6]);

/**
 * @brief Load TX sender MAC address from NVS
 * @param tx_mac Buffer to store 6-byte TX MAC address
 * @return true if TX MAC was loaded successfully, false if not found or error
 */
bool elrs_config_load_tx_mac(uint8_t tx_mac[6]);

#endif // ELRS_CONFIG_H
