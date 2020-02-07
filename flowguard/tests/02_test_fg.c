//typedef struct {
//    unsigned char dest_addr[8];
//    unsigned char dest_pid;
//} S;

//void S_parse(unsigned char *data, int len, S *pf)
void S_parse(unsigned char *data)
{
    int has_dest_panid = 1;
    unsigned char whatever;
//    if (len >= 2) {
//        data += 2;
//
        if (has_dest_panid) {
              data += 2;
        }
//        else pf->dest_pid = (unsigned short) 0;
//
        whatever = *(data + 1);
//        pf->dest_addr[0] = *(data + 1);
//        pf->dest_addr[1] = *(data + 0);
//    }
}

//int main (void)
//{
//    unsigned int rnd_nmbr = 7;
//}
