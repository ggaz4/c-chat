#ifndef NETWORK_H
#define NETWORK_H

#define REGISTER_BYTE   'R'
#define UNREGISTER_BYTE 'U'
#define CONNECT_BYTE    'C'
#define LISTEN_BYTE     'L'

int extract_status_code(char *plaintext);

void received_bytes_increase_and_report(const size_t *rxb, size_t *t_rxb, const char *tag, int flag);

void transmitted_bytes_increase_and_report(const size_t *txb, size_t *t_txb, const char *tag, int flag);

#endif //NETWORK_H
