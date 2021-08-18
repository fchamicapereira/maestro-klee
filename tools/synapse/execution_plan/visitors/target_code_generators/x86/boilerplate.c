unsigned char data_buffer[512];
unsigned data_buffer_offset;

{{global_state}}

bool synapse_runtime_configure(synapse_config_t *config) {
  {{runtime_configure}}  
}

bool nf_init() {
  {{nf_init}}
}

int nf_process(uint16_t device, uint8_t **p, uint16_t pkt_len, int64_t now, struct rte_mbuf *mbuf) {
  data_buffer_offset = 0;
  {{nf_process}}
}