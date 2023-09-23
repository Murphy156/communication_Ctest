#include <stdio.h>
#include <stdint.h>

#define CMD_NONE 0xff
#define max_length 128
uint8_t	 rd_p,wr_p;
uint8_t	 data_buff[max_length];

uint16_t calculateCRC16(uint8_t arr[7]) {
    uint16_t crc = 0xFFFF;
    uint16_t poly = 0x8005;

    for (int i = 0; i < 7; i++) {
        printf("循环次数: %d\n", i);
        printf("当前处理的字节: %02X\n", arr[i]);
        crc ^= (arr[i] << 8);

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ poly;
                printf("crc2: %04X\n", crc);
            } else {
                crc <<= 1;
                printf("crc3: %04X\n", crc);
            }
        }
    }
    return crc & 0xFFFF;
}

/**
  * @brief  这个函数是用来将data_buff的帧数据读取到用来协议解析的函数中
	*	@param 	第一个参数指向data_buff
	*	@param 	第二个参数指向用来存放一帧数据的数组中
	*	@note 	无
  * @retval 返回1时，说明了存入的数据和读出的数据同步处理完了，即存入多少数据就已经读完多少数据一帧一帧地处理；
						返回0时，说明了存入了数据的，但没有将这些存入的数据解析完
  */
uint8_t PopArr(uint8_t *arr ,uint8_t *data)
{
	if(rd_p == wr_p)return 1;
	*data = arr[rd_p];
	rd_p= (rd_p+1)%max_length;
	return 0;
}

/**
 * @brief   查找帧头
 *          直接操作读指针
 * @return  0：没有找到帧头，1:找到了帧头，并且当前rd_p指向0x55的下一个位置即命令代码
 */
static uint8_t recvBuff_Find_Header(void)
{
    uint8_t index;
	index = rd_p;
    printf("the current rd_p:%d\n", index);
    printf("the databuff: %02X\n", data_buff[index]);
    //如果相等，则读指针和写指针指向同一位置,这里要先将他注释掉
    if(rd_p == wr_p) return 0;

	if(data_buff[index] != 0xAA)
	{
		rd_p = (index +1) % max_length;
        printf("cann't find 0xaa so the rd_p1 point to next: %d\n", rd_p);  // 输出当前 rd_p 的值
		return 0;
	}
	else 
	{
        //对应如果找到aa后，将指针加1去到下一个位
		index = (rd_p + 1) % max_length;
		if(data_buff[index] != 0x55)
		{
            printf("have find the 0xaa but not match the 0x55: %d\n", rd_p);  // 输出当前 rd_p 的值
            //当前位找不到，加1寻找指向下一个
			rd_p = (index +1) % max_length;
			return 0;
		}
		else
		{
            //找到了当前55，所以包头的位置锁定，将读指针指向命令位
            printf("have find the 0xaa and the 0x55: %d\n", rd_p);
            rd_p = (index +1) % max_length;
            printf("the current rp_d point to next: %d\n", rd_p);  // 输出当前 rd_p 的值
            return 1;
		}
	}	
}

/**
 * @brief   函数用于按反序合并两个字节为一个16位整数
 * @return  返回一个两个字节的数据
 */
uint16_t mergeBytesToUint16(uint8_t data[2]) {
    uint16_t mergedData = 0;

    for (int i = 0; i < 4; i++) {
        mergedData |= ((uint16_t)data[i] << (i * 8));
    }
    return mergedData;
}

/**
 * @brief   输入一个指针数据，用来处理参数的函数，将指针对应的数据提取四个字节出来，返回的是速度值，可能有负数。
 * @para    处理的指针数据
 * @return  返回一个四字节的数据，
 */
