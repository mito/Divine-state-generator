#ifndef NEG_CYC_PROCESS_MESSAGES_HH
#define NEG_CYC_PROCESS_MESSAGES_HH

void process_message(char *buf, int size, int src, int tag, bool urgent);
void process_message_about_counterexample_reconstruction
                    (char *buf, int size, int src, int tag, bool urgent);
void process_message_about_counterexamples_path_to_cycle_search
                    (char *buf, int size, int src, int tag, bool urgent);
void manager_manages_found_negative_cycle();

#endif
