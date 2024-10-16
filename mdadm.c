#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"

int mountStatus = 0;

int opHelper(int DiskID, int BlockID, int Command, int Reserved) {
        int disk = DiskID;
        int block = BlockID;
        int command = Command;
        int reserved = Reserved;

        int op = disk|block<<4|command<<12|reserved<<20;

        return op;
}


int mdadm_mount(void) {
   int opValue = opHelper(0, 0, 0, 0);

        if (mountStatus == 1) {
                return -1;
        }

        if (jbod_operation(opValue, NULL) == 0) {
                mountStatus = 1;
                return 1;
        }

        else {
                return -1;
        }
}

int mdadm_unmount(void) {
  int opValue = opHelper(0,0,1,0);

        if (mountStatus == 0) {
                return -1;
        }

        if (jbod_operation(opValue, NULL) == 0) {
                mountStatus = 0;
                return 1;
        }

        else {
                return -1;
        }
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
    int lengthOfRead = read_len;

        if (start_addr + read_len > 1048576) {
                return -1;
        }

        else if (read_len > 1024) {
                return -2;
        }

        else if (mountStatus == 0) {
                return -3;
        }

        else if (read_len > 0 && read_buf == NULL) {
                return -4;
        }

        int DiskID = start_addr/65536;
        int BlockID = (start_addr % 65536)/256;


        //first case
        uint32_t JBOD_STD = JBOD_SEEK_TO_DISK<<12;
        JBOD_STD |= DiskID<<0;
        if (jbod_operation(JBOD_STD, read_buf) == -1) {
                return -1;
        }
        
        uint32_t JBOD_STB = JBOD_SEEK_TO_BLOCK<<12;
        JBOD_STB |= BlockID<<4;
        if (jbod_operation(JBOD_STB, read_buf) == -1) {
                return -1;
        }

        uint8_t tempBuf[256];
        int offset = start_addr % 256;

        uint32_t op = JBOD_READ_BLOCK;
        op = op<<12;
        jbod_operation(op, tempBuf);
        memcpy(read_buf, tempBuf+offset, read_len);


        BlockID += 1;
        if (BlockID == 256) {
                DiskID += 1;
                BlockID = 0;
        }


        //second case
        lengthOfRead = read_len-(256-offset);
        int bytesRead = 256-offset;
        while (lengthOfRead > 256) {

                uint32_t JBOD_STD = JBOD_SEEK_TO_DISK<<12;
                JBOD_STD |= DiskID<<0;
                if (jbod_operation(JBOD_STD, read_buf) == -1) {
                        return -1;
                }
                uint32_t JBOD_STB = JBOD_SEEK_TO_BLOCK<<12;
                JBOD_STB |= BlockID<<4;
                if (jbod_operation(JBOD_STB, read_buf) == -1) {
                        return -1;
                }


                uint32_t op = JBOD_READ_BLOCK;
                op = op<<12;
                jbod_operation(op, tempBuf);
                memcpy(read_buf+bytesRead, tempBuf, lengthOfRead);

                bytesRead += 256;
                lengthOfRead -= 256;

                BlockID += 1;
                if (BlockID == 256) {
                        DiskID += 1;
                        BlockID = 0;
                }
        }

        //third case
        if (lengthOfRead > 0) {
                uint32_t JBOD_STD = JBOD_SEEK_TO_DISK<<12;
                JBOD_STD |= DiskID<<0;
                if (jbod_operation(JBOD_STD, read_buf) == -1) {
                        return -1;
                }

                uint32_t JBOD_STB = JBOD_SEEK_TO_BLOCK<<12;
                JBOD_STB |= BlockID<<4;
                if (jbod_operation(JBOD_STB, read_buf) == -1) {
                        return -1;
                }

                uint32_t op = JBOD_READ_BLOCK;
                op = op<<12;
                jbod_operation(op, tempBuf);
                memcpy(read_buf+bytesRead, tempBuf, lengthOfRead);
        }
        return read_len;
}
