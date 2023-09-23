#include <stdio.h>
#include <stdint.h>

#define BUFFER_SIZE 128

uint8_t data_buff[BUFFER_SIZE];
uint8_t rd_p = 0;

/**
 * @brief   输入一个指针数据，用来处理参数的函数，将指针对应的数据提取四个字节出来，返回的是速度值，可能有负数。
 * @para    处理的指针数据
 * @return  返回一个四字节的数据，
 */
uint32_t mergeParametersToUint32() {
    // 读取四个字节参数码，注意大端序转换
    uint32_t mergedData = 0;
    for (int i = 0; i < 4; i++) {
        printf("the current rd_p:%d",rd_p);
        rd_p = (rd_p + 1) % 128;  // 更新 rd_p
        //先将取出来的一个字节的数据扩展成4个字节的数据，然后再根据在4个字节中的位序依次平移，每次平移时都要进行32位的或操作，从而保证位次一定
        mergedData |= ((uint32_t)data_buff[rd_p] << (8 * i));
        printf("the next rd_p:%d",rd_p);
    }
    return mergedData;
}

int main() {
    // 假设数据为 0x12, 0x34, 0x56, 0x78，存储在 data_buff 中
    data_buff[0] = 0x00;
    data_buff[1] = 0xaa;
    data_buff[2] = 0x55;
    data_buff[3] = 0x01;
    data_buff[4] = 0x17;
    data_buff[5] = 0x23;
    data_buff[6] = 0x45;
    data_buff[7] = 0x56;
    data_buff[8] = 0x00;
    data_buff[9] = 0x20;
    data_buff[10] = 0x56;
    data_buff[11] = 0x78;
    data_buff[12] = 0x12;
    data_buff[13] = 0x34;
    data_buff[14] = 0x56;
    data_buff[15] = 0x78;

    // 设置读指针指向第一个参数码
    rd_p = 3;
    uint32_t mergedData = mergeParametersToUint32();

// 打印合并后的数据
    printf("Merged Data: %08X\n", mergedData);

    return 0;
}
