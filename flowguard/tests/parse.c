typedef struct {
    unsigned char dest_addr[8];
    unsigned char dest_pid;
} S;

void S_parse(unsigned char *data, int len, S *pf)
{
    int has_dest_panid = 1;
    if (len >= 2) {
        data += 2;

        if (has_dest_panid) {
            pf->dest_pid = (unsigned short) ((int)*(data + 0) + ((int)*(data + 1) << 8));
            data += 2;
        }
        else pf->dest_pid = (unsigned short) 0;

        pf->dest_addr[0] = *(data + 1);
        pf->dest_addr[1] = *(data + 0);
     }
}