uint32_t mergeParametersToUint32(void) {
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




/**
 * @brief   查找帧头,验证数据的正确性即验证CRC的值
 * @return  返回一个标志状态，状态代表找到帧头以及数据正确并且通过，1代表通过，0代表错误
 */
static uint8_t protocol_Check_header_CRC(void)
{
    uint8_t tmp_arr[7];
    uint8_t tmp_crc[2];
    uint16_t cal_crc16;
    uint16_t send_crc16;
    uint8_t pack_header_flag;
    uint8_t header_CRC_Ready_flat;
    pack_header_flag = recvBuff_Find_Header();
    uint8_t tmp_ptr;
    //没找到数据头
    if(0 == pack_header_flag)
    {
        return header_CRC_Ready_flat = 0;
    }
    else
    {
        //将指向命令的的rd_p保护起来，用于下面的直接使用
        tmp_ptr = rd_p;
        tmp_arr[0] = 0xAA;
        tmp_arr[1] = 0x55;
        //第三个位置放入命令码8位，因为在寻找包头的时候寻找结束后当前指针指向，命令码的位置
        tmp_arr[2] = data_buff[tmp_ptr];

        //对数据进行crc校验，如果校验结果正确提取出命令以及参数
        //for（init;condition;increment ）循环的控制流是，首先先执行init的值，然后进行条件的判断，判断通过，执行主体for内的内同，然后再到increment中进行操作
        for(int i = 3; i < 7; i++)
        {
            tmp_ptr = (tmp_ptr+1) % max_length;
            tmp_arr[i] = data_buff[tmp_ptr];
        }
        //假设这个for循环后，返回的是参数的最后一个值

        //输出tmp_arr的数组
        for (int i = 0; i < 7; i++) {
            printf("tmp_arr[%d]: %02X\n", i, tmp_arr[i]);
        }
        //将原始的数据包头、命令码、参数值进行计算CRC码
        cal_crc16 = calculateCRC16(tmp_arr); 


        //取出CRC校验码
        for (int j = 0; j < 2; j++)
        {
            tmp_ptr = (tmp_ptr+1) % max_length;
            tmp_crc[j] = data_buff[tmp_ptr];
        }

        //如今tmp_ptr指向校验码高位
        //将其反向合并成16位的数据
        send_crc16 = mergeBytesToUint16(tmp_crc);
        printf("send_crc16,%04X\n:", send_crc16);


        //如果发送过来的校验值不等于我收到数据后计算的校验值，则这帧数据为错误的数据，丢弃
        if (send_crc16 != cal_crc16)
        {
            // 将数据指向下一组数据
            tmp_ptr = (tmp_ptr+1) % max_length;
            rd_p = tmp_ptr;
            return 0;
        }
        else
        {
            //指向这帧数据的命令字节，由于前面我已经将rd_p保护起来，如果执行到这行命令，则代表验证通过
            return 1;
        }
    }
}


/**
 * @brief   接收的数据处理
 * @param   void
 * @return  -1：没有找到一个正确的命令.
 */
int8_t receiving_process(void)
{
    uint8_t cmd_type = CMD_NONE;
    uint8_t header_CRC_Ready_flat;
    uint32_t tmp_para;            //注意，参数的有可能是负数，如果是负数时，传输过来的数据是补码


    while (1)
    {  
        header_CRC_Ready_flat = protocol_Check_header_CRC();
        printf("the current head crc flat:%d",header_CRC_Ready_flat);
        if (1 != header_CRC_Ready_flat)
        {
            return -1;
        }
        //已经找到了帧头以及校对了CRC,此时，rd_p指向当前数据帧的命令位
        else
        {   
            printf("the cmd rd_p: %d",rd_p);
            cmd_type = data_buff[rd_p];
            //提取了命令，解析参数。
            tmp_para = mergeParametersToUint32();

            switch (cmd_type)
            {
            case 0x01:
            {
                printf("enter the 0x01");
                //处理完一帧数据后将rd_p移到下一个数据帧开头
                rd_p = (rd_p+3) % max_length;
            }
                break;
            
            default:
                return -1;
            }

        }   
    }
}


int main() {
    uint8_t initial_data[15] = {0x11, 0x61, 0x17, 0xaa, 0x55, 0x01, 0x9c, 0xff, 0xff, 0xff, 0xb3, 0x5c, 0x00, 0x00,0x11};
    wr_p = 14;

    // 正确初始化data_buff数组
    for (int i = 0; i < 15; i++) {
        data_buff[i] = initial_data[i];
    }

    while (1)
    {
        receiving_process();
    }
    
    return 0;
}