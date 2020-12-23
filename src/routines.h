#ifndef ROUTINES_H
#define ROUTINES_H


void close_program(void);

uint8_t *get_min_ram_address(void);
uint8_t *get_max_ram_address(void);
void copy_data(void *from, void *to, uint24_t amount, uint8_t copy_direction);

uint8_t find_ti_file_type(uint8_t editor_file_type);
bool copy_file(char *file_name, uint8_t ti_file_type);
bool save_file(char *file_name, uint8_t ti_file_type);

// File-specific edit routines
void create_goto_undo_action(void);
void editor_goto(uint24_t offset);
void write_nibble(uint8_t nibble_byte, uint8_t sel_nibble);
void create_file_insert_bytes_undo_action(uint8_t num_bytes);
bool file_insert_bytes(uint24_t offset, uint8_t num_bytes);
void create_file_delete_bytes_undo_action(void);
bool file_delete_bytes(uint24_t offset, uint8_t num_bytes);
void undo_action(void);

#endif