
package com.topvs.client;

public class dev_io_control_t
{
	public final int MAX_IO_INPUT_NUM = 16;
	public final int MAX_IO_OUTPUT_NUM = 16;
	
	public byte input[] = new byte[MAX_IO_INPUT_NUM];
	public byte output[] = new byte[MAX_IO_OUTPUT_NUM];
	public int  timer[] = new int[MAX_IO_OUTPUT_NUM];
	public byte input_num;//有效输入总数
	public byte output_num;//有效输出总数
}
